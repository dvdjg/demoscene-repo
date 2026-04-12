#!/usr/bin/env python3

import argparse
import asyncio
import logging
import os
import shutil
import signal
import sys
import traceback

from debug.uae import UaeDebugger, UaeProcess
from debug.gdb import GdbConnection, GdbDisconnect, GdbStub


BREAK = 0xCF47  # no-op: 'exg.l d7,d7'


def ResolveEmulatorExecutable(emulator):
    """
    En Windows, CreateProcess no ejecuta 'fs-uae' sin .exe ni ruta; shutil.which
    tampoco siempre resuelve el nombre corto. Opcional: FS_UAE_EXE o UAE_EMULATOR.
    """
    override = os.environ.get('FS_UAE_EXE') or os.environ.get('UAE_EMULATOR')
    if override and os.path.isfile(override):
        return override
    if sys.platform == 'win32':
        if os.path.isfile(emulator):
            return os.path.abspath(emulator)
        for name in (emulator, emulator + '.exe'):
            found = shutil.which(name)
            if found:
                return found
        for root in (
                os.environ.get('ProgramFiles'),
                os.environ.get('ProgramFiles(x86)'),
                os.environ.get('LocalAppData')):
            if not root:
                continue
            for rel in (
                    r'FS-UAE\FS-UAE.exe',
                    r'FS-UAE\Portable\FS-UAE.exe',
                    r'Programs\FS-UAE\FS-UAE.exe'):
                p = os.path.join(root, rel)
                if os.path.isfile(p):
                    return p
        return emulator
    return emulator


async def UaeLaunch(loop, args):
    log_file = open(args.log, 'w') if args.log else None

    emulator_exe = ResolveEmulatorExecutable(args.emulator)
    if sys.platform == 'win32' and not os.path.isfile(emulator_exe):
        raise SystemExit(
            'No se encuentra el ejecutable de FS-UAE (%r → %r).\n'
            'En Windows, `make debug DEBUGGER=gdbserver` usa WinUAE (winuae-gdb.exe) '
            'del paquete Bartman si está junto al GCC; si caes aquí, instala FS-UAE, '
            'añádelo al PATH, o define FS_UAE_EXE con la ruta a FS-UAE.exe.\n'
            'Ejemplo: set FS_UAE_EXE=C:\\Program Files\\FS-UAE\\FS-UAE.exe'
            % (args.emulator, emulator_exe))

    # Create the subprocess, redirect the standard I/O to respective pipes
    uaeproc = UaeProcess(
        await asyncio.create_subprocess_exec(
            emulator_exe, *args.params,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE),
        log_file)

    gdbserver = None

    async def GdbClient(reader, writer):
        try:
            await GdbStub(GdbConnection(reader, writer), uaeproc).run()
        except GdbDisconnect:
            logging.info('gdb session finished (client disconnected)')
            uaeproc.terminate()
        except asyncio.CancelledError:
            raise
        except Exception:
            traceback.print_exc()
            uaeproc.terminate()
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except (ConnectionError, OSError, BrokenPipeError):
                pass

    async def GdbListen():
        await uaeproc.prologue()
        uaeproc.break_opcode('{:04x}'.format(BREAK))
        print('Listening for gdb connection at localhost:2345')
        gdbserver = await asyncio.start_server(
            GdbClient, host='127.0.0.1', port=2345)

    # Terminate FS-UAE when connection with terminal is broken (Unix).
    if sys.platform != 'win32':
        loop.add_signal_handler(signal.SIGHUP, uaeproc.terminate)

    logger_task = asyncio.ensure_future(uaeproc.logger())

    if args.gdbserver:
        await GdbListen()
    else:
        prompt_task = asyncio.ensure_future(UaeDebugger(uaeproc))

    await uaeproc.wait()

    if gdbserver:
        gdbserver.close()


if __name__ == '__main__':
    # XXX: change from INFO to DEBUG to display messages exchanged between
    # fs-uae and code in `debug/uae.py`, gdb and code in `debug/gdb.py`
    logging.basicConfig(level=logging.INFO,
                        format='%(levelname)s: %(message)s')
    # logging.getLogger('asyncio').setLevel(logging.DEBUG)

    if sys.platform == 'win32':
        loop = asyncio.ProactorEventLoop()
    else:
        loop = asyncio.SelectorEventLoop()
    asyncio.set_event_loop(loop)
    # loop.set_debug(True)

    parser = argparse.ArgumentParser(
        description='Run FS-UAE with enabled console debugger.')
    parser.add_argument('-e', '--emulator', type=str, default='fs-uae',
                        help='Path to FS-UAE emulator binary.')
    parser.add_argument('-g', '--gdbserver', action='store_true',
                        help='Configure and run gdbserver on localhost:2345')
    parser.add_argument('-l', '--log', type=str, default=None,
                        help='Log stdout to a given file.')
    parser.add_argument('params', nargs='*', type=str,
                        help='Parameters passed to FS-UAE emulator.')
    args = parser.parse_args()

    uae = UaeLaunch(loop, args)
    loop.run_until_complete(uae)
    loop.close()
