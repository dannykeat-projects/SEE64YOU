# See64You

**Publish to the web from the Commodore 64 Ultimate, and browse from it too**

See64You is a text editor and publishing system for the Commodore 64 Ultimate (C64U). 

The publishing layer keeps web complexity on the server, which hosts a tiny database-free website.

The text editor should be familiar to modern users, mimicking modern basic text editors in function.

Use Markdown to format text.

Browse websites built with See64You from the C64U.

The websites generated are extremely simple and should work on ancient browsers available on other systems.


## Repository layout

```
client/       C64/C64U editor client (cc65)
site/         Standalone flat-file PHP site and publishing API
wordpress/    WordPress publishing plugin
docs/         Architecture, editor, protocol and deployment notes
tools/        Local validation/development helpers
```

## Current status

The project is in its first implementation phase. The repository includes the initial editor core, flat-file web backend and WordPress integration scaffold. See `todo.md` for the authoritative work queue.

## Design principle

The C64 client works with documents, not HTML:

```
Document -> Publisher interface -> Standalone PHP site
                             \-> WordPress plugin
```

A document has a title, body, type (page/post), status, slug and metadata. Publishing backends are responsible for storage and HTML rendering on the live site.

## Running the standalone site locally

Requires PHP 8.1+.

```sh
php -S 127.0.0.1:8080 -t site
```

Then open `http://127.0.0.1:8080/`.

For publishing/API access, open `http://127.0.0.1:8080/setup.php` and complete the one-time setup form. It creates `config.php` with a password hash and random token-signing secret. Manual setup using `config.example.php` remains available.
