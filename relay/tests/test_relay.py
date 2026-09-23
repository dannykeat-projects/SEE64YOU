import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "relay"))

from see64you_relay import (  # noqa: E402
    RelayConfigError,
    build_upstream_url,
    client_allowed,
    load_config,
    relay_key_allowed,
    request_headers,
)


VALID_KEY = "0123456789abcdef" * 4


class HeaderBag:
    def __init__(self, values):
        self.values = values

    def get(self, name):
        return self.values.get(name)


def write_config(directory: Path, **overrides) -> Path:
    data = {
        "listen_host": "0.0.0.0",
        "listen_port": 6464,
        "upstream_base": "https://example.test/site/",
        "relay_key": VALID_KEY,
        "allowed_clients": ["192.168.1.64/32", "10.0.0.0/24"],
        "allowed_path_prefixes": ["/api.php", "/wp-json/see64you/"],
        "max_request_bytes": 65536,
        "max_response_bytes": 262144,
        "timeout_seconds": 20,
    }
    data.update(overrides)
    path = directory / "config.json"
    path.write_text(json.dumps(data), encoding="utf-8")
    return path


class RelayConfigTests(unittest.TestCase):
    def test_valid_config_and_client_networks(self):
        with tempfile.TemporaryDirectory() as temp:
            config = load_config(write_config(Path(temp)))

        self.assertEqual(config.upstream_netloc, "example.test")
        self.assertTrue(client_allowed(config, "192.168.1.64"))
        self.assertTrue(client_allowed(config, "10.0.0.25"))
        self.assertFalse(client_allowed(config, "192.168.1.65"))

    def test_upstream_must_use_https(self):
        with tempfile.TemporaryDirectory() as temp:
            path = write_config(Path(temp), upstream_base="http://example.test/site/")
            with self.assertRaises(RelayConfigError):
                load_config(path)

    def test_upstream_rejects_unsafe_base_path(self):
        bad_values = [
            "https://example.test/site/../admin/",
            "https://example.test/site/%2e%2e/admin/",
            "https://example.test/site/%252e%252e/admin/",
            "https://example.test/site/%5cadmin/",
        ]
        for value in bad_values:
            with self.subTest(value=value), tempfile.TemporaryDirectory() as temp:
                path = write_config(Path(temp), upstream_base=value)
                with self.assertRaises(RelayConfigError):
                    load_config(path)

    def test_upstream_rejects_credentials_query_and_fragment(self):
        bad_values = [
            "https://user:pass@example.test/site/",
            "https://example.test/site/?target=other",
            "https://example.test/site/#fragment",
        ]
        for value in bad_values:
            with self.subTest(value=value), tempfile.TemporaryDirectory() as temp:
                path = write_config(Path(temp), upstream_base=value)
                with self.assertRaises(RelayConfigError):
                    load_config(path)

    def test_relay_key_is_exact_64_hex(self):
        bad_keys = ["short", "g" * 64, "a" * 65]
        for key in bad_keys:
            with self.subTest(key=key), tempfile.TemporaryDirectory() as temp:
                path = write_config(Path(temp), relay_key=key)
                with self.assertRaises(RelayConfigError):
                    load_config(path)

    def test_allowed_path_prefix_must_be_safe(self):
        with tempfile.TemporaryDirectory() as temp:
            path = write_config(Path(temp), allowed_path_prefixes=["/../secret"])
            with self.assertRaises(RelayConfigError):
                load_config(path)


class RelayPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.config = load_config(write_config(Path(self.temp.name)))

    def tearDown(self):
        self.temp.cleanup()

    def test_builds_fixed_origin_url_under_base_path(self):
        actual = build_upstream_url(
            self.config,
            "/api.php?action=get&type=post&slug=hello",
        )
        self.assertEqual(
            actual,
            "https://example.test/site/api.php?action=get&type=post&slug=hello",
        )

    def test_wordpress_prefix_is_allowed(self):
        actual = build_upstream_url(
            self.config,
            "/wp-json/see64you/v1/posts?status=draft",
        )
        self.assertEqual(
            actual,
            "https://example.test/site/wp-json/see64you/v1/posts?status=draft",
        )

    def test_unconfigured_path_is_rejected(self):
        with self.assertRaises(RelayConfigError):
            build_upstream_url(self.config, "/admin.php")

    def test_traversal_is_rejected_even_when_percent_encoded(self):
        for path in [
            "/wp-json/see64you/../users",
            "/wp-json/see64you/%2e%2e/users",
            "/wp-json/see64you/%252e%252e/users",
            "/wp-json/see64you/%5c..%5cusers",
        ]:
            with self.subTest(path=path):
                with self.assertRaises(RelayConfigError):
                    build_upstream_url(self.config, path)

    def test_key_check_accepts_only_matching_key(self):
        self.assertTrue(relay_key_allowed(self.config, VALID_KEY))
        self.assertTrue(relay_key_allowed(self.config, VALID_KEY.upper()))
        self.assertFalse(relay_key_allowed(self.config, None))
        self.assertFalse(relay_key_allowed(self.config, "0" * 64))

    def test_request_headers_forward_only_safe_subset(self):
        forwarded = request_headers(
            HeaderBag(
                {
                    "Authorization": "Bearer token",
                    "Content-Type": "application/x-see64you",
                    "Accept": "application/json",
                    "X-See64You-Relay-Key": VALID_KEY,
                    "Host": "relay.local",
                    "Cookie": "secret=1",
                }
            )
        )

        self.assertEqual(forwarded["Authorization"], "Bearer token")
        self.assertEqual(forwarded["Content-Type"], "application/x-see64you")
        self.assertEqual(forwarded["Accept"], "application/json")
        self.assertEqual(forwarded["Accept-Encoding"], "identity")
        self.assertNotIn("X-See64You-Relay-Key", forwarded)
        self.assertNotIn("Host", forwarded)
        self.assertNotIn("Cookie", forwarded)


if __name__ == "__main__":
    unittest.main()
