# SEE64YOU on the C64 Ultimate

The Editor creates and saves portable `.8tml` source files. It works offline; publishing is optional. This is experimental software: back up existing documents and configuration before installing.

Enable Command Interface and a 16 MiB RAM Expansion Unit in the Ultimate **Memory & ROMs** settings. Copy **all eleven matching** program, BIN and manifest files together into the package's specified `SEE64YOU` folder (normally `/sd/SEE64YOU/`). Do not mix SD and USB builds or execute files that fail package checksum/readback verification. A package built for `/Usb0/` will not automatically follow a different USB mount.

Launch `SEE64YOU.PRG` for the Editor; `S64CFG.PRG` is optional Settings and `S64PUB.PRG` is optional Publisher. Do not launch `S64EDIT.PRG` (an internal asset). A packaged `S64WEB.PRG` does not read 8TML.

Create a file with Ctrl+N; save with Ctrl+S or File → Save. A basename such as `HELLO` becomes `HELLO.8tml`. Reopen with Ctrl+O; F8 gives Help. **Save manually: autosave is not enabled.** Exit confirms before returning to BASIC.

Publishing needs a separate PHP 8.1+ site, HTTPS key provisioning in the browser, and explicit consent for C64U direct HTTP. Challenge/HMAC does **not** encrypt document text. Publisher/hardware acceptance and external browser/full-renderer compatibility remain incomplete.

For setup and usage see https://github.com/dannykeat-projects/SEE64YOU/blob/main/docs/INSTALL.md .
