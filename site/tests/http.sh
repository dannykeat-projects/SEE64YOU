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
rm -f "$SITE/config.php" "$SITE/ENABLE_SETUP"

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

SETUP_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/setup.php")
[ "$SETUP_CODE" = "404" ]
curl -fsS "$BASE/start.php" | grep -q 'Step 1 of 6'
curl -fsS "$BASE/start.php?step=99" | grep -q 'Step 1 of 6'
curl -fsS "$BASE/start.php?step=5&kind=post" | grep -q 'Publish your first post'
curl -fsS "$BASE/start.php?step=5&kind=%3Cscript%3E" | grep -q 'Publish your first page'

touch "$SITE/ENABLE_SETUP"
curl -fsS "$BASE/setup.php" | grep -q 'See64You setup'

curl -fsS -X POST \
    --data-urlencode "site_title=HTTP Test" \
    --data-urlencode "theme=plain" \
    --data-urlencode "password=$PASSWORD" \
    --data-urlencode "confirm=$PASSWORD" \
    "$BASE/setup.php" | grep -q 'Setup complete'

[ -f "$SITE/config.php" ]
[ ! -f "$SITE/ENABLE_SETUP" ]
BEFORE=$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")
for STEP in 1 2 3 4 5 6; do
    GUIDE=$(curl -fsS "$BASE/start.php?step=$STEP&kind=post")
    printf '%s' "$GUIDE" | grep -q "Step $STEP of 6"
    if printf '%s' "$GUIDE" | grep -Eq '<script|type="password"'; then
        echo 'FAIL: walkthrough must not require script or collect credentials' >&2
        exit 1
    fi
done
[ "$BEFORE" = "$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")" ]
SETUP_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/setup.php")
[ "$SETUP_CODE" = "404" ]

STATUS=$(curl -fsS "$BASE/api.php?action=status")
printf '%s' "$STATUS" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false) || !($d["configured"] ?? false)) exit(1);
if (!($d["direct_auth"] ?? false)) exit(1);
'

DIRECT_SECRET=$(php -r '$c=require $argv[1]; echo $c["direct_secret"] ?? "";' "$SITE/config.php")
if [ "${#DIRECT_SECRET}" -ne 64 ]; then
    echo "FAIL: direct secret missing after setup" >&2
    exit 1
fi

CHALLENGE_REPLY=$(curl -fsS "$BASE/api.php?action=challenge")
CHALLENGE=$(printf '%s' "$CHALLENGE_REPLY" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false)) exit(1);
if (!preg_match("/^[a-f0-9]{32}$/", (string)($d["challenge"] ?? ""))) exit(1);
if (!is_int($d["expires"] ?? null) || $d["expires"] <= time()) exit(1);
echo $d["challenge"];
')
DIRECT_URI='/api.php?action=list&type=post'
DIRECT_SIG=$(php -r '
$secret=$argv[1];
$challenge=$argv[2];
$uri=$argv[3];
$body="";
$canonical="see64you-direct/1\n".$challenge."\nGET\n".$uri."\n".hash("sha256",$body);
echo hash_hmac("sha256",$canonical,hex2bin($secret));
' "$DIRECT_SECRET" "$CHALLENGE" "$DIRECT_URI")
TAMPER_CODE=$(curl -sS -o /dev/null -w '%{http_code}' \
    -H "X-See64You-Challenge: $CHALLENGE" \
    -H "X-See64You-Signature: $DIRECT_SIG" \
    "$BASE/api.php?action=list&type=page")
if [ "$TAMPER_CODE" != "401" ]; then
    echo "FAIL: tampered direct URI returned HTTP $TAMPER_CODE" >&2
    cat "$TMP/server.log" >&2 || true
    exit 1
fi

DIRECT_CODE=$(curl -sS -o "$TMP/direct-list.json" -w '%{http_code}' \
    -H "X-See64You-Challenge: $CHALLENGE" \
    -H "X-See64You-Signature: $DIRECT_SIG" \
    "$BASE$DIRECT_URI")
if [ "$DIRECT_CODE" != "200" ]; then
    echo "FAIL: valid direct signature returned HTTP $DIRECT_CODE" >&2
    cat "$TMP/direct-list.json" >&2 || true
    cat "$TMP/server.log" >&2 || true
    exit 1
fi
DIRECT_LIST=$(cat "$TMP/direct-list.json")
printf '%s' "$DIRECT_LIST" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false)) exit(1);
if (!is_int($d["count"] ?? null) || !is_array($d["documents"] ?? null)) exit(1);
if ($d["count"] !== count($d["documents"])) exit(1);
'

