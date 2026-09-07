#!/usr/bin/python3
"""KDE output switching for Sunshine, including disconnect without app exit."""
import fcntl
import json
import os
from pathlib import Path
import re
import select
import signal
import subprocess
import sys
import time

ROOT = Path(os.environ.get('XDG_STATE_HOME', Path.home() / '.local/state')) / 'sunshine-display'
STATE = ROOT / 'desktop.json'
CONFIG = Path(os.environ.get('XDG_CONFIG_HOME', Path.home() / '.config')) / 'virtmonitors-sunshine.json'

def settings():
    config = json.loads(CONFIG.read_text())
    return config['output'], config['monitor_service'], config['sunshine_service']

def run(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.PIPE, timeout=20)

def outputs():
    return json.loads(run('kscreen-doctor', '-j'))['outputs']

def apply(args):
    if args:
        run('kscreen-doctor', *args)

def journal_message(line):
    message = json.loads(line).get('MESSAGE', '')
    # journalctl JSON represents binary fields as arrays of bytes.
    if isinstance(message, list):
        message = bytes(message).decode('utf-8', errors='replace')
    return message if isinstance(message, str) else ''

def switch(active):
    VIRTUAL, monitor_service, _ = settings()
    ROOT.mkdir(parents=True, exist_ok=True, mode=0o700)
    with (ROOT / 'lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if active:
            if not STATE.exists():
                saved = [o for o in outputs() if o['name'] != VIRTUAL]
                tmp = STATE.with_suffix('.tmp')
                tmp.write_text(json.dumps(saved))
                tmp.replace(STATE)
            run('systemctl', '--user', 'start', monitor_service)
            for _ in range(40):
                current = outputs()
                if any(o['name'] == VIRTUAL for o in current):
                    break
                time.sleep(.25)
            else:
                raise RuntimeError('Virtual monitor did not appear')
            apply([f'output.{VIRTUAL}.enable', f'output.{VIRTUAL}.position.0,0',
                   f'output.{VIRTUAL}.priority.1'] +
                  [f"output.{o['name']}.disable" for o in current if o['name'] != VIRTUAL])
            if {o['name'] for o in outputs() if o['enabled']} != {VIRTUAL}:
                raise RuntimeError('Virtual-only layout was not applied')
        else:
            if not STATE.exists():
                return
            current = outputs()
            saved = json.loads(STATE.read_text()) if STATE.exists() else []
            available = {o['name']: o for o in current if o['connected']}
            args = []
            for o in saved:
                name = o['name']
                if name not in available:
                    continue
                prefix = f'output.{name}.'
                args.append(prefix + ('enable' if o['enabled'] else 'disable'))
                if o['enabled']:
                    mode = next((m['name'] for m in o['modes'] if m['id'] == o['currentModeId']), None)
                    if mode:
                        args.append(prefix + 'mode.' + mode)
                    args.extend([prefix + f"position.{o['pos']['x']},{o['pos']['y']}",
                                 prefix + f"scale.{o['scale']}", prefix + f"priority.{o['priority']}"])
            if not any(a.endswith('.enable') for a in args):
                args.extend(f'output.{name}.enable' for name in available if name != VIRTUAL)
            if not any(a.endswith('.enable') for a in args):
                raise RuntimeError('No physical display available; keeping virtual display')
            if VIRTUAL in available:
                args.append(f'output.{VIRTUAL}.disable')
            apply(args)
            if not any(o['enabled'] and o['name'] != VIRTUAL for o in outputs()):
                raise RuntimeError('Physical display restoration failed')
            STATE.unlink(missing_ok=True)
        print('Virtual display active' if active else 'Physical displays restored', flush=True)

def watch():
    _, _, UNIT = settings()
    # Replay only this Sunshine process invocation, then follow without a gap.
    invocation = run('systemctl', '--user', 'show', UNIT, '-p', 'InvocationID', '--value').strip()
    if not re.fullmatch(r'[0-9a-f]{32}', invocation):
        raise RuntimeError('Sunshine has no active service invocation')
    command = ['journalctl', '--user', '-f', '-o', 'json', '--no-pager', '-n', 'all',
               f'_SYSTEMD_INVOCATION_ID={invocation}']
    journal = subprocess.Popen(command, stdout=subprocess.PIPE)
    count = 0
    pending = None
    buffer = b''
    def stop(*_):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, stop)
    try:
        while journal.poll() is None:
            ready, _, _ = select.select([journal.stdout], [], [], .5)
            if ready:
                buffer += os.read(journal.stdout.fileno(), 65536)
                while b'\n' in buffer:
                    line, buffer = buffer.split(b'\n', 1)
                    message = journal_message(line)
                    match = re.search(r'New streaming session started \[active sessions: (\d+)\]', message)
                    if match:
                        count = int(match[1])
                        pending = True
                    elif 'Session ended' in message:
                        count = max(0, count - 1)
                        pending = count > 0
            if pending is not None:
                switch(pending)
                pending = None
            # Recover from a launch which ran the prep command but never streamed.
            if count == 0 and STATE.exists() and time.time() - STATE.stat().st_mtime > 30:
                switch(False)
        raise RuntimeError('Sunshine journal follower exited')
    finally:
        journal.terminate()
        try:
            journal.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            journal.kill()
            journal.communicate()
        switch(False)

if __name__ == '__main__':
    try:
        if sys.argv[1] == 'watch':
            watch()
        else:
            switch(sys.argv[1] == 'on')
    except KeyboardInterrupt:
        pass
    except Exception as exc:
        print(str(exc), file=sys.stderr, flush=True)
        if sys.argv[1] == 'on':
            switch(False)
        sys.exit(1)
