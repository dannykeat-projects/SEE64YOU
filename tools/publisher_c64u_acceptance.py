"""Bounded shipping Publisher checks against the configured independent server.

The caller owns local backups, staging and machine restoration. Credentials stay
in memory; host cleanup uses the standard signed delete operation only for this
run's previously absent disposable names. Mutating requests are never retried.
"""
import datetime
import hashlib
import hmac
import re
import secrets
import urllib.error
import urllib.parse
import urllib.request


FIXTURE_NAMES = {"P8PAGE.8tml", "P8PAGE2.8tml", "P8STYLE.8ss", "P8STYLE2.8ss", "P8NOTES.txt", "P8NOTES2.txt", "P8LIMIT.txt"}


def run(*, folder, docs_dir, fetch, put, keys, native, wait, launch,
        exitcheck, mark, screen, state, save):
    fields = dict(line.split(":", 1) for line in fetch(folder + "/8TML.KEY").decode("ascii").splitlines()
                  if ":" in line)
    fields = {name.strip(): value.strip() for name, value in fields.items()}
    identity, secret = fields["id"], bytes.fromhex(fields["key"])
    assert len(secret) == 32, "Dedicated publishing key required"
    parsed = urllib.parse.urlsplit(fields["url"])
    assert parsed.hostname == "t8wn.com" and not parsed.username and not parsed.password, \
        "This acceptance procedure is bounded to the independently verified server"
    assert not parsed.query and not parsed.fragment and parsed.port in (None, 80, 443)
    endpoint = urllib.parse.urlunsplit(("https", parsed.hostname, parsed.path, "", ""))
    config = fetch(folder + "/SEE64YOU.CFG")
    # These are existing native record values, not replacement configuration.
    assert all(native(value) + b"\0" in config for value in
               (fields["url"], identity, fields["key"])), "Settings and dedicated key differ"
    assert len(config) == 334 and config[4] == 2 and config[7] == 3 and config[8] == 1, \
        "Existing version-2 settings with explicit HTTP consent required"
    prefix = "c64-check-" + datetime.datetime.now(datetime.timezone.utc).strftime("%m%d%H%M%S") + "-" + secrets.token_hex(3)
    receipt = {"endpoint": endpoint, "prefix": prefix, "tests": [], "cleanup": []}
    state["publisher_acceptance"] = receipt
    owned = []

    def values(raw):
        return dict(line.split(": ", 1) for line in raw.splitlines() if ": " in line)

    def request(params, body=None):
        req = urllib.request.Request(endpoint + "?" + urllib.parse.urlencode(params), data=body)
        try:
            with urllib.request.urlopen(req, timeout=40) as response:
                return response.read().decode("ascii")
        except urllib.error.HTTPError as error:
            return error.read().decode("ascii")

    def signed(operation, name=""):
        challenge = values(request({"op": "challenge", "id": identity}, b""))["challenge"]
        content = "\n".join(("8tml-publish/1", operation, identity, challenge, name, "0", "")).encode("ascii")
        signature = hmac.new(secret, content, hashlib.sha256).hexdigest().encode("ascii")
        params = {"op": operation, "id": identity, "size": 0, "c": challenge}
        if name:
            params["name"] = name
        return request(params, signature)

    def listing():
        raw = signed("list")
        assert values(raw).get("status") == "ok", "Authenticated fixture inventory failed"
        return {line.split(" ", 2)[2] for line in raw.splitlines() if line.startswith("file: ")}

    def open_source(name, label):
        keys(b"1")
        wait("[7] TYPE NAME", label + "-local-picker", timeout=180)
        keys(b"7")
        wait("OPEN SOURCE:", label + "-local-prompt")
        keys(native(name) + b"\r")
        loaded = wait("Local source loaded", label + "-source-loaded", timeout=360)
        assert name.upper() in loaded

    def target(name, label):
        keys(b"2")
        wait("BROWSE REMOTE", label + "-remote-list", timeout=180)
        keys(b"7")
        wait("REMOTE NAME:", label + "-remote-prompt")
        keys(native(name) + b"\r")
        wait("Remote name selected", label + "-target-set")

    def confirm(name, local, replacing, label):
        keys(b"3")
        text = wait("[1] CONFIRM  [0] CANCEL", label + "-confirmation", timeout=180)
        flat = "".join(line.strip() for line in text.splitlines())
        assert name.upper() in flat and local.upper() in flat
        assert ("REPLACE EXISTING FILE?" if replacing else "PUBLISH PUBLIC FILE?") in text

    def readback(url, expected):
        parsed_url = urllib.parse.urlsplit(url)
        assert parsed_url.hostname == parsed.hostname and not parsed_url.username and not parsed_url.password
        with urllib.request.urlopen(url, timeout=40) as response:
            assert response.read() == expected, "Published raw source differs"

    def upload(name, body, label):
        keys(b"1")
        text = wait("PRESS KEY", label + "-published", timeout=240)
        assert "PUBLISHED" in text
        url = "".join(line.strip() for line in text.split("PUBLISHED", 1)[1].split("PRESS KEY", 1)[0].splitlines()).lower()
        assert re.fullmatch(r"https?://[^\s]+/" + re.escape(name), url), "Unexpected public source URL"
        readback(url, body)
        assert name in listing(), "Uploaded source missing from authenticated list"
        keys(b" ")
        wait("Published; URL shown", label + "-published-status")
        return url

    save()
    try:
        baseline = listing()
        status = values(request({"op": "status"}))
        assert status.get("status") == "ok" and status.get("api") == "1"
        receipt["server_max_size"] = int(status["max-size"])
        save()
        fixtures = (("8tml", "P8PAGE.8tml", "P8PAGE2.8tml",
                     b"---\n8tml: 1\ntitle: C64 Publisher test\n---\n" + b"Publisher body line.\n" * 8),
                    ("8ss", "P8STYLE.8ss", "P8STYLE2.8ss", b"# C64U stylesheet\ntext: lightgrey\nbg: black\n"),
                    ("txt", "P8NOTES.txt", "P8NOTES2.txt", b"Plain text from the C64U Publisher.\n"))
        for extension, initial_name, replacement_name, source in fixtures:
            put(docs_dir + "/" + initial_name, source)
            put(docs_dir + "/" + replacement_name, source + b"# Replacement source\n")
            assert fetch(docs_dir + "/" + initial_name) == source, "Local fixture readback differs"
            assert fetch(docs_dir + "/" + replacement_name) == source + b"# Replacement source\n"
        launch("S64PUB.PRG", "[0] EXIT TO BASIC")
        keys(b"4")
        wait("Endpoint ready", "publisher-network-status", timeout=180)
        mark("shipping-publisher-independent-server-status")
        for extension, initial_name, replacement_name, source in fixtures:
            name = prefix + "." + extension
            assert name not in baseline
            owned.append(name)
            test = {"name": name, "source_sha256": hashlib.sha256(source).hexdigest()}
            receipt["tests"].append(test)
            save()
            open_source(initial_name, extension)
            target(name, extension)
            confirm(name, initial_name, False, extension + "-create-cancel")
            keys(b"0")
            wait("Publish cancelled", extension + "-create-cancelled")
            assert name not in listing()
            test["cancel_create_absent"] = True
            save()
            confirm(name, initial_name, False, extension + "-create")
            url = upload(name, source, extension + "-create")
            test["create_readback_exact"] = True
            test["url"] = url
            save()
            replacement = source + b"# Replacement source\n"
            open_source(replacement_name, extension + "-replace")
            target(name, extension + "-replace")
            confirm(name, replacement_name, True, extension + "-replace-cancel")
            keys(b"0")
            wait("Publish cancelled", extension + "-replace-cancelled")
            readback(url, source)
            test["cancel_replace_preserved"] = True
            save()
            confirm(name, replacement_name, True, extension + "-replace")
            upload(name, replacement, extension + "-replace")
            test["replace_readback_exact"] = True
            test["replacement_sha256"] = hashlib.sha256(replacement).hexdigest()
            save()
            mark("shipping-publisher-" + extension + "-list-create-replace-cancel-exact-independent-readback")
        maximum = receipt["server_max_size"]
        if 0 < maximum < 61440:
            name = prefix + "-limit.txt"
            assert name not in baseline
            owned.append(name)
            put(docs_dir + "/P8LIMIT.txt", b"x" * (maximum + 1))
            open_source("P8LIMIT.txt", "size-limit")
            target(name, "size-limit")
            keys(b"3")
            wait("Source exceeds server limit", "publisher-server-size-limit", timeout=180)
            assert name not in listing()
            receipt["size_limit_rejected_without_write"] = True
            save()
            mark("shipping-publisher-server-size-limit-without-write")
        keys(b"0")
        exitcheck("publisher-independent-server-exit")
    finally:
        # A failed/uncertain PUT is inspected, never replayed. Cleanup is itself
        # a single signed operation and any failure remains visible in receipt.
        try:
            remaining = listing()
            for name in owned:
                if name in remaining:
                    assert values(signed("delete", name)).get("status") == "ok", "Fixture cleanup failed"
                    receipt["cleanup"].append(name)
                    save()
            assert not set(owned) & listing(), "Remote fixtures remain"
            receipt["remote_fixtures_removed"] = True
            save()
        except Exception as error:
            receipt["cleanup_error"] = type(error).__name__
            save()
            raise
