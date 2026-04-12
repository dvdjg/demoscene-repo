#!/usr/bin/env python3

import argparse
import os
import re
import shlex
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

try:
    from libtmux import Server, Session, exc
except ImportError:
    print(
        'Falta el paquete Python "libtmux". Instálalo en el mismo intérprete que usa Make '
        '(en Windows suele ser C:\\Python3xx\\python.exe):\n'
        '  python -m pip install -r requirements.txt\n'
        'o al menos: python -m pip install libtmux',
        file=sys.stderr)
    raise SystemExit(1) from None


def WaitForTcpPort(host, port, timeout=90.0):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=1.0):
                return True
        except OSError:
            time.sleep(0.15)
    return False


def HerePath(*components):
    path = Path(os.getenv('TOPDIR', ''), *components)
    if not path.exists():
        print(f"warning: '{path}' does not exists")
    return str(path)


SOCKET = 'fsuae'
SESSION = 'fsuae'
GDBSERVER = 'uaedbg.py'
REMOTE = 'localhost:2345'
TMUX_CONF = HerePath('.tmux.conf')


def _parse_uae_config(text):
    out = {}
    for raw in text.replace('\r\n', '\n').split('\n'):
        line = raw.strip()
        if not line or line.startswith(';'):
            continue
        if '=' not in line:
            continue
        k, _, v = line.partition('=')
        out[k.strip()] = v.strip()
    return out


def _stringify_uae_config(cfg):
    return ''.join(f'{k}={v}\r\n' for k, v in cfg.items())


def FindBartmanWinUAEGdb():
    """
    winuae-gdb.exe del paquete Bartman (Amiga Debug): suele estar en el mismo
    árbol que m68k-amiga-elf-gcc (p. ej. .../bin/win32/winuae-gdb.exe).
    Override: WINUAE_GDB_EXE=ruta\\winuae-gdb.exe
    """
    override = os.environ.get('WINUAE_GDB_EXE', '').strip()
    if override and Path(override).is_file():
        return Path(override).resolve()
    if sys.platform != 'win32':
        return None
    for tool in (
            'm68k-amiga-elf-gcc',
            'm68k-amiga-elf-gdb',
            'm68k-amigaos-gcc',
            'm68k-amigaos-gdb',
    ):
        w = shutil.which(tool)
        if not w:
            continue
        p = Path(w).resolve()
        for parent in p.parents:
            cand = parent / 'winuae-gdb.exe'
            if cand.is_file():
                return cand
    return None


def _winuae_debugging_trigger(host_executable):
    """Nombre del .exe en el Amiga (p. ej. cathedral.exe.dbg -> :cathedral.exe)."""
    name = Path(host_executable).name
    if name.endswith('.exe.dbg'):
        return ':' + name[:-4]
    if name.endswith('.dbg'):
        return ':' + name[:-4]
    if not name.lower().endswith('.exe'):
        return ':' + name + '.exe'
    return ':' + name


