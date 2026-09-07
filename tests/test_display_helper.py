import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch, Mock

spec = importlib.util.spec_from_file_location('display', Path(__file__).parents[1] / 'src/helpers/sunshine-display.py')
display = importlib.util.module_from_spec(spec)
spec.loader.exec_module(display)

class DisplayTest(unittest.TestCase):
    def test_binary_journal_message(self):
        self.assertEqual(display.journal_message(json.dumps({'MESSAGE': [255, 0, 10]})), '\ufffd\0\n')
        self.assertEqual(display.journal_message('{}'), '')

    def test_watch_keeps_active_session_after_binary_message(self):
        journal = Mock()
        journal.poll.return_value = None
        journal.stdout.fileno.return_value = 123
        records = [
            {'MESSAGE': [255, 0, 10]},
            {'MESSAGE': 'New streaming session started [active sessions: 1]'},
        ]
        data = b''.join(json.dumps(x).encode() + b'\n' for x in records)
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(display, 'STATE', Path(tmp) / 'desktop.json'), \
             patch.object(display, 'settings', return_value=('Virtual-Test', 'test.service', 'sunshine.service')), \
             patch.object(display, 'run', return_value='a' * 32), \
             patch.object(display.subprocess, 'Popen', return_value=journal), \
             patch.object(display.signal, 'signal'), \
             patch.object(display.os, 'read', return_value=data), \
             patch.object(display.select, 'select', side_effect=[([journal.stdout], [], []), ([], [], []), KeyboardInterrupt]), \
             patch.object(display, 'switch') as switch:
            display.STATE.write_text('[]')
            with patch.object(display.time, 'time', return_value=display.STATE.stat().st_mtime + 60):
                with self.assertRaises(KeyboardInterrupt):
                    display.watch()
            # A stale prep timestamp must not restore displays during the stream.
            self.assertEqual([c.args[0] for c in switch.call_args_list], [True, False])
            journal.communicate.assert_called_once_with(timeout=5)

    def test_switch_and_restore(self):
        physical = {'name': 'DP-1', 'connected': True, 'enabled': True, 'pos': {'x': 50, 'y': 0},
                    'scale': 1.25, 'priority': 1, 'modes': [{'id': '1', 'name': '2560x1440@60'}], 'currentModeId': '1'}
        virtual = {'name': 'Virtual-Test', 'connected': True, 'enabled': False}
        current = [physical, virtual]
        commands = []
        def apply(args):
            commands.extend(args)
            for arg in args:
                for output in current:
                    if arg == f"output.{output['name']}.enable": output['enabled'] = True
                    if arg == f"output.{output['name']}.disable": output['enabled'] = False
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(display, 'ROOT', Path(tmp)), \
             patch.object(display, 'STATE', Path(tmp) / 'desktop.json'), \
             patch.object(display, 'settings', return_value=('Virtual-Test', 'test.service', 'sunshine.service')), \
             patch.object(display, 'outputs', side_effect=lambda: current), \
             patch.object(display, 'run') as run, \
             patch.object(display, 'apply', side_effect=apply):
            display.switch(True)
            self.assertFalse(physical['enabled'])
            self.assertTrue(virtual['enabled'])
            saved = display.STATE.read_text()
            display.switch(True)
            self.assertEqual(display.STATE.read_text(), saved)
            run.assert_called_with('systemctl', '--user', 'start', 'test.service')
            display.switch(False)
            self.assertTrue(physical['enabled'])
            self.assertFalse(virtual['enabled'])
            self.assertIn('output.DP-1.position.50,0', commands)
            self.assertIn('output.DP-1.scale.1.25', commands)
            self.assertFalse(display.STATE.exists())
            count = len(commands)
            display.switch(False)
            self.assertEqual(len(commands), count)

    def test_watch_restores_on_disconnect_without_app_exit(self):
        journal = Mock()
        journal.poll.return_value = None
        journal.stdout.fileno.return_value = 123
        messages = [
            'New streaming session started [active sessions: 1]',
            'Session ended',
            'New streaming session started [active sessions: 1]',
        ]
        with tempfile.TemporaryDirectory() as tmp, \
             patch.object(display, 'STATE', Path(tmp) / 'desktop.json'), \
             patch.object(display, 'settings', return_value=('Virtual-Test', 'test.service', 'sunshine.service')), \
             patch.object(display, 'run', return_value='a' * 32), \
             patch.object(display.subprocess, 'Popen', return_value=journal), \
             patch.object(display.signal, 'signal'), \
             patch.object(display.os, 'read', side_effect=[json.dumps({'MESSAGE': m}).encode() + b'\n' for m in messages]), \
             patch.object(display.select, 'select', side_effect=[([journal.stdout], [], [])] * 3 + [KeyboardInterrupt]), \
             patch.object(display, 'switch') as switch:
            with self.assertRaises(KeyboardInterrupt):
                display.watch()
            self.assertEqual([c.args[0] for c in switch.call_args_list], [True, False, True, False])

if __name__ == '__main__':
    unittest.main()
