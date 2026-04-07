#!/usr/bin/env python3

import argparse
import os
import re
import shlex
import socket
import subprocess
import sys
import time
from pathlib import Path
from libtmux import Server, Session, exc


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
REMOTE = 'localhost:8888'
TMUX_CONF = HerePath('.tmux.conf')


def CleanupStaleEmulator():
    """Cierra sesiones tmux/fs-uae previas para evitar quedar enganchado."""
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
        super().__init__('gdb', 'm68k-amigaos-gdb')
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


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Launch effect in FS-UAE emulator.')
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