def LaunchWinUAEBartmanGdbserver(winuae_exe, args):
    """
    Windows: WinUAE de Bartman expone gdbserver RSP (no el depurador consola
    de FS-UAE). Puerto por defecto 2345 (como la extensión Amiga Debug / GDB MI).
    """
    winuae_dir = winuae_exe.parent
    floppy_path = Path(args.floppy).resolve()
    if not floppy_path.is_file():
        raise SystemExit('%s: file does not exist!' % floppy_path)
    floppy = floppy_path.as_posix()

    cfg = {}
    default_uae = winuae_dir / 'default.uae'
    if default_uae.is_file():
        cfg = _parse_uae_config(
            default_uae.read_text(encoding='utf-8', errors='replace'))
    for k in list(cfg.keys()):
        if k in ('filesystem', 'filesystem2', 'filesystem3'):
            del cfg[k]
        elif k.startswith('uaehf'):
            del cfg[k]

    if args.model == 'A1200':
        cfg['quickstart'] = 'a1200,0'
    else:
        cfg['quickstart'] = 'a500,1'

    # Sin panel de configuración de WinUAE: arranque directo a emulación (como default.uae
    # de Bartman). use_gui=yes fuerza el modo «GUI» del emulador.
    cfg['use_gui'] = 'no'
    cfg['win32.start_not_captured'] = 'yes'
    cfg['win32.nonotificationicon'] = 'yes'
    cfg['boot_rom_uae'] = 'min'
    cfg['cpu_cycle_exact'] = 'true'
    cfg['cpu_memory_cycle_exact'] = 'true'
    cfg['blitter_cycle_exact'] = 'true'
    cfg['cycle_exact'] = 'true'
    cfg['input.config'] = '1'
    cfg['input.1.keyboard.0.friendlyname'] = 'WinUAE keyboard'
    cfg['input.1.keyboard.0.name'] = 'NULLKEYBOARD'
    cfg['input.1.keyboard.0.empty'] = 'false'
    cfg['input.1.keyboard.0.disabled'] = 'false'
    cfg['input.1.keyboard.0.button.41.GRAVE'] = 'SPC_SINGLESTEP.0'
    cfg['input.1.keyboard.0.button.201.PREV'] = 'SPC_WARP.0'
    cfg['floppy0'] = floppy
    cfg['debugging_features'] = 'gdbserver'
    cfg['debugging_trigger'] = _winuae_debugging_trigger(args.executable)
    cfg.pop('statefile', None)

    fd, tmp_path = tempfile.mkstemp(suffix='.uae', prefix='demoscene-')
    os.close(fd)
    Path(tmp_path).write_text(
        _stringify_uae_config(cfg), encoding='utf-8')

    popen_kw = {
        'cwd': str(winuae_dir),
        'stdin': subprocess.DEVNULL,
    }
    subprocess.Popen(
        [str(winuae_exe), '-portable', '-f', tmp_path],
        **popen_kw)
    print(
        'WinUAE (Bartman): {}'.format(winuae_exe),
        flush=True)


def TmuxAvailable():
    """Python en Windows usa CreateProcess: hace falta tmux.exe en el PATH."""
    return shutil.which('tmux') is not None


