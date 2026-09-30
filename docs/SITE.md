# Standalone site

The standalone backend is intentionally understandable by opening the folder in a file manager. It requires PHP but no database and no URL rewriting.

## Files

```
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
```

## Document files

Documents are stored as ordinary readable source with a small guarded PHP prefix and metadata header. A typical logical document contains:

```
Title: About
Status: published
Order: 20
Date: 2026-09-23

This page is stored as readable source text.
```

The filename is the slug, for example `about.s64.php`. Direct requests to content files return 404 through the guard so drafts are not exposed by guessing a filename. Slugs are restricted to lowercase letters, numbers and hyphens.

Canonical See64You source remains the editing authority; HTML is rendered on request.

## Rendering

Public navigation/rendering includes published content only.

Themes:

- `plain`: semantic HTML with no CSS;
- `simple`: small inline stylesheet;
- `c64`: small inline stylesheet inspired by a C64 text screen.

JavaScript is not required. Styled themes enforce readable foreground/background contrast.

## Setup/security

Browser setup is enabled only while an `ENABLE_SETUP` marker exists. Successful setup writes the local configuration and removes the marker.

Normal browser/API authentication uses the site password to obtain short-lived bearer sessions. C64U direct publishing uses the separate key provisioned through HTTPS `pair.php`; the C64U does not send the normal site password over its raw HTTP connection.

See `PROTOCOL.md` for authentication details.

## Deployment

Build a self-contained upload directory with:

```sh
php tools/package-site.php
```

Default output:

`dist/see64you-site/`

The package includes runtime files and starter content, excludes tests and local `config.php`, and adds a fresh `ENABLE_SETUP` marker. An alternate output directory may be supplied as the first argument.

Each installation keeps its own configuration/content, so multiple installations can coexist on ordinary PHP hosting.
