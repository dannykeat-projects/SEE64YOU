# Standalone site

The standalone backend is intended to be understandable by opening the folder in a file manager.

## Files

```
site/
  index.php
  page.php
  posts.php
  post.php
  api.php
  config.example.php
  lib/
  pages/
  posts/
```

No database and no URL rewriting are required.

## Document files

Documents use a tiny header followed by a blank line and source body:

```
Title: About
Status: published
Order: 20
Date: 2026-09-23

This page is stored as readable source text.
```

The filename is the slug, for example `about.s64`. Slugs are restricted to lowercase letters, numbers and hyphens.

## Rendering

The public site lists published content only.

Themes:
- `plain`: semantic HTML with no CSS.
- `simple`: small inline stylesheet.
- `c64`: small inline stylesheet inspired by a C64 text screen.

CSS is enhancement only. Navigation and content remain ordinary HTML links/text.

## Deployment

The final packaging workflow will produce a self-contained directory that can be uploaded to an arbitrary PHP-capable folder. Multiple installations can coexist because each installation keeps its own configuration and content.

Until the packaging task is complete, the checked-in `site/` tree itself is intended to remain self-contained.
