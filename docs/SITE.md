# Standalone site

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
