#!/usr/bin/env python3
"""Disposable HTTP/HMAC test for exact 8TML source publication and retrieval."""
import hashlib
import hmac
import json
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

SITE = Path(__file__).resolve().parents[1]
SOURCE = ("---\n8TmL: 1\ntitle: Source title\nX-Future:  Keep : CASE  \n"
          "# preserve this comment\n---\n# Hello **world**\n"
          "<script>alert(1)</script>\n:::future data\nopaque\ttext\n:::\n<?php not executable\n")


def main():
    with tempfile.TemporaryDirectory(prefix="s64-8tml-http-") as directory:
        root = Path(directory) / "site"
        shutil.copytree(SITE, root, ignore=shutil.ignore_patterns(
            "config.php", "ENABLE_SETUP", "data", "pages", "posts", "tests"))
        key = secrets.token_bytes(32)
        (root / "config.php").write_text("<?php return " +
            "['password_hash'=>'unused-test-hash','token_secret'=>'" + secrets.token_hex(32) +
            "','direct_secret'=>'" + key.hex() + "'];\n")
        with socket.socket() as sock:
            sock.bind(("127.0.0.1", 0))
            port = sock.getsockname()[1]
        base = f"http://127.0.0.1:{port}"

        def request(uri, method="GET", body=b"", headers=None):
            request_headers = {"Content-Type": "application/json", **(headers or {})}
            req = urllib.request.Request(base + uri, data=body if method == "POST" else None,
                                         method=method, headers=request_headers)
            try:
                response = urllib.request.urlopen(req, timeout=10)
            except urllib.error.HTTPError as error:
                response = error
            with response:
                return response.status, response.read(), response.headers

        def signed(uri, method="GET", body=b""):
            status, payload, _ = request("/api.php?action=challenge")
            assert status == 200
            challenge = json.loads(payload)["challenge"]
            canonical = "\n".join(("see64you-direct/1", challenge, method, uri,
                                    hashlib.sha256(body).hexdigest())).encode("ascii")
            return {"X-See64You-Challenge": challenge,
                    "X-See64You-Signature": hmac.new(key, canonical, hashlib.sha256).hexdigest()}

        def authenticated(uri, method="GET", body=b""):
            return request(uri, method, body, signed(uri, method, body))

        with (Path(directory) / "server.log").open("w") as log:
            server = subprocess.Popen(["php", "-S", f"127.0.0.1:{port}", "-t", str(root)],
                                      stdout=log, stderr=log)
            try:
                for attempt in range(50):
                    try:
                        if request("/api.php?action=status")[0] == 200:
                            break
                    except urllib.error.URLError:
                        pass
                    time.sleep(.1)
                else:
                    raise AssertionError("Disposable PHP server did not start")

                save_uri = "/api.php?action=save&ack=1"
                get_uri = "/api.php?action=get&type=page&slug=eighttml-http&format=8tml"
                document = {"type": "page", "slug": "eighttml-http", "status": "published",
                            "date": "2026-10-21T12:00:00+00:00", "order": 7, "raw8tml": SOURCE}
                body = json.dumps(document, separators=(",", ":")).encode("ascii")
                assert request(save_uri, "POST", body)[0] == 401, "Unauthenticated save"
                code, too_large, _ = request(save_uri, "POST", b"x" * (131072 + 1))
                assert code == 413 and json.loads(too_large)["error"] == "request_too_large"
                # The maximum source is still allowed after JSON escapes double its size.
                source_prefix = "---\n8tml: 1\n---\n"
                source_full = source_prefix + "\\" * (61440 - len(source_prefix))
                full_doc = dict(document, slug="escaped-full", raw8tml=source_full)
                full_body = json.dumps(full_doc, separators=(",", ":")).encode("ascii")
                assert len(full_body) < 131072
                assert authenticated(save_uri, "POST", full_body)[0] == 200
                code, public_full, _ = request('/source.php?type=page&slug=escaped-full')
                assert code == 200 and public_full == source_full.encode('ascii')
                remove_full = json.dumps({"type": "page", "slug": "escaped-full"}).encode("ascii")
                assert authenticated("/api.php?action=delete", "POST", remove_full)[0] == 200
                headers = signed(save_uri, "POST", body)
                assert request(save_uri, "POST", body + b" ", headers)[0] == 401, "Changed body accepted"
                # A signature mismatch does not consume its challenge; exact signed bytes still work.
                code, reply, save_headers = request(save_uri, "POST", body, headers)
                assert code == 200 and json.loads(reply)["ok"], reply
                assert request(save_uri, "POST", body, headers)[0] == 401, "Replay accepted"
                assert request(get_uri)[0] == 401, "Unauthenticated source retrieval"
                assert authenticated(get_uri, "POST")[0] == 405, "Source retrieval requires GET"
                code, source, response_headers = authenticated(get_uri)
                assert code == 200 and source == SOURCE.encode("ascii"), (code, source)
                assert response_headers.get_content_type() == "text/8tml"
                assert response_headers.get_content_charset() == "us-ascii"
                assert response_headers["Content-Disposition"] == 'attachment; filename="eighttml-http.8tml"'
                assert response_headers["X-See64You-Status"] == "published"
                revision = response_headers["X-See64You-Revision"]
                assert save_headers['X-See64You-Revision'] == revision
                assert len(revision) == 32 and all(c in '0123456789abcdef' for c in revision)
                code, head_body, head_headers = authenticated(get_uri, "HEAD")
                assert code == 200 and head_body == b''
                assert head_headers['X-See64You-Revision'] == revision
                assert head_headers['X-See64You-Status'] == 'published'
                assert request(get_uri, "HEAD")[0] == 401
                assert "no-store" in response_headers["Cache-Control"]
                public_uri = '/source.php?type=page&slug=eighttml-http'
                code, public_source, public_headers = request(public_uri)
                assert code == 200 and public_source == SOURCE.encode('ascii')
                assert public_headers.get_content_type() == 'text/8tml'
                assert public_headers.get_content_charset() == 'us-ascii'
                assert int(public_headers['Content-Length']) == len(public_source)
                assert public_headers['X-Content-Type-Options'] == 'nosniff'
                assert 'no-store' in public_headers['Cache-Control']
                assert request(public_uri, 'HEAD')[:2] == (200, b'')
                assert request(public_uri, 'POST', b'ignored')[0] == 405
                assert request(public_uri, 'OPTIONS')[0] == 405
                for invalid_uri in ('/source.php?type=other&slug=eighttml-http',
                                    '/source.php?type=page&slug=../outside',
                                    '/source.php?type[]=page&slug=eighttml-http',
                                    '/source.php?type=page&slug[]=eighttml-http',
                                    '/source.php?type=page&slug=missing'):
                    assert request(invalid_uri)[0] == 404


                code, payload, _ = authenticated(get_uri.replace("&format=8tml", ""))
                metadata = json.loads(payload)["document"]
                assert code == 200 and metadata["raw8tml"] == SOURCE
                assert metadata["title"] == "Source title" and metadata["order"] == 7
                update_uri = save_uri + "&intent=update&identity=eighttml-http"
                assert metadata['revision'] == revision
                prior = (root / 'pages/eighttml-http.8tml.php').read_bytes()
                for suffix in ('', '&revision=bad', '&revision[]=bad'):
                    code, payload, _ = authenticated(update_uri + suffix, 'POST', body)
                    assert code == 400 and json.loads(payload)['error'] == 'revision_required'
                    assert (root / 'pages/eighttml-http.8tml.php').read_bytes() == prior
                def update(payload, token):
                    return authenticated(update_uri + '&revision=' + token, 'POST', payload)
                signed_uri = update_uri + '&revision=' + revision
                tampered_uri = update_uri + '&revision=' + ('0' if revision[0] != '0' else '1') + revision[1:]
                signed_headers = signed(signed_uri, 'POST', body)
                assert request(tampered_uri, 'POST', body, signed_headers)[0] == 401
                assert authenticated(save_uri, "POST", body)[0] == 400, "Create overwrote existing slug"
                for suffix in ('&intent=update', '&intent=update&identity=wrong', '&intent=upsert', '&intent[]=update'):
                    assert authenticated(save_uri + suffix, 'POST', body)[0] == 400
                native = {field: document[field] for field in ("type", "slug", "status", "raw8tml")}
                code, _, update_headers = update(json.dumps(native).encode("ascii"), revision)
                assert code == 200 and update_headers['X-See64You-Revision'] != revision
                winner_bytes = (root / 'pages/eighttml-http.8tml.php').read_bytes()
                code, conflict, _ = update(body, revision)
                assert code == 409 and json.loads(conflict)['error'] == 'revision_conflict'
                assert (root / 'pages/eighttml-http.8tml.php').read_bytes() == winner_bytes
                code, payload, _ = authenticated(get_uri.replace("&format=8tml", ""))
                preserved = json.loads(payload)["document"]
                assert code == 200 and preserved["date"] == document["date"] and preserved["order"] == 7
                explicit = dict(native, date="2003-04-05", order=-19)
                assert update(json.dumps(explicit).encode("ascii"), preserved['revision'])[0] == 200
                code, payload, _ = authenticated(get_uri.replace("&format=8tml", ""))
                updated = json.loads(payload)["document"]
                assert code == 200 and updated["date"] == "2003-04-05" and updated["order"] == -19
                assert update(body, updated['revision'])[0] == 200
                code, payload, _ = authenticated("/api.php?action=list&type=page")
                listed = json.loads(payload)["documents"]
                assert code == 200 and len(listed) == 1
                assert "raw8tml" not in listed[0] and "body" not in listed[0]
                # Filter the native list before parsing excluded old-format paths.
                fresh = dict(native, type="post", slug="native-defaults", status="draft")
                assert authenticated(save_uri, "POST", json.dumps(fresh).encode("ascii"))[0] == 200
                code, payload, _ = authenticated("/api.php?action=get&type=post&slug=native-defaults")
                created = json.loads(payload)["document"]
                assert request('/source.php?type=post&slug=native-defaults')[0] == 404
                assert code == 200 and created["date"] and created["order"] == 0 and created["raw8tml"] == SOURCE
                assert authenticated("/api.php?action=delete", "POST",
                    json.dumps({"type": "post", "slug": "native-defaults"}).encode("ascii"))[0] == 200
                for slug, date in (("earlier", "2000-01-01"), ("later", "2002-01-01")):
                    item = dict(native, type="post", slug=slug, date=date)
                    assert authenticated(save_uri, "POST", json.dumps(item).encode("ascii"))[0] == 200
                    assert request('/source.php?type=post&slug=' + slug)[1] == SOURCE.encode('ascii')
                (root / "posts" / "excluded.s64.php").write_text("not a valid source record")
                source_list = "/api.php?action=list&type=post&source=8tml"
                code, payload, _ = authenticated(source_list)
                filtered = json.loads(payload)
                assert code == 200 and filtered["count"] == 2
                assert [item["slug"] for item in filtered["documents"]] == ["later", "earlier"]
                code, payload, _ = authenticated(source_list + "&format=summary&offset=0")
                assert code == 200 and b"Count: 2\nSlug: later\n" in payload
                code, payload, _ = authenticated(source_list + "&format=summary&offset=1")
                assert code == 200 and b"Count: 2\nSlug: earlier\n" in payload
                for source_filter in ("unknown", "s64", "8tml&source[]=8tml"):
                    code, payload, _ = authenticated("/api.php?action=list&type=post&source=" + source_filter)
                    assert code == 400 and json.loads(payload)["error"] == "invalid_source"
                (root / "posts" / "excluded.s64.php").unlink()
                code, html, _ = request("/page.php?slug=eighttml-http")
                assert code == 200 and b"<h1>Hello <strong>world</strong></h1>" in html
                assert b"<script>alert(1)</script>" not in html and b"&lt;script&gt;" in html
                assert b"X-Future:" not in html and b"8TmL:" not in html
                assert request("/pages/eighttml-http.8tml.php")[0] == 404, "Private record served directly"
                assert authenticated(get_uri.replace("format=8tml", "format=s64"))[0] == 400

                # An invalid signed replacement must leave prior source and metadata intact.
                invalid = dict(document, raw8tml="---\n8tml: 1\n---\nbad\r\n")
                invalid_body = json.dumps(invalid).encode("ascii")
                current_revision = authenticated(get_uri, 'HEAD')[2]['X-See64You-Revision']
                assert update(invalid_body, current_revision)[0] == 400
                assert authenticated(get_uri)[1] == SOURCE.encode("ascii")
                assert request(public_uri)[1] == SOURCE.encode("ascii")
                for field in ("type", "slug", "status", "date"):
                    malformed = json.dumps(dict(document, **{field: []})).encode("ascii")
                    code, payload, _ = update(malformed, current_revision)
                    assert code == 400 and json.loads(payload)["error"] == "invalid_" + field
                    assert authenticated(get_uri)[1] == SOURCE.encode("ascii")
                changed = SOURCE.replace("opaque\ttext", "changed\ttext")
                replacement = dict(document, raw8tml=changed, status="draft")
                replacement_body = json.dumps(replacement).encode("ascii")
                assert update(replacement_body, current_revision)[0] == 200
                code, payload, draft_headers = authenticated(get_uri)
                assert code == 200 and payload == changed.encode("ascii")
                assert draft_headers["X-See64You-Status"] == "draft"
                assert request("/page.php?slug=eighttml-http")[0] == 404, "Draft leaked publicly"
                assert request(public_uri)[0] == 404, 'Unpublished source leaked'
                assert authenticated(public_uri)[0] == 404, 'Authentication exposed draft public source'
                assert request(public_uri + '&preview=anything')[0] == 404
                assert update(body, draft_headers['X-See64You-Revision'])[0] == 200
                assert request(public_uri)[1] == SOURCE.encode('ascii'), 'Republished source missing'

                # Invalid identity and wrong method must not create source records.
                traversal = json.dumps(dict(document, slug="../outside")).encode("ascii")
                assert authenticated(save_uri, "POST", traversal)[0] == 400
                assert authenticated(save_uri)[0] == 405
                delete_body = json.dumps({"type": "page", "slug": "eighttml-http"}).encode("ascii")
                assert authenticated("/api.php?action=delete", "POST", delete_body)[0] == 200
                assert authenticated(get_uri)[0] == 404
                assert request(public_uri)[0] == 404, 'Deleted source leaked'
                print("8TML HTTP/HMAC: exact source, metadata, replay/tamper refusal, HTML escaping, public source/privacy and delete passed")
            finally:
                server.terminate()
                server.wait(timeout=10)


if __name__ == "__main__":
    main()
