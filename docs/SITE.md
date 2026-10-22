# Standalone site

> **Historical alpha reference only, not future 8TML requirements.** The released version described here used `.s64`/SEE64YOU/1. The next build supports only `.8tml`, with no old-file migration, importer or compatibility support. Reuse applicable hardware safety rules when replacing code; see [8TML direction](8TML-ROADMAP.md).


The standalone backend is intentionally understandable by opening the folder in a file manager. It requires PHP but no database and no URL rewriting.

The current client is C64 Ultimate-only, but the site itself remains an ordinary web application with no dependency on C64U-specific server software.

## Files

~~~text
site/
  index.php
  page.php
  posts.php
  post.php
  api.php
  pair.php
  setup.php
  config.example.php
  lib/
  pages/
  posts/
~~~

## Document files

Documents are stored as readable canonical source with a guarded PHP prefix and metadata header.

Example logical source:

~~~text
Title: About
Status: published
Order: 20
Date: 2026-09-23

This page is stored as readable source text.
~~~

The filename is the slug, for example about.s64.php. Direct content-file requests return 404 through the guard so drafts are not exposed by guessing filenames.

Slugs are restricted to lowercase letters, numbers and hyphens.

Canonical See64You source remains the editing authority; HTML is generated for display.

## Rendering

Public navigation/rendering includes published content only.

Themes:

- plain: semantic HTML with no CSS;
- simple: small inline stylesheet;
- c64: small inline stylesheet inspired by a C64 text screen.

JavaScript is not required.

Styled themes enforce readable foreground/background contrast.

### Browser theme editor

Open `themes.php` (also linked from navigation) and log in with the existing site publishing password. This is a single-owner site, not a multi-account service: named custom themes belong to the installation and selection changes the public site. Credentials require HTTPS, except loopback development. Sessions use HttpOnly cookies, expire with the existing bearer lifetime, and every form POST requires a session CSRF token. Logout removes the browser's login.

The editor creates themes from built-in presets, duplicates saved values, edits/renames, previews without writing, selects and deletes custom themes. Deleting the active custom theme selects Plain. Built-in Plain/Simple/C64 remain unchanged and cannot be edited/deleted; copy them to customize. Up to 32 named themes are stored under `custom_themes` in the existing atomically replaced `config.php`, preserving authentication settings. Existing API settings keys/response remain intact; custom theme IDs can also be selected through settings.

Controls are grouped into Page layout, Typography, Colours, and Borders/controls. Page layout can override the displayed website title, show or hide the title/navigation/header separator, switch between a boxed content panel and an open page, and adjust width, alignment, margins and spacing. Typography includes the bundled C64 face plus conservative system stacks for monospace, sans/serif, Arial, Verdana, Trebuchet MS, Tahoma, Georgia, Times New Roman and Courier New. The remaining controls cover text/background/link/visited/heading colours, panel/menu/input/button colours, font size/weight, line/letter spacing, paragraph spacing, panel borders/radius, menu layout and control padding. Colours must be six-digit hex and relevant foreground/background pairs must reach 4.5:1 contrast. Dimensions are bounded integers with fixed units; choices are enumerated. Unknown fields never become CSS. Rendering revalidates custom values and falls back safely on invalid configuration.

The editor uses ordinary GET/POST forms and text fields, with no JavaScript or native colour-picker requirement. Layout uses block flow with ordinary div wrappers, pixel/em values and literal CSS declarations; no flex/grid, CSS variables or media-query dependency. Unsupported max-width/radius/font rules may be ignored. C64 font failure falls back to Courier New/Courier/monospace. Plain remains usable with CSS disabled. No named ancient browser version is guaranteed until manually tested; old TLS support may also limit authenticated access independently of HTML/CSS.

The built-in C64 theme and custom themes using the C64 font load the locally bundled CC0 `assets/fonts/c64.ttf`, with Courier New/Courier/monospace fallbacks. The built-in C64 preset also models the familiar startup-screen relationship: the outer page uses the light text colour as the border, while an inner 40-em-wide, 25-em-minimum screen area uses the dark background colour with light text. The wrapper is ordinary block-flow HTML, so Plain and Simple remain unaffected and old browsers may safely ignore unsupported font/min-height details. TrueType loading is an enhancement, not required for access. IE versions needing EOT use the fallback; no EOT/WOFF conversion is claimed. The font contains C64-style Latin/PETSCII glyphs but Unicode mappings, browser rasterization, heading boldness and fallback glyphs mean this is not a cycle/pixel-exact C64 screen emulator. The Themes navigation link is emitted only for a browser session holding a currently valid theme-editor login token; public visitors do not see the management link. See ENGINEERING-REFERENCES.md for font provenance and licence.

## Setup/security

Browser setup is enabled only while an ENABLE_SETUP marker exists. Successful setup writes local configuration and removes the marker.

Normal browser/API authentication uses the site password to obtain short-lived bearer sessions.

C64U direct publishing uses the separate key provisioned through HTTPS pair.php; the C64U does not send the normal site password over its raw Network $03 HTTP connection.

See PROTOCOL.md for authentication details.

## Deployment

Build a self-contained upload directory with:

~~~sh
php tools/package-site.php
~~~

Default output:

dist/see64you-site/

The package includes runtime files/starter content, excludes tests/local config.php and adds a fresh ENABLE_SETUP marker.

Each installation keeps its own configuration/content, so multiple sites can coexist on ordinary PHP hosting.

The future C64U launcher/module-cache changes do not require a corresponding site-format change unless the publishing protocol itself changes.
