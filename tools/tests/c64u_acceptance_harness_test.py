"""Exercise acceptance-harness retry and directory ownership without hardware."""
import ast
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import Mock
import urllib.error
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[2]


def extract(name, scope):
    tree = ast.parse((ROOT / "tools/test_8tml_c64u.py").read_text())
    main = next(node for node in tree.body
                if isinstance(node, ast.FunctionDef) and node.name == "main")
    function = next(node for node in main.body
                    if isinstance(node, ast.FunctionDef) and node.name == name)
    exec(compile(ast.Module(body=[function], type_ignores=[]),
                 "<acceptance-harness>", "exec"), scope)
    return scope[name]


class Reply:
    def __enter__(self): return self
    def __exit__(self, *unused): return False
    def read(self): return b"response"


class ApiTests(unittest.TestCase):
    def make_api(self, outcomes):
        opener = Mock(side_effect=outcomes)
        sleep, save = Mock(), Mock()
        state = {}
        scope = dict(
            args=SimpleNamespace(host="192.0.2.1"), state=state,
            save=save, time=SimpleNamespace(sleep=sleep),
            urllib=SimpleNamespace(
                error=urllib.error, parse=urllib.parse,
                request=SimpleNamespace(Request=urllib.request.Request,
                                        urlopen=opener)))
        return extract("api", scope), opener, sleep, save, state

    def test_get_transient_failures_recover(self):
        api, opener, sleep, save, state = self.make_api([
            TimeoutError("busy"), urllib.error.URLError("temporary"), Reply()])
        self.assertEqual(api("machine:readmem", address="0400", length=1000),
                         b"response")
        self.assertEqual(opener.call_count, 3)
        self.assertEqual(sleep.call_count, 2)
        self.assertEqual(save.call_count, 2)
        self.assertEqual(len(state["api_read_retries"]), 2)
        for call in opener.call_args_list:
            request = call.args[0]
            self.assertEqual(request.get_method(), "GET")
            self.assertEqual(request.full_url,
                             "http://192.0.2.1/v1/machine:readmem?address=0400&length=1000")
            self.assertEqual(call.kwargs, {"timeout": 12})
        for call in sleep.call_args_list:
            self.assertEqual(call.args, (.5,))

    def test_get_exhaustion_is_bounded_to_three_attempts(self):
        for error in (TimeoutError("busy"), urllib.error.URLError("offline")):
            with self.subTest(error=type(error).__name__):
                api, opener, sleep, save, state = self.make_api([error] * 3)
                with self.assertRaises(type(error)) as raised:
                    api("info")
                self.assertIs(raised.exception, error)
                self.assertEqual(opener.call_count, 3)
                self.assertEqual(sleep.call_count, 2)
                self.assertEqual(save.call_count, 2)
                self.assertEqual(len(state["api_read_retries"]), 2)

    def test_put_is_never_replayed(self):
        for error in (TimeoutError("uncertain write"),
                      urllib.error.URLError("uncertain write")):
            with self.subTest(error=type(error).__name__):
                api, opener, sleep, save, state = self.make_api([error, Reply()])
                with self.assertRaises(type(error)) as raised:
                    api("machine:writemem", "PUT", address="0277", data="59")
                self.assertIs(raised.exception, error)
                self.assertEqual(opener.call_count, 1)
                self.assertEqual(opener.call_args.args[0].get_method(), "PUT")
                sleep.assert_not_called()
                save.assert_not_called()
                self.assertNotIn("api_read_retries", state)

    def test_http_error_is_not_retried(self):
        error = urllib.error.HTTPError("http://192.0.2.1/v1/info", 503,
                                       "Unavailable", {}, None)
        api, opener, sleep, save, state = self.make_api([error, Reply()])
        with self.assertRaises(urllib.error.HTTPError) as raised:
            api("info")
        self.assertIs(raised.exception, error)
        self.assertEqual(opener.call_count, 1)
        sleep.assert_not_called()
        save.assert_not_called()
        self.assertNotIn("api_read_retries", state)


class DocsOwnershipTests(unittest.TestCase):
    def make_ensure(self, existed, names):
        folder = "/sd/SEE64YOU"
        entries = list(names)
        def mkdir(path):
            self.assertEqual(path, folder + "/docs")
            entries.append(path)
        ftp = SimpleNamespace(nlst=Mock(return_value=entries),
                              mkd=Mock(side_effect=mkdir))
        state, save = {"created_docs": False}, Mock()
        ensure = extract("ensure_docs", dict(
            folder=folder, docs_dir=folder + "/docs", ftp=ftp,
            docs_existed=existed, state=state, save=save))
        return ensure, ftp, state, save

    def test_absent_docs_created_once_and_owned_by_test(self):
        ensure, ftp, state, save = self.make_ensure(False, [])
        ensure()
        ensure()
        ftp.mkd.assert_called_once_with("/sd/SEE64YOU/docs")
        self.assertTrue(state["created_docs"])
        self.assertTrue(save.called)

    def test_existing_docs_preserves_user_ownership(self):
        ensure, ftp, state, save = self.make_ensure(
            True, ["/sd/SEE64YOU/DOCS/"])
        ensure()
        ftp.mkd.assert_not_called()
        self.assertFalse(state["created_docs"])
        save.assert_not_called()

    def test_editor_created_docs_remains_test_owned(self):
        ensure, ftp, state, save = self.make_ensure(
            False, ["/sd/SEE64YOU/docs"])
        ensure()
        ftp.mkd.assert_not_called()
        self.assertTrue(state["created_docs"])
        save.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()