REPLAY_CODE=$(curl -sS -o /dev/null -w '%{http_code}' \
    -H "X-See64You-Challenge: $CHALLENGE" \
    -H "X-See64You-Signature: $DIRECT_SIG" \
    "$BASE$DIRECT_URI")
if [ "$REPLAY_CODE" != "401" ]; then
    echo "FAIL: replayed direct challenge returned HTTP $REPLAY_CODE" >&2
    exit 1
fi

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

PREVIEW_UNAUTH_CODE=$(curl -sS -o /dev/null -w '%{http_code}' \
    "$BASE/api.php?action=preview&type=post&slug=api-test")
[ "$PREVIEW_UNAUTH_CODE" = "401" ]

PREVIEW_REPLY=$(curl -fsS -H "$AUTH" \
    "$BASE/api.php?action=preview&type=post&slug=api-test")
PREVIEW_URL=$(printf '%s' "$PREVIEW_REPLY" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || !($d["ok"] ?? false)) exit(1);
if (!is_string($d["preview_url"] ?? null) || !str_starts_with($d["preview_url"], "post.php?")) exit(1);
if (!is_int($d["expires"] ?? null) || $d["expires"] <= time()) exit(1);
echo $d["preview_url"];
')
PREVIEW=$(curl -fsS -D "$TMP/preview.headers" "$BASE/$PREVIEW_URL")
printf '%s' "$PREVIEW" | grep -q 'Draft preview'
printf '%s' "$PREVIEW" | grep -q '<strong>C64U</strong>'
if printf '%s' "$PREVIEW" | grep -q '<script>bad()'; then
    echo "FAIL: raw script reached draft preview HTML" >&2
    exit 1
fi
grep -qi '^Cache-Control: private, no-store' "$TMP/preview.headers"
grep -qi '^X-Robots-Tag: noindex, nofollow' "$TMP/preview.headers"
grep -qi '^Referrer-Policy: no-referrer' "$TMP/preview.headers"

TAMPERED_PREVIEW_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/${PREVIEW_URL}x")
[ "$TAMPERED_PREVIEW_CODE" = "404" ]

WRONG_TYPE_PREVIEW=${PREVIEW_URL#post.php?}
WRONG_TYPE_PREVIEW_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/page.php?$WRONG_TYPE_PREVIEW")
[ "$WRONG_TYPE_PREVIEW_CODE" = "404" ]

DIRECT_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/posts/api-test.s64.php")
[ "$DIRECT_CODE" = "404" ]

BACKUP_UNAUTH_CODE=$(curl -sS -o /dev/null -w '%{http_code}' "$BASE/api.php?action=backup")
[ "$BACKUP_UNAUTH_CODE" = "401" ]

BACKUP=$(curl -fsS -D "$TMP/backup.headers" -H "$AUTH" "$BASE/api.php?action=backup")
grep -qi '^Content-Type: application/json' "$TMP/backup.headers"
grep -qi '^Content-Disposition: attachment; filename="see64you-backup.json"' "$TMP/backup.headers"
grep -qi '^Cache-Control: no-store' "$TMP/backup.headers"
printf '%s' "$BACKUP" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
if (!is_array($d) || ($d["format"] ?? "") !== "see64you-backup/1") exit(1);
if (!is_string($d["exported_at"] ?? null)) exit(1);
if (($d["settings"]["site_title"] ?? "") !== "HTTP Test") exit(1);
if (array_key_exists("password_hash", $d["settings"] ?? [])) exit(1);
if (array_key_exists("token_secret", $d["settings"] ?? [])) exit(1);
$posts=$d["posts"] ?? [];
$bySlug=[];
foreach ($posts as $post) $bySlug[$post["slug"] ?? ""]=$post;
if (($bySlug["api-test"]["status"] ?? "") !== "draft") exit(1);
if (($bySlug["api-test"]["body"] ?? "") !== "Hello **C64U** <script>bad()</script>") exit(1);
if (($bySlug["wire-test"]["body"] ?? "") !== "A body sent without JSON escaping.\nSecond line.") exit(1);
'

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
    --data-urlencode "theme=simple" \
    --data-urlencode "text_color=#75cec8" \
    "$BASE/api.php?action=settings")
