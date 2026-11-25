# Install and use SEE64YOU

## Requirements

A **Commodore 64 Ultimate** with Command Interface enabled, a **16 MiB REU**, Ultimate-DOS, and an SD card or supported USB storage mount. Original hardware compatibility was checked on firmware 1.1.0; firmware 1.2.1 also has bounded REST-assisted Editor/performance evidence, but final physical-input acceptance remains open. The Editor needs no network connection. Optional publishing requires a compatible independent 8TML publishing API 1 endpoint and its standard publishing key.

This is an alpha test release. SD source replacement/recovery has bounded real-C64U evidence; physical input on this exact package, cold-power/interruption recovery, USB-only operation and complete external-reader navigation remain unverified. Back up your files and machine configuration first. Choose the complete matching package for your actual storage mount; verify its checksums when supplied. For USB mounts other than the package's exact root, rebuild with `make -B -C client STORAGE_ROOT=/YOURMOUNT/ independent-package`. A folder rename does not change the executable's embedded root.

In the Ultimate **Memory & ROMs** settings (called **C64 and Cartridge Settings** on some menus), enable **Command Interface**, **RAM Expansion Unit**, and **REU Size: 16 MB**. Recheck after a settings reset.

On launch, SEE64YOU reports a pass for each verified prerequisite: Command Interface, Ultimate-DOS, 16 MB REU and writable storage. When all checks pass, it continues into the Editor without a setup prompt. If a check fails, the launcher displays the setting or storage path to check. Press **C= + RESTORE** to open the Ultimate menu, then **F2** for Settings. After applying a setting and returning to SEE64YOU, press **R** to recheck. If the firmware restarts the C64, launch `SEE64YOU.PRG` again. Use **X** to exit to BASIC.

Set Command Interface and REU options in the Ultimate menu. SEE64YOU uses temporary runtime changes for operations such as module verification and restores the original machine state on exit.

**Alpha.6 and later use `SYSTEM/`; alpha.5 and earlier retain their original flat inventories. Do not rearrange an older ZIP.** Copy the **complete matching folder tree** to `/sd/SEE64YOU/` by default (or the selected USB mount): keep `SEE64YOU.PRG`, `S64EDIT.PRG`, `S64PUB.PRG` and `S64CFG.PRG` directly in `SEE64YOU/`, and keep the four `.BIN` files plus `S64OVL.MNF` and `S64MODULE.MNF` in `SEE64YOU/SYSTEM/`. Do not mix these files with an older flat-layout package. Alpha.7 and later store editable source files in `SEE64YOU/docs/`; alpha.6 stores its 8TML documents at the root. `SEE64YOU.CFG`, `8TML.KEY`, recovery snapshots and journals remain in `SEE64YOU/`. Existing sources are not moved or deleted automatically: back up and copy them into `docs/` manually when upgrading, retaining all `.tmp`/`.bak` and recovery files until resolved. Physical input, cold power and USB-only acceptance remain unverified.

## Programs

| Launch from BASIC | Purpose |
| --- | --- |
| `SEE64YOU.PRG` | Open the Editor |
| `S64CFG.PRG` | Configure optional publishing |
| `S64PUB.PRG` | Publish local source |

Launch each program **independently** and return to BASIC when finished. `S64EDIT.PRG` and the BIN/manifest files are internal; do not launch or mix them separately. Older releases may include `S64WEB.PRG`; the current ten-file package excludes it.

## Create your first page

1. Launch `SEE64YOU.PRG` and choose New (Ctrl+N, or the File menu), then press **1** for an 8TML document. Press **2** for an empty 8SS stylesheet, **3** for empty plain text, or **0** to cancel.
2. New source begins with an editable 8TML header, including `8tml: 1` and a title. Keep the `---` header delimiters and version entry intact. Enter content below the header. For example:

   ~~~text
   ---
   8tml: 1
   title: Hello
   ---

   # Hello

   My first page.
   ~~~

