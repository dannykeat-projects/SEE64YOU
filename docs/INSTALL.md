# Install and use SEE64YOU

## Requirements

A **Commodore 64 Ultimate** with Command Interface enabled, a **16 MiB REU**, Ultimate-DOS, and an SD card or supported USB storage mount. Tested hardware baseline is C64 Ultimate firmware 1.1.0. The Editor needs no network connection. Optional publishing requires a separate PHP 8.1+ website and a reachable HTTP publishing endpoint.

This is a test release. Back up your files and machine configuration first. Choose the complete matching package for your actual storage mount; verify its checksums when supplied. For USB mounts other than the package's exact root, rebuild with `make -B -C client STORAGE_ROOT=/YOURMOUNT/ independent-package`. A folder rename does not change the executable's embedded root.

In the Ultimate **Memory & ROMs** settings (called **C64 and Cartridge Settings** on some menus), enable **Command Interface**, **RAM Expansion Unit**, and **REU Size: 16 MB**. Recheck after a settings reset.

Copy the complete matching program, BIN and manifest files into `/sd/SEE64YOU/` by default, or the same folder under the selected mount. Do not remove existing documents, configuration or recovery files.

## Programs

| Launch from BASIC | Purpose |
| --- | --- |
| `SEE64YOU.PRG` | Open the Editor |
| `S64CFG.PRG` | Configure optional publishing |
| `S64PUB.PRG` | Publish or download source |

Launch each program **independently** and return to BASIC when finished. `S64EDIT.PRG` and the BIN/manifest files are internal; do not launch or mix them separately. Use a compatible 8TML/T8WN browser to read published files.

## Create your first page

1. Launch `SEE64YOU.PRG` and choose New (Ctrl+N, or the File menu).
2. New source begins with an editable 8TML header, including `8tml: 1` and a title. Keep the `---` header delimiters and version entry intact. Enter content below the header. For example:

   ~~~text
   ---
   8tml: 1
   title: Hello
   ---

   # Hello

   My first page.
   ~~~

3. Save with Ctrl+S or File → Save. A basename such as `HELLO` becomes `HELLO.8tml` in the application's folder. Use Save As to choose a different name. Existing files are protected from unintended overwrite.
4. Reopen with Ctrl+O or File → Open. F8 opens Help. Ctrl+Z/ Ctrl+Shift+Z perform undo/redo where the keyboard mapping supports them.
5. RUN/STOP or File → Exit asks for confirmation. Saved documents require Y to exit; unsaved documents offer Save, Discard or Cancel.

**Save manually.** There are no timed background saves. Existing `SEE64YOU.REC` snapshots may offer `Recover snapshot? Y/N`; failed reads preserve them, and recovered content must be saved again. Retain any completed `.tmp` file if a disk operation fails. Back up important `.8tml` files independently. Current source is ASCII/LF; CR/CRLF input normalizes to LF; complete source limit is 61,440 bytes.

## Optional: publish to your own PHP site

1. Upload the separate SEE64YOU PHP site package to an **empty/disposable** PHP 8.1+ hosting directory, including hidden access-control files. Ensure `data/` is blocked from direct web access and the server can write its configuration, `pages/` and `posts/`.
2. Open `start.php`, then `setup.php` in a modern browser. Complete setup; use `pair.php` **over HTTPS** to provision a dedicated publishing key.
3. Run `S64CFG.PRG`. Configure the site-folder URL (without `api.php`) and dedicated key, and confirm `SETTINGS SAVED AND VERIFIED`. Direct firmware-1.1 publishing accepts an explicit `http://` URL and port 1–65535; its content is **unencrypted**. Enable the explicit HTTP consent only when appropriate. Do not put your site login password into the C64U Publisher.
4. Save the page in Editor and exit. Launch `S64PUB.PRG`, press **L** to load its `.8tml` file, **2** to test the key, **T** to select page/post, and **4** to select draft/published. Press **3** to publish; a first upload asks for a remote slug. Wait for the success message before closing.
5. Check the public page in a modern browser. For an existing remote item, the Publisher can select page/post with **6**/**7** and save its source under a **new** local name with **D**. Reopen the downloaded source in Editor. Never assume a document was updated merely because the Publisher exited successfully. **Caution:** the current service can overwrite an existing document with the same page/post slug, even when the Publisher has no bound remote identity. Verify the target and maintain a separate backup until create/update safeguards are implemented.

The Publisher also exposes remote deletion; it is not necessary for ordinary first-page publishing. Use it only when certain of the remote document. Authentication prevents unauthorized writes and replay but does **not** keep HTTP source private. Public HTML currently supports only a subset of the 8TML language. Publishing to T8WN or other external services is not yet supported.

## Troubleshooting

- **Missing UCI/REU:** verify the Ultimate settings above; power-cycle only as instructed by the hardware, and recheck.
- **File not found:** check the selected SD/USB mount and exact folder/filename. The application does not search another device.
- **Key or connection failure:** verify the site URL, port, HTTPS pairing, dedicated key, explicit HTTP consent and C64U network access. No silent HTTPS downgrade is supported.
- **USB corruption/readback mismatch:** do not execute or trust the copied program. Verify package hashes and readback. USB hardware acceptance remains incomplete on the tested firmware.
- **Unexpected exit or failed save:** preserve `.tmp`/recovery files, note the error and keep the original source backed up.