def CleanupStaleEmulator():
    """Cierra sesiones tmux/fs-uae previas para evitar quedar enganchado."""
    if not TmuxAvailable():
        return
    top = os.getenv('TOPDIR', '').strip()
    tmux_cmd = ['tmux', '-L', SOCKET, 'kill-server']
    if top:
        conf = str(Path(top) / '.tmux.conf')
        tmux_cmd = ['tmux', '-f', conf, '-L', SOCKET, 'kill-server']
    subprocess.run(
        tmux_cmd, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
    time.sleep(0.15)
    if not top:
        return
    base = Path(top)
    for suffix in (Path('tools') / GDBSERVER, Path('config.fs-uae')):
        pat = re.escape(str(base / suffix))
        subprocess.run(
            ['pkill', '-f', pat],
            stderr=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL)


class Launchable():
    def __init__(self, name, cmd):
        self.name = name
        self.cmd = cmd
        self.window = None
        self.options = []

    def configure(self, *args, **kwargs):
        raise NotImplementedError

    def start(self, session):
        cmd = ' '.join([self.cmd] + list(map(shlex.quote, self.options)))
        # XXX: print `cmd` to display command and its arguments
        self.window = session.new_window(
            attach=False, window_name=self.name, window_shell=cmd)


class FSUAE(Launchable):
    def __init__(self):
        super().__init__('fs-uae', HerePath('tools', GDBSERVER))

    def configure(self, floppy=None, model=None, log='', debug=False):
        self.options.extend(['-e', 'fs-uae'])
        if debug:
            self.options.append('-g')
        if log:
            self.options.extend(['-l', log])
        # Now options for FS-UAE.
        self.options.append('--')
        if floppy:
            self.options.append(
                '--floppy_drive_0={}'.format(Path(floppy).resolve()))
        if model:
            self.options.append(f'--amiga_model={model}')
        if debug:
            self.options.append('--use_debugger=1')
        self.options.append('--warp_mode=1')
        self.options.append(HerePath('config.fs-uae'))


class SOCAT(Launchable):
    def __init__(self, name):
        super().__init__(name, 'socat')

    def configure(self, tcp_port):
        # The simulator will only open the server after some time has
        # passed.  To minimize the delay, keep reconnecting until success.
        self.options = [
            'STDIO', 'tcp:localhost:%d,retry,forever,interval=0.01' % tcp_port]


class GDB(Launchable):
    def __init__(self):
        gdb = os.environ.get('M68K_GDB', 'm68k-amigaos-gdb')
        super().__init__('gdb', gdb)
        # gdbtui & cgdb output is garbled if there is no delay
        self.cmd = 'sleep 0.5 && ' + self.cmd

    def configure(self, program=''):
        self.options += ['-ex=set prompt \033[35;1m(gdb) \033[0m']
        self.options += [
            '-iex=directory {}/'.format(HerePath()),
            '-ix={}'.format(HerePath('.gdbinit')),
            '-ex=set tcp connect-timeout 30',
            '-ex=target remote {}'.format(REMOTE),
            '--nh',
            '--silent',
            program]


def LaunchWithoutTmux(uae, args):
    """
    Windows / entornos sin tmux en el PATH: lanza uaedbg.py directamente.
    En Windows, si hay winuae-gdb.exe de Bartman, gdbserver ya se resolvió
    antes (salida temprana en __main__) con WinUAE nativo.
    Serial/parallel vía socat no se arrancan (no suelen existir en Windows).
    """
    top = os.getenv('TOPDIR', '').strip()
    if not top:
        raise SystemExit(
            'TOPDIR no está definido; ejecuta launch.py desde Make (export TOPDIR).')

    if args.debug == 'gdb':
        print(
            'Modo -d gdb requiere tmux (varias ventanas). Instala tmux en el PATH '
            '(p. ej. MSYS2: pacman -S tmux) o enlaza gdb a mano contra el gdbserver.',
            file=sys.stderr)
        raise SystemExit(1)

    script = Path(top) / 'tools' / GDBSERVER
    if not script.is_file():
        raise SystemExit('No existe %s' % script)

    argv = [sys.executable, str(script.resolve())] + uae.options
    env = os.environ.copy()
    pylib = str(Path(top) / 'tools' / 'pylib')
    env['PYTHONPATH'] = pylib + os.pathsep + env.get('PYTHONPATH', '')

    popen_kw = {
        'cwd': top,
        'env': env,
        'stdin': subprocess.DEVNULL,
    }
    # Consola interactiva del depurador integrado (sin -g): ventana aparte en Windows.
    if sys.platform == 'win32' and args.debug != 'gdbserver':
        popen_kw['creationflags'] = getattr(
            subprocess, 'CREATE_NEW_CONSOLE', 0)

    subprocess.Popen(argv, **popen_kw)

    if args.debug == 'gdbserver':
        host, port = REMOTE.split(':')
        if WaitForTcpPort(host, int(port)):
            print(
                'Listening for gdb connection at localhost:{}'.format(port),
                flush=True)
        else:
            raise SystemExit(
                'timeout waiting for gdbserver on {}:{}'.format(host, port))
    else:
        print(
            'uaedbg iniciado sin tmux (sin redirección serial/parallel tipo socat).',
            flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=(
            'Lanzar el efecto en emulador: FS-UAE (Linux/uaedbg) o, en Windows '
            'con toolchain Bartman, WinUAE (winuae-gdb.exe) para depuración.'))
    parser.add_argument('-f', '--floppy', metavar='ADF', type=str,
                        help='Floppy disk image in ADF format.')
    parser.add_argument('-e', '--executable', metavar='EXE', type=str,
                        help='Provide executable file for GDB debugger.')
    parser.add_argument('-d', '--debug', choices=['gdbserver', 'gdb'],
                        help=('Run gdbserver on {} and launch gdb '
                              'if requested.'.format(REMOTE)))
    parser.add_argument('-w', '--window', metavar='WIN', type=str,
                        default='fs-uae',
                        help='Select tmux window name to switch to.')
    parser.add_argument('-m', '--model', choices=['A500', 'A1200'],
                        default=os.getenv('MODEL', 'A500'),
                        help='Emulate given Amiga computer model.')
    args = parser.parse_args()

    # Check if floppy disk image file exists
    if args.floppy and not Path(args.floppy).is_file():
        raise SystemExit('%s: file does not exist!' % args.floppy)

    # Check if executable file exists.
    if args.debug and not Path(args.executable).is_file():
        raise SystemExit('%s: file does not exist!' % args.executable)

    # Windows + toolchain Bartman: WinUAE con gdbserver RSP (no FS-UAE ni uaedbg).
    if args.debug == 'gdbserver' and sys.platform == 'win32':
        winuae_gdb = FindBartmanWinUAEGdb()
        if winuae_gdb is not None:
            host, port = REMOTE.rsplit(':', 1)
            gdb_port = int(port)
            LaunchWinUAEBartmanGdbserver(winuae_gdb, args)
            if WaitForTcpPort(host, gdb_port):
                print(
                    'Listening for gdb connection at localhost:{}'.format(port),
                    flush=True)
            else:
                raise SystemExit(
                    'timeout waiting for gdbserver on {}:{}'.format(host,
                                                                     port))
            raise SystemExit(0)

    uae = FSUAE()
    uae.configure(floppy=args.floppy,
                  model=args.model,
                  log=Path(args.floppy).stem + '.log',
                  debug=args.debug)

    ser_port = SOCAT('serial')
    ser_port.configure(tcp_port=8000)

    par_port = SOCAT('parallel')
    par_port.configure(tcp_port=8001)

    if args.debug == 'gdb':
        debugger = GDB()
        debugger.configure(args.executable)

    if not TmuxAvailable():
        LaunchWithoutTmux(uae, args)
        raise SystemExit(0)

    CleanupStaleEmulator()

    subprocess.run(['tmux', '-f', TMUX_CONF, '-L', SOCKET, 'start-server'])

    server = Server(config_file=TMUX_CONF, socket_name=SOCKET)

    if server.has_session(SESSION):
        server.kill_session(SESSION)

    session = server.new_session(session_name=SESSION, attach=False,
                                 window_name=':0', window_command='sleep 1')

    # tmux attach() needs a real TTY. IDE tasks / pipes / automation often are
    # not a TTY — skip attach and leave the session running (user: tmux -L fsuae attach).
    # DEMOSCENE_LAUNCH_NO_ATTACH=1 forces the same (e.g. scripts).
    want_attach = sys.stdin.isatty() and os.environ.get(
        'DEMOSCENE_LAUNCH_NO_ATTACH', '') not in ('1', 'yes', 'true')

    try:
        uae.start(session)
        ser_port.start(session)
        par_port.start(session)
        if args.debug == 'gdb':
            debugger.start(session)

        session.kill_window(':0')
        if args.debug == 'gdb':
            session.select_window(debugger.name)
        else:
            session.select_window(args.window or par_port.name)
        if want_attach:
            Session.attach(session)
        elif args.debug == 'gdbserver':
            # Salida que ve la tarea de VS Code / Cursor (uaedbg escribe en el panel tmux).
            host, port = REMOTE.split(':')
            if WaitForTcpPort(host, int(port)):
                print(
                    'Listening for gdb connection at localhost:{}'.format(port),
                    flush=True)
            else:
                raise SystemExit(
                    'timeout waiting for gdbserver on {}:{}'.format(host, port))
    except exc.TmuxObjectDoesNotExist:
        pass
    finally:
        if want_attach:
            Server.kill(server)
