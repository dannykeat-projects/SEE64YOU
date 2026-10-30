"""Test bounded screen-read retries without contacting hardware."""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest
import urllib.error

ROOT = Path(__file__).resolve().parents[2]

class Clock:
    def __init__(self): self.now = 0
    def monotonic(self): return self.now
    def sleep(self, seconds): self.now += seconds

class WaitTests(unittest.TestCase):
    def make_wait(self, screen):
        tree = ast.parse((ROOT / 'tools/test_8tml_c64u.py').read_text())
        main = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
        function = next(n for n in main.body if isinstance(n, ast.FunctionDef) and n.name == 'wait')
        clock, state = Clock(), {}
        scope = dict(time=clock, state=state, screen=screen, save=lambda: None,
                     mem=lambda *args: b'\0', urllib=urllib,
                     args=SimpleNamespace(storage_only=True))
        exec(compile(ast.Module(body=[function], type_ignores=[]), '<wait>', 'exec'), scope)
        return scope['wait'], clock, state

    def test_transient_read_recovers(self):
        outcomes = iter([urllib.error.URLError(TimeoutError('busy')), 'PASSED'])
        def screen(label):
            value = next(outcomes)
            if isinstance(value, Exception): raise value
            return value
        wait, clock, state = self.make_wait(screen)
        self.assertEqual(wait('PASSED', 'probe', timeout=6), 'PASSED')
        self.assertEqual(clock.now, 2)
        self.assertEqual(len(state['monitor_read_errors']), 1)

    def test_repeated_timeouts_keep_deadline(self):
        def screen(label): raise urllib.error.URLError(TimeoutError('busy'))
        wait, clock, state = self.make_wait(screen)
        with self.assertRaises(AssertionError): wait('PASSED', 'probe', timeout=6)
        self.assertEqual(clock.now, 6)
        self.assertEqual(len(state['monitor_read_errors']), 3)

    def test_native_failure_is_not_retried(self):
        wait, clock, state = self.make_wait(lambda label: '8TML FAIL 44')
        with self.assertRaises(AssertionError): wait('PASSED', 'probe', timeout=6)
        self.assertEqual(clock.now, 0)
        self.assertNotIn('monitor_read_errors', state)

if __name__ == '__main__': unittest.main()