3. Save with Ctrl+S or File → Save. A basename such as `HELLO` becomes `HELLO.8tml` in `SEE64YOU/docs/`. The Editor creates `docs/` on the first save if necessary. Use Save As to choose a different name. Existing files are protected from unintended overwrite.
4. Reopen by entering the **complete filename** with Ctrl+O or File → Open, such as `HELLO.8tml`. F8 opens Help. Ctrl+Z / Ctrl+Shift+Z perform undo/redo where the keyboard mapping supports them.
5. RUN/STOP or File → Exit asks for confirmation. Saved documents require Y to exit; unsaved documents offer Save, Discard or Cancel.

**Other formats:** an 8SS file starts empty and defaults to `.8ss`. Plain text starts empty and defaults to `.txt`; Save As also accepts explicit safe suffixes such as `.md` and `.ini`. Publisher accepts `.8tml`, `.8ss` and `.txt`. Other text suffixes remain local; Save As `.txt` to publish their literal text. Format shortcuts insert 8TML markup only when editing 8TML files.

**Save manually.** There are no timed background saves. Recovery snapshots use `SEE64YOU.REC` for 8TML, `SEE64YOU.8SS.REC` for 8SS and `SEE64YOU.TXT.REC` for text; failed reads preserve them, and recovered content must be saved again. If multiple snapshots exist, resolve them externally before choosing a recovery. Retain completed `.tmp`/`.bak` files if a disk operation fails. Back up source files independently. All formats use ASCII/LF; CR/CRLF input normalizes to LF; the complete source limit is 61,440 bytes.

## Optional: publish to a standard endpoint

The native client targets the **26 October 2026 API 1 test draft**. Complete hardware and compatible-reader acceptance is required before stable promotion. Alpha previews disclose the outstanding gates.

1. Obtain a dedicated standard publishing key file from your server through its HTTPS interface. Copy it as `8TML.KEY` into the selected `SEE64YOU` folder. Do not use an ordinary account password or the old key-only `SEE64KEY.TXT`. For T8wn, create the key through its [HTTPS Account page](https://t8wn.com/account/), as described in its [publishing guide](https://t8wn.com/8tml/guides/publishing/). A downloaded `.key` file is standard text; name the C64U copy `8TML.KEY`. Keep the key out of public pages and source documents.
2. Launch `S64CFG.PRG`: **1** imports the standard key, **2** changes HTTP policy, **3** disables publishing, **4** validates configuration, **5** saves and returns, and **0** cancels. The previous letter shortcuts remain available. Check the endpoint and ID; the secret stays hidden. A new import enables HTTP by default; use **2** to block it before saving if required. Source sent over HTTP is not confidential; HTTPS is never silently downgraded.
3. Save your source, exit Editor and launch `S64PUB.PRG`. Press **1** to select a local source, **2** to choose a remote target, then **3** to review and confirm publication. Check the reported URL and any server warning. If the outcome is uncertain, inspect the remote content before retrying.

See [Publisher controls](PUBLISHER.md) for typed names, paging, confirmation and recovery after errors. Use an external compatible 8TML reader to view published pages. See the [work queue](../todo.md) for the exact scope of hardware acceptance.

## Troubleshooting

- **Missing UCI/REU:** verify the Ultimate settings above; power-cycle only as instructed by the hardware, and recheck.
- **File not found:** check the selected SD/USB mount and exact folder/filename. The application does not search another device.
- **Key or connection failure:** verify the endpoint URL, port, key ID, dedicated standard key, explicit HTTP consent and C64U network access. No silent HTTPS downgrade is supported.
- **USB corruption/readback mismatch:** do not execute or trust the copied program. Verify package hashes and readback. USB hardware acceptance remains incomplete on the tested firmware.
- **Unexpected exit or failed save:** preserve `.tmp`/recovery files, note the error and keep the original source backed up.
