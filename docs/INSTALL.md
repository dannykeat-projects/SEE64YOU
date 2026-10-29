# Install and use SEE64YOU

## Requirements

A **Commodore 64 Ultimate** with Command Interface enabled, a **16 MiB REU**, Ultimate-DOS, and an SD card or supported USB storage mount. Tested hardware baseline is C64 Ultimate firmware 1.1.0. The Editor needs no network connection. Optional publishing requires a compatible independent 8TML publishing API 1 endpoint and its standard publishing key.

This is an alpha test release. SD source replacement/recovery has bounded real-C64U evidence; physical input on this exact package, cold-power/interruption recovery, USB-only operation and complete external-reader navigation remain unverified. Back up your files and machine configuration first. Choose the complete matching package for your actual storage mount; verify its checksums when supplied. For USB mounts other than the package's exact root, rebuild with `make -B -C client STORAGE_ROOT=/YOURMOUNT/ independent-package`. A folder rename does not change the executable's embedded root.

In the Ultimate **Memory & ROMs** settings (called **C64 and Cartridge Settings** on some menus), enable **Command Interface**, **RAM Expansion Unit**, and **REU Size: 16 MB**. Recheck after a settings reset.

Copy all matching program, BIN and manifest files into `/sd/SEE64YOU/` by default, or the same folder under the selected mount. Do not remove existing documents, configuration or recovery files.

## Programs

| Launch from BASIC | Purpose |
| --- | --- |
| `SEE64YOU.PRG` | Open the Editor |
| `S64CFG.PRG` | Configure optional publishing |
| `S64PUB.PRG` | Publish local source |

Launch each program **independently** and return to BASIC when finished. `S64EDIT.PRG` and the BIN/manifest files are internal; do not launch or mix them separately. Older releases may include `S64WEB.PRG`; the current ten-file package excludes it.

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

## Optional: publish to a standard endpoint

The native client targets the **26 October 2026 API 1 test draft**. Complete hardware and compatible-reader acceptance is required before stable promotion. Alpha previews disclose the outstanding gates.

1. Obtain a dedicated standard publishing key file from your server through its HTTPS interface. Copy it as `8TMLKEY.TXT` into the selected `SEE64YOU` folder. Do not use an ordinary account password or the old key-only `SEE64KEY.TXT`. For T8wn, create the key through its [HTTPS Account page](https://t8wn.com/account/), as described in its [publishing guide](https://t8wn.com/8tml/guides/publishing/). A downloaded `.key` file is standard text; name the C64U copy `8TMLKEY.TXT`. Keep the key out of public pages and source documents.
2. Launch `S64CFG.PRG`: **I** imports the standard key, **H** changes HTTP consent, **T** validates configuration and **S** saves. Check the endpoint and ID; the secret stays hidden. A new import clears previous HTTP consent. Source sent over HTTP is not confidential; HTTPS is never silently downgraded.
3. Save your source in Editor, exit and launch `S64PUB.PRG`. In the **new numbered-key test build** press **1** to enter the local `.8tml` filename, then **2** to enter the remote relative path (for example `index.8tml`). The older **L** and **N** shortcuts still work. The local/server file pickers are **not yet implemented**.
4. Press **3** to publish (older **P** also works). Publisher checks server limits and existing filenames, displays the endpoint/path and asks for confirmation. Press **1** to confirm a public upload/replacement (older **Y** also works), or **0** to cancel. Uploaded source is public, and the standard cannot prevent races between the existing-file check and replacement.
5. Wait for the reported URL and any server warning. After an uncertain outcome, inspect the remote file before trying again. Press **4** to test the endpoint (older **T** also works), **R** to reload saved Settings, or **0** / RUN/STOP to exit to BASIC. The new numbered screen is committed but has **not yet passed new build or hardware acceptance**, so previously published alpha binaries can still show the letter-based menu.

Publisher does not provide remote editing, deletion, page/post categories or draft visibility. Keep unpublished drafts as local files and read published pages using an external compatible 8TML browser. Planned numbered local/server selectors and optional mouse input require implementation and acceptance; downloading raw source is deferred until independently verified and is not a release prerequisite. See [protocol](PROTOCOL.md) for transport limits.
## Troubleshooting

- **Missing UCI/REU:** verify the Ultimate settings above; power-cycle only as instructed by the hardware, and recheck.
- **File not found:** check the selected SD/USB mount and exact folder/filename. The application does not search another device.
- **Key or connection failure:** verify the endpoint URL, port, key ID, dedicated standard key, explicit HTTP consent and C64U network access. No silent HTTPS downgrade is supported.
- **USB corruption/readback mismatch:** do not execute or trust the copied program. Verify package hashes and readback. USB hardware acceptance remains incomplete on the tested firmware.
- **Unexpected exit or failed save:** preserve `.tmp`/recovery files, note the error and keep the original source backed up.
