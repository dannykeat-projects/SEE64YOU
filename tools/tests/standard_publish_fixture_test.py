#!/usr/bin/env python3
"""Independent stdlib client checks for the disposable fixture itself."""
import hashlib
import hmac
import http.client
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
import unittest

SPEC = importlib.util.spec_from_file_location(
    "fixture", Path(__file__).resolve().parents[1] / "standard_publish_fixture.py")
fixture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fixture)
SOURCE = b"---\n8tml: 1\n---\n# Hello\n"


class FixtureTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.receipt = Path(self.temp.name) / "receipt.json"
        self.server = fixture.ThreadingHTTPServer(("127.0.0.1", 0), fixture.Handler)
        self.port = self.server.server_port
        self.server.state = fixture.State(f"http://127.0.0.1:{self.port}", self.receipt)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()
        self.temp.cleanup()

    def request(self, method, path, body=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=5)
        connection.request(method, path, body)
        response = connection.getresponse()
        result = response.status, response.read()
        connection.close()
        return result

    def challenge(self):
        status, body = self.request("POST", "/publish?op=challenge&id=test-1", b"")
        self.assertEqual(status, 200)
        return dict(line.split(": ", 1) for line in body.decode().splitlines())["challenge"]

    def signed(self, op, source=b"", name="", challenge=None, signature=None):
        challenge = challenge or self.challenge()
        prefix = f"8tml-publish/1\n{op}\ntest-1\n{challenge}\n{name}\n{len(source)}\n".encode()
        signature = signature or hmac.new(bytes(range(32)), prefix + source, hashlib.sha256).hexdigest().encode()
        path = f"/publish?op={op}&id=test-1&size={len(source)}&c={challenge}"
        if name:
            path += "&name=" + name
        return self.request("POST", path, source + signature)

    def test_workflow_and_replay(self):
        status, body = self.request("GET", "/publish?op=status")
        self.assertEqual(status, 200)
        self.assertIn(b"auth: hmac\nmax-size: 61440\n", body)
        self.assertEqual(self.signed("list"), (200, b"status: ok\n"))
        challenge = self.challenge()
        status, body = self.signed("put", SOURCE, "index.8tml", challenge)
        self.assertEqual(status, 200)
        self.assertIn(b"name: index.8tml\nsize: 24\n", body)
        self.assertEqual(self.signed("put", SOURCE, "index.8tml", challenge)[0], 403)
        self.assertIn(b"file: 24 index.8tml\n", self.signed("list")[1])
        self.assertEqual(self.request("GET", "/files/index.8tml"), (200, SOURCE))
        replaced = SOURCE.replace(b"Hello", b"Again")
        self.assertEqual(self.signed("put", replaced, "index.8tml")[0], 200)
        self.assertEqual(self.request("GET", "/files/index.8tml"), (200, replaced))
        receipt = json.loads(self.receipt.read_text())
        self.assertEqual(receipt["successful_puts"], 2)
        self.assertEqual(receipt["files"]["index.8tml"]["sha256"], hashlib.sha256(replaced).hexdigest())
        puts = [event for event in receipt["events"] if event.get("op") == "put" and "replaced" in event]
        self.assertEqual([event["replaced"] for event in puts], [False, True])
        self.assertNotIn(bytes(range(32)).hex(), self.receipt.read_text())

    def test_failures_preserve_files_and_consume_challenges(self):
        self.assertEqual(self.signed("put", SOURCE, "index.8tml")[0], 200)
        challenge = self.challenge()
        self.assertEqual(self.signed("put", b"wrong", "index.8tml", challenge, b"0" * 64)[0], 403)
        self.assertEqual(self.signed("put", SOURCE, "index.8tml", challenge)[0], 403)
        self.assertEqual(self.signed("put", SOURCE, "../index.8tml")[0], 400)
        self.assertEqual(self.signed("put", SOURCE, "fixture-denied.8tml")[0], 403)
        self.assertEqual(self.request("GET", "/files/index.8tml"), (200, SOURCE))
        self.assertEqual(self.server.state.puts, 1)
        self.assertNotIn("fixture-denied.8tml", self.server.state.files)
        self.assertEqual(self.request("GET", "/publish?op=status&op=put")[0], 400)


if __name__ == "__main__":
    unittest.main()