printf '%s' "$SETTINGS" | php -r '
$d=json_decode(stream_get_contents(STDIN),true);
$s=$d["settings"] ?? [];
if (($s["site_title"] ?? "") !== "Changed by API") exit(1);
if (($s["theme"] ?? "") !== "simple") exit(1);
if (($s["text_color"] ?? "") !== "#75cec8") exit(1);
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

# Browser themes use cookies and CSRF-protected, JavaScript-free forms.
JAR="$TMP/theme-cookies"
FORM=$(curl -fsS -c "$JAR" "$BASE/themes.php")
CSRF=$(printf '%s' "$FORM" | sed -n 's/.*name="csrf" value="\([a-f0-9]*\)".*/\1/p' | head -n 1)
[ "${#CSRF}" = "64" ]
CODE=$(curl -sS -b "$JAR" -o /dev/null -w '%{http_code}' -d 'action=create&id=simple&name=Unauthorised' "$BASE/themes.php")
[ "$CODE" = "403" ]
FORM=$(curl -fsS -b "$JAR" -c "$JAR" --data-urlencode "csrf=$CSRF" --data-urlencode "action=login" --data-urlencode "password=$PASSWORD" "$BASE/themes.php")
printf '%s' "$FORM" | grep -q 'Create custom theme'
BEFORE=$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")
FORM=$(curl -fsS -b "$JAR" -d "csrf=$CSRF&action=preview&id=simple&name=Preview" "$BASE/themes.php")
printf '%s' "$FORM" | grep -q 'nothing saved'
[ "$BEFORE" = "$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")" ]
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=create&id=simple&name=Browser&font=c64" "$BASE/themes.php" | grep -q 'value="save">Save changes'
THEME_ID=$(php -r '$c=require $argv[1]; echo array_key_first($c["custom_themes"]);' "$SITE/config.php")
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=select&id=$THEME_ID" "$BASE/themes.php" >/dev/null
curl -fsS "$BASE/index.php" | grep -q 'assets/fonts/c64.ttf'
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=save&id=$THEME_ID&name=Renamed&padding=8" "$BASE/themes.php" | grep -q 'Renamed'
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=duplicate&id=$THEME_ID&name=Copy" "$BASE/themes.php" >/dev/null
php -r '$c=require $argv[1]; if(count($c["custom_themes"])!==2)exit(1);' "$SITE/config.php"
BEFORE=$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=delete&id=$THEME_ID" "$BASE/themes.php" | grep -q 'confirm delete'
[ "$BEFORE" = "$(php -r 'echo hash_file("sha256",$argv[1]);' "$SITE/config.php")" ]
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=delete&id=$THEME_ID&confirm_delete=yes" "$BASE/themes.php" >/dev/null
php -r '$c=require $argv[1]; if($c["theme"]!=="plain" || count($c["custom_themes"])!==1)exit(1);' "$SITE/config.php"
curl -fsS -b "$JAR" -d "csrf=$CSRF&action=logout" "$BASE/themes.php" | grep -q 'Site publishing password'
CODE=$(curl -sS -b "$JAR" -o /dev/null -w '%{http_code}' -d "csrf=$CSRF&action=select&id=simple" "$BASE/themes.php")
[ "$CODE" = "403" ]

# Public text/link format preserves ordinary HTML and missing-page status.
curl -fsS -D "$TMP/browser-headers" -H 'Accept: application/x-see64you-browser' "$BASE/index.php" > "$TMP/browser-body"
grep -qi 'Content-Type: application/x-see64you-browser' "$TMP/browser-headers"
grep -q '^SEE64YOU-BROWSE/1$' "$TMP/browser-body"
curl -fsS -H 'Accept: application/x-see64you-browser;q=0, text/html' "$BASE/index.php" | grep -qi '<!doctype html>'
curl -fsS -H 'Accept: application/x-see64you-browser-extra' "$BASE/index.php" | grep -qi '<!doctype html>'
curl -fsS -H 'Accept: application/x-see64you-browser' "$BASE/posts.php" | grep -q '^SEE64YOU-BROWSE/1$'
CODE=$(curl -sS -H 'Accept: application/x-see64you-browser' -o /dev/null -w '%{http_code}' "$BASE/post.php?slug=api-test")
[ "$CODE" = "404" ]

echo "HTTP API, browser format and browser theme tests passed"
