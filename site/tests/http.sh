#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
TMP=$(mktemp -d)
PORT=$((18000 + ($$ % 10000)))
BASE="http://127.0.0.1:$PORT"
SITE="$TMP/site"
PASSWORD="see64you-http-test"

cleanup() {
    if [ "${SERVER_PID:-}" != "" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -rf "$TMP"
}
trap cleanup EXIT INT TERM

cp -R "$ROOT/site" "$SITE"
rm -f "$SITE/config.php"

php -r '
$path = $argv[1];
$password = $argv[2];
$config = [
    "site_title" => "HTTP Test",
    "theme" => "plain",
    "text_color" => "#a5a5ff",
    "background_color" => "#3e31a2",
    "link_color" => "#ffffff",
    "visited_color" => "#c46cff",
    "password_hash" => password_hash($password, PASSWORD_DEFAULT),
    "token_secret" => bin2hex(random_bytes(32)),
    "token_ttl" => 600,
];
file_put_contents($path, "<?php\ndeclare(strict_types=1);\nreturn " . var_export($config, true) . ";\n");
' "$SITE/config.php" "$PASSWORD"

php -S "127.0.0.1:$PORT" -t "$SITE" >"$TMP/server.log" 2>&1 &
SERVER_PID=$!

i=0
while [ "$i" -lt 50 ]; do
    if curl -fsS "$BASE/api.php?action=status" >/dev/null 2>&1; then
        break
    fi
    i=$((i + 1))
    sleep 0.1
done

STATUS=$(curl -fsS "$BASE/api.php?action=status")
printf '%s' "$STATUS" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false) || !($d["configured"] ?? false)) exit(1);
'

LOGIN=$(curl -fsS -X POST --data-urlencode "password=$PASSWORD" "$BASE/api.php?action=login")
TOKEN=$(printf '%s' "$LOGIN" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false) || !is_string($d["token"] ?? null)) exit(1);
echo $d["token"];
')
AUTH="Authorization: Bearer $TOKEN"

curl -fsS -X POST -H "$AUTH" \
    --data-urlencode "type=post" \
    --data-urlencode "slug=api-test" \
    --data-urlencode "title=API Test" \
    --data-urlencode "status=draft" \
    --data-urlencode "date=2026-09-23T20:00:00+10:00" \
    --data-urlencode "body=Hello **C64U** <script>bad()</script>" \
    "$BASE/api.php?action=save" >/dev/null

LIST=$(curl -fsS -H "$AUTH" "$BASE/api.php?action=list&type=post")
printf '%s' "$LIST" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
$documents=$d["documents"] ?? [];
$slugs=array_column($documents, "slug");
if (($d["count"] ?? -1) !== count($documents)) exit(1);
if (!in_array("api-test", $slugs, true)) exit(1);
'

GET=$(curl -fsS -H "$AUTH" "$BASE/api.php?action=get&type=post&slug=api-test")
printf '%s' "$GET" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (($d["document"]["body"] ?? "") !== "Hello **C64U** <script>bad()</script>") exit(1);
if (($d["document"]["status"] ?? "") !== "draft") exit(1);
'

WIRE=$(curl -fsS -H "$AUTH" "$BASE/api.php?action=get&type=post&slug=api-test&format=s64")
printf '%s' "$WIRE" | grep -q '^SEE64YOU/1'
printf '%s' "$WIRE" | grep -q '^Slug: api-test'
printf '%s' "$WIRE" | grep -q 'Hello \*\*C64U\*\* <script>bad()</script>'

WIRE_BODY=$(cat <<'WIRE_EOF'
SEE64YOU/1
Type: post
Slug: wire-test
Title: Wire Test
Status: draft
Date: 2026-09-23T20:10:00+10:00
Order: 0

A body sent without JSON escaping.
Second line.
WIRE_EOF
)
WIRE_REPLY=$(printf '%s' "$WIRE_BODY" | curl -fsS -X POST -H "$AUTH" \
    -H 'Content-Type: application/x-see64you' \
    --data-binary @- \
    "$BASE/api.php?action=save&format=s64")
printf '%s' "$WIRE_REPLY" | grep -q '^SEE64YOU/1'
printf '%s' "$WIRE_REPLY" | grep -q '^Slug: wire-test'
printf '%s' "$WIRE_REPLY" | grep -q 'Second line.'

ACK_REPLY=$(printf '%s' "$WIRE_BODY" | curl -fsS -X POST -H "$AUTH" \
    -H 'Content-Type: application/x-see64you' \
    --data-binary @- \
    "$BASE/api.php?action=save&ack=1")
printf '%s' "$ACK_REPLY" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false)) exit(1);
if (($d["slug"] ?? "") !== "wire-test") exit(1);
if (($d["status"] ?? "") !== "draft") exit(1);
if (array_key_exists("document", $d) || array_key_exists("body", $d)) exit(1);
'

CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/post.php?slug=api-test")
[ "$CODE" = "404" ]

DIRECT_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/posts/api-test.s64.php")
[ "$DIRECT_CODE" = "404" ]

curl -fsS -X POST -H "$AUTH" \
    --data-urlencode "type=post" \
    --data-urlencode "slug=api-test" \
    --data-urlencode "title=API Test" \
    --data-urlencode "status=published" \
    --data-urlencode "date=2026-09-23T20:00:00+10:00" \
    --data-urlencode "body=Hello **C64U** <script>bad()</script>" \
    "$BASE/api.php?action=save" >/dev/null

PUBLIC=$(curl -fsS "$BASE/post.php?slug=api-test")
printf '%s' "$PUBLIC" | grep -q '<strong>C64U</strong>'
if printf '%s' "$PUBLIC" | grep -q '<script>bad()'; then
    echo "FAIL: raw script reached public HTML" >&2
    exit 1
fi
printf '%s' "$PUBLIC" | grep -q '&lt;script&gt;bad()&lt;/script&gt;'

SETTINGS=$(curl -fsS -X POST -H "$AUTH" \
    --data-urlencode "site_title=Changed by API" \
    --data-urlencode "theme=c64" \
    --data-urlencode "text_color=#abcdef" \
    "$BASE/api.php?action=settings")
printf '%s' "$SETTINGS" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
$s=$d["settings"] ?? [];
if (($s["site_title"] ?? "") !== "Changed by API") exit(1);
if (($s["theme"] ?? "") !== "c64") exit(1);
if (($s["text_color"] ?? "") !== "#abcdef") exit(1);
if (array_key_exists("password_hash", $s) || array_key_exists("token_secret", $s)) exit(1);
'

curl -fsS -X POST -H "$AUTH" \
    --data-urlencode "type=post" \
    --data-urlencode "slug=api-test" \
    "$BASE/api.php?action=delete" >/dev/null

curl -fsS -X POST -H "$AUTH" \
    --data-urlencode "type=post" \
    --data-urlencode "slug=wire-test" \
    "$BASE/api.php?action=delete" >/dev/null

CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/post.php?slug=api-test")
[ "$CODE" = "404" ]

echo "HTTP API tests passed"
