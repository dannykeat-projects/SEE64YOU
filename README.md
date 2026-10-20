# SEE64YOU

**Create and publish 8TML pages from the Commodore 64 Ultimate, and browse them from the C64U too.**

## Format direction (20 October 2026)

SEE64YOU is moving to the external **8TML 1 draft** for all new documents (`.8tml`), with optional `.8ss` stylesheets. The legacy `.s64` document format is retired; no compatibility layer or conversion is planned. The user-supplied external specification and validator are our reference; they are not bundled until licensing is clarified.

**This migration is in progress.** The current default-branch editor has a placeholder for saving documents, and an existing working storage implementation must be reconciled from feature work before 8TML support can be marked implemented. Do not infer compatibility or hardware validation from this README.

See [migration and acceptance criteria](docs/8TML-MIGRATION.md) and the [authoritative work queue](todo.md).

## Repository layout

- `client/`: cc65 editor and C64U client code
- `site/`: standalone flat-file PHP site and publishing API
- `wordpress/`: historical WordPress integration, subject to MVP scope review
- `docs/`: project architecture and migration guidance
- `tools/`: local validation and development helpers

The intended MVP preserves separate Editor, Publisher and Settings programs, plus an independent 8TML browser. Source files remain unchanged when the web server generates HTML for modern browsers. Publishing transport and authentication are SEE64YOU services, not extensions to the 8TML document standard.

## Running the existing standalone site locally

Requires PHP 8.1+:

```sh
php -S 127.0.0.1:8080 -t site
```

Visit `http://127.0.0.1:8080/`. For the existing API, visit `http://127.0.0.1:8080/setup.php` for one-time configuration. **This is the existing backend, not yet an 8TML interoperability demonstration.**
