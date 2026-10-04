TARGET := build/see64you.prg
C64U_TARGET := build/see64you-c64u.prg
C64U_CHAIN_TARGET := build/SEE64YOU-C64U.PRG
C64U_SHORT_CHAIN_TARGET := build/S64EDIT.PRG
MANAGED_EDITOR_FIT_TARGET := build/managed-editor-fit.prg
LAUNCHER_TARGET := build/SEE64YOU.PRG
LAUNCHER_IMAGE := build/see64you-launcher.prg
MODULE_CACHE_GATE_TARGET := build/module-cache-gate.prg
EDITOR_PROBE_TARGET := build/editor-probe.prg
UI_PROBE_TARGET := build/ui-probe.prg
INPUT_REPEAT_PROBE_TARGET := build/input-repeat-probe.prg
PUBLISHER_CHARSET_PROBE_TARGET := build/publisher-charset-probe.prg
UCI_ABSENCE_PROBE_TARGET := build/uci-absence-probe.prg
NETWORK_PROBE_TARGET := build/network-probe.prg
HTTPS_PROBE_TARGET := build/https-probe.prg
DIRECT_SITE_PROBE_TARGET := build/direct-site-probe.prg
DIRECT_AUTH_PROBE_TARGET := build/direct-auth-probe.prg
HMAC_PROBE_TARGET := build/hmac-probe.prg
SETTINGS_IO_PROBE_TARGET := build/settings-io-probe.prg
SETTINGS_EXCHANGE_PROBE_TARGET := build/settings-exchange-probe.prg
SETTINGS_SAVE_PROBE_TARGET := build/settings-save-probe.prg
SETTINGS_LOAD_PROBE_TARGET := build/settings-load-probe.prg
COMPANION_STREAM_PROBE_TARGET := build/companion-stream-probe.prg
COMPANION_LAUNCH_PROBE_TARGET := build/companion-launch-probe.prg
COMPANION_LOADER_TARGET := build/S64LDTST.PRG
RESIDENT_LOADER_BIN := build/companion-resident.bin
RESIDENT_LOADER_OBJ := build/companion-resident.o
RESIDENT_LOADER_BLOB_OBJ := build/companion-resident-blob.o
PLATFORM_PREFLIGHT_PROBE_TARGET := build/platform-preflight-probe.prg
REU_PROBE_TARGET := build/reu-probe.prg
REU_BUFFER_PROBE_TARGET := build/reu-buffer-probe.prg
REU_EDITOR_PROBE_TARGET := build/reu-editor-probe.prg
RECOVERY_PROBE_TARGET := build/recovery-probe.prg
PUBLISHER_COMPANION_FIT_TARGET := build/publisher-companion-fit.prg
PUBLISHER_TARGET := build/see64you-publisher.prg
PUBLISHER_CHAIN_TARGET := build/S64PUB.PRG
SETTINGS_TARGET := build/see64you-settings.prg
SETTINGS_CHAIN_TARGET := build/S64CFG.PRG
MODULE_MANIFEST_TARGET := build/S64MODULE.MNF
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/menu.c src/input/input.c src/input/repeat.s src/platform/uci.c src/platform/dos.c src/storage/local.c
C64U_SOURCES := $(filter-out src/buffer.c src/history.c,$(SOURCES)) src/buffer_reu.c src/history_reu.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/editor_handoff.c src/publisher/editor_handoff_record.s
# Destinations validate existing state; allocation and manifest parsing stay in the launcher.
MANAGED_EDITOR_RUNTIME_SOURCES := src/platform/code_overlay_bootstrap.c src/platform/module_arrival.c src/platform/module_runtime.c src/platform/transfer_capsule.c src/platform/module_session.c src/platform/module_session_read.c src/platform/module_manifest.c src/platform/reu_runtime.c src/platform/session_lifecycle.c src/platform/session_journal.c src/platform/system_state.c src/platform/turbo.c
CODE_ASSET_RUNTIME_SOURCES := src/platform/code_assets_validate.c src/platform/code_assets_session.c src/platform/code_assets_session_read.c
CODE_ASSET_SOURCES := src/platform/code_assets.c src/platform/code_assets_validate.c src/platform/code_assets_session.c src/platform/code_assets_session_build.c
EDITOR_PROBE_SOURCES := diagnostics/editor_probe.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c
UI_PROBE_SOURCES := diagnostics/ui_probe.c diagnostics/ui_conio_shim.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s
INPUT_REPEAT_PROBE_SOURCES := diagnostics/input_repeat_probe.c src/input/repeat.s
PUBLISHER_CHARSET_PROBE_SOURCES := diagnostics/publisher_charset_probe.c src/buffer.c src/document.c src/charset.c
UCI_ABSENCE_PROBE_SOURCES := diagnostics/uci_absence_probe.c src/platform/uci.c src/charset.c
NETWORK_PROBE_SOURCES := diagnostics/network_probe.c src/platform/uci.c src/platform/network.c src/charset.c
HTTPS_PROBE_SOURCES := diagnostics/https_probe.c src/platform/uci.c src/platform/http.c src/charset.c
DIRECT_SITE_PROBE_SOURCES := diagnostics/direct_site_probe.c src/platform/uci.c src/platform/network.c src/charset.c
DIRECT_AUTH_PROBE_SOURCES := diagnostics/direct_auth_probe.c src/charset.c src/crypto/sha256.c src/platform/uci.c src/platform/network.c src/platform/dos.c src/platform/system_state.c src/platform/system_state_start.c src/platform/turbo.c src/platform/turbo_capture.c src/platform/reu.c src/platform/reu_dma.s src/publisher/direct_auth.c src/publisher/settings_exchange.c src/publisher/settings_store.c
HMAC_PROBE_SOURCES := diagnostics/hmac_probe.c src/crypto/sha256.c
SETTINGS_IO_PROBE_SOURCES := diagnostics/settings_io_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/publisher/settings_exchange.c src/platform/reu.c src/platform/reu_dma.s
SETTINGS_EXCHANGE_PROBE_SOURCES := diagnostics/settings_exchange_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/publisher/settings_store.c
COMPANION_STREAM_PROBE_SOURCES := diagnostics/companion_stream_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/platform/dos_file.c src/platform/companion_loader.c
COMPANION_LAUNCH_PROBE_SOURCES := diagnostics/companion_launch_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/platform/dos_file.c src/platform/companion_loader.c src/platform/companion_resident_enter.s
COMPANION_LOADER_TARGET_SOURCES := diagnostics/companion_loader_target.c diagnostics/companion_loader_target_pad.s
PLATFORM_PREFLIGHT_PROBE_SOURCES := diagnostics/platform_preflight_probe.c src/charset.c src/platform/preflight.c src/platform/requirements.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s
REU_PROBE_SOURCES := diagnostics/reu_probe.c src/platform/reu.c src/platform/reu_dma.s
REU_BUFFER_PROBE_SOURCES := diagnostics/reu_buffer_probe.c src/buffer_reu.c src/platform/reu.c src/platform/reu_dma.s
REU_EDITOR_PROBE_SOURCES := diagnostics/reu_editor_probe.c src/buffer_reu.c src/history_reu.c src/clipboard.c src/editor.c src/platform/reu.c src/platform/reu_dma.s
RECOVERY_PROBE_SOURCES := diagnostics/recovery_probe.c src/buffer.c src/document.c src/charset.c src/platform/dos.c src/storage/local.c
PUBLISHER_CORE_SOURCES := src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/http.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/publisher/destination.c src/publisher/state.c src/publisher/workspace.c src/publisher/handoff.c src/publisher/plaintext.c src/publisher/preview.c src/publisher/relay.c src/publisher/settings_bridge.c src/publisher/standalone.c
PUBLISHER_DOCUMENT_SOURCES := src/publisher/standalone_documents.c
PUBLISHER_COMPANION_FIT_SOURCES := diagnostics/publisher_companion_fit.c $(PUBLISHER_CORE_SOURCES) $(PUBLISHER_DOCUMENT_SOURCES)
PUBLISHER_SOURCES := src/publisher/main.c src/platform/chain_editor.s $(filter-out src/platform/http.c src/publisher/relay.c src/publisher/standalone.c,$(PUBLISHER_CORE_SOURCES)) src/platform/network.c src/crypto/sha256.c src/publisher/direct_auth.c src/publisher/standalone_direct.c
SETTINGS_SOURCES := src/publisher/settings_main.c src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/destination.c src/publisher/settings_exchange.c src/publisher/settings_store.c
LAUNCHER_SOURCES := src/platform/module_session.c src/platform/module_session_read.c src/platform/module_session_build.c src/platform/transfer_capsule.c src/platform/transfer_capsule_build.c src/platform/transfer_capsule_write.c src/platform/module_transfer.c src/platform/module_arrival.c src/platform/module_transfer_stub.s src/launcher/main.c src/platform/module_integrity.c src/platform/crc32.c src/platform/module_preload.c src/platform/dos_reu.c src/platform/dos_file.c src/launcher/recovery_runtime.c src/launcher/recovery_flow.c src/platform/preflight.c src/platform/requirements.c src/platform/module_manifest.c src/platform/module_manifest_build.c src/platform/module_manifest_parse.c src/platform/reu_runtime.c src/platform/reu_runtime_build.c src/platform/reu_runtime_write.c src/platform/session_lifecycle.c src/platform/session_lifecycle_start.c src/platform/session_recovery.c src/platform/session_journal.c src/platform/session_journal_start.c src/platform/system_state.c src/platform/system_state_start.c src/platform/dos.c src/platform/turbo.c src/platform/turbo_capture.c src/platform/uci.c src/platform/reu.c src/platform/reu_dma.s src/charset.c

CL65 ?= cl65
CA65 ?= ca65
LD65 ?= ld65
# Keep the active conventional-C64 editor inside base RAM. Basic formatting is
# linked in editor_format.c. The larger URL/link formatter remains host/module-tested
# but unlinked because it does not fit while retaining the 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all c64u managed-editor-fit launcher publisher settings module-manifest editor-probe ui-probe input-repeat-probe publisher-charset-probe uci-absence-probe platform-preflight-probe network-probe https-probe direct-site-probe direct-auth-probe hmac-probe settings-io-probe settings-exchange-probe settings-save-probe settings-load-probe companion-stream-probe companion-launch-probe companion-loader-target reu-probe reu-buffer-probe reu-editor-probe recovery-probe publisher-companion-fit clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

c64u: $(C64U_TARGET) $(C64U_CHAIN_TARGET) $(C64U_SHORT_CHAIN_TARGET) $(SETTINGS_IO_PROBE_TARGET) $(SETTINGS_EXCHANGE_PROBE_TARGET) $(SETTINGS_SAVE_PROBE_TARGET) $(SETTINGS_LOAD_PROBE_TARGET)

module-manifest: $(MODULE_MANIFEST_TARGET)

$(MODULE_MANIFEST_TARGET): $(C64U_SHORT_CHAIN_TARGET) $(PUBLISHER_CHAIN_TARGET) $(SETTINGS_CHAIN_TARGET)
	python3 ../tools/generate_module_manifest.py build --output $@
	python3 ../tools/generate_module_manifest.py build --validate $@

launcher: $(LAUNCHER_IMAGE)
	# Refresh the public name even on case-insensitive Windows filesystems.
	cp $(LAUNCHER_IMAGE) $(LAUNCHER_TARGET)

$(LAUNCHER_IMAGE): $(LAUNCHER_SOURCES) src/platform/package_version.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you-launcher.map -o $@ $(LAUNCHER_SOURCES)

.PHONY: module-cache-gate
module-cache-gate: $(MODULE_CACHE_GATE_TARGET)

$(MODULE_CACHE_GATE_TARGET): diagnostics/module_cache_gate.c $(LAUNCHER_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/module-cache-gate.map -o $@ diagnostics/module_cache_gate.c $(filter-out src/launcher/main.c,$(LAUNCHER_SOURCES))

.PHONY: companion-stream-probe
companion-stream-probe: $(COMPANION_STREAM_PROBE_TARGET)

$(COMPANION_STREAM_PROBE_TARGET): $(COMPANION_STREAM_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/companion-stream-probe.map -o $@ $(COMPANION_STREAM_PROBE_SOURCES)

.PHONY: companion-loader-target
companion-loader-target: $(COMPANION_LOADER_TARGET)

$(COMPANION_LOADER_TARGET): $(COMPANION_LOADER_TARGET_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/companion-loader-target.map -o $@ $(COMPANION_LOADER_TARGET_SOURCES)

$(RESIDENT_LOADER_OBJ): src/platform/companion_resident.s
	@mkdir -p build
	$(CA65) -o $@ $<

$(RESIDENT_LOADER_BIN): $(RESIDENT_LOADER_OBJ) cfg/companion_resident.cfg
	$(LD65) -C cfg/companion_resident.cfg -o $@ $(RESIDENT_LOADER_OBJ)

$(RESIDENT_LOADER_BLOB_OBJ): src/platform/companion_resident_blob.s $(RESIDENT_LOADER_BIN)
	$(CA65) -I build -o $@ $<

.PHONY: companion-launch-probe
companion-launch-probe: $(COMPANION_LAUNCH_PROBE_TARGET) $(COMPANION_LOADER_TARGET)

$(COMPANION_LAUNCH_PROBE_TARGET): $(COMPANION_LAUNCH_PROBE_SOURCES) $(RESIDENT_LOADER_BLOB_OBJ)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/companion-launch-probe.map -o $@ $(COMPANION_LAUNCH_PROBE_SOURCES) $(RESIDENT_LOADER_BLOB_OBJ)

.PHONY: settings-load-probe
settings-load-probe: $(SETTINGS_LOAD_PROBE_TARGET)

$(SETTINGS_LOAD_PROBE_TARGET): diagnostics/settings_load_probe.c $(SETTINGS_SOURCES) diagnostics/settings_save_trace.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/settings-load-probe.map -o $@ diagnostics/settings_load_probe.c $(filter-out src/publisher/settings_main.c,$(SETTINGS_SOURCES))

.PHONY: settings-save-probe
settings-save-probe: $(SETTINGS_SAVE_PROBE_TARGET)

$(SETTINGS_SAVE_PROBE_TARGET): $(SETTINGS_SOURCES) diagnostics/settings_save_trace.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_SETTINGS_SAVE_TRACE=1 -m build/settings-save-probe.map -o $@ $(SETTINGS_SOURCES)

.PHONY: settings-io-probe
settings-io-probe: $(SETTINGS_IO_PROBE_TARGET)

$(SETTINGS_IO_PROBE_TARGET): $(SETTINGS_IO_PROBE_SOURCES) src/publisher/settings_store.c
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/settings-io-probe.map -o $@ $(SETTINGS_IO_PROBE_SOURCES)

.PHONY: settings-exchange-probe
settings-exchange-probe: $(SETTINGS_EXCHANGE_PROBE_TARGET)

$(SETTINGS_EXCHANGE_PROBE_TARGET): $(SETTINGS_EXCHANGE_PROBE_SOURCES) src/publisher/settings_exchange.c
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/settings-exchange-probe.map -o $@ $(SETTINGS_EXCHANGE_PROBE_SOURCES)

$(C64U_TARGET): $(C64U_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-c64u.map -o $@ $(C64U_SOURCES)

$(C64U_CHAIN_TARGET): $(C64U_TARGET)
	cp $< $@

$(C64U_SHORT_CHAIN_TARGET): $(C64U_TARGET)
	cp $< $@

managed-editor-fit: $(MANAGED_EDITOR_FIT_TARGET)

.PHONY: editor-overlay-fit
editor-overlay-fit:
	python3 ../tools/build_editor_overlay_fit.py

# Reviewable preload package only. Its launcher stops before executable DMA.
.PHONY: managed-package
managed-package: editor-overlay-fit code-assets-fit publisher settings
	python3 ../tools/check_memory_headroom.py build/code-assets-fit.map --label managed-launcher --min-headroom 1024
	@mkdir -p build/managed-package
	cp build/code-assets-fit.prg build/managed-package/SEE64YOU.PRG
	cp build/editor-overlay-fit/editor-overlay-fit.prg build/managed-package/S64EDIT.PRG
	cp build/S64PUB.PRG build/S64CFG.PRG build/managed-package/
	cp build/editor-overlay-fit/S64MODEL.BIN build/editor-overlay-fit/S64FILE.BIN build/editor-overlay-fit/S64RUN.BIN build/editor-overlay-fit/S64OVL.MNF build/managed-package/
	python3 ../tools/generate_code_asset_manifest.py build/managed-package --root build/managed-package/S64EDIT.PRG --validate build/managed-package/S64OVL.MNF
	python3 ../tools/generate_module_manifest.py build/managed-package --output build/managed-package/S64MODULE.MNF
	python3 ../tools/generate_module_manifest.py build/managed-package --validate build/managed-package/S64MODULE.MNF

.PHONY: code-assets-fit
# Asset-preload diagnostic; executable handoff remains disabled.
code-assets-fit: build/code-assets-fit.prg

build/code-assets-fit.prg: $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/package_version.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_CODE_ASSET_LAUNCHER=1 -m build/code-assets-fit.map -o $@ $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c

.PHONY: managed-handoff-package
managed-handoff-package: managed-package build/managed-handoff-launcher.prg
	python3 ../tools/check_memory_headroom.py build/managed-handoff-launcher.map --label managed-handoff-launcher --min-headroom 1024
	cp build/managed-handoff-launcher.prg build/managed-package/SEE64YOU.PRG

# Current supported package: verified executable launcher plus managed editor.
# Publisher/Settings remain cached companions; their ownership cycles are gated.
C64U_PACKAGE_FILES := SEE64YOU.PRG S64EDIT.PRG S64PUB.PRG S64CFG.PRG S64MODULE.MNF S64OVL.MNF S64MODEL.BIN S64FILE.BIN S64RUN.BIN
.PHONY: c64u-package
c64u-package: managed-handoff-package
	@mkdir -p build/c64u-package
	cp $(addprefix build/managed-package/,$(C64U_PACKAGE_FILES)) build/c64u-package/
	python3 ../tools/generate_module_manifest.py build/c64u-package --validate build/c64u-package/S64MODULE.MNF
	python3 ../tools/generate_code_asset_manifest.py build/c64u-package --root build/c64u-package/S64EDIT.PRG --validate build/c64u-package/S64OVL.MNF
	python3 ../tools/check_memory_headroom.py build/see64you-publisher.map --label publisher --min-headroom 1024
	python3 ../tools/check_memory_headroom.py build/see64you-settings.map --label settings --min-headroom 1024

build/managed-handoff-launcher.prg: $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/package_version.h src/platform/basic_return_capture.s src/platform/basic_return.c src/platform/basic_return_build.c src/launcher/basic_return_runtime.c
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_CODE_ASSET_LAUNCHER=1 -DS64_EXECUTABLE_HANDOFF=1 -m build/managed-handoff-launcher.map -o $@ src/platform/basic_return_capture.s $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/basic_return.c src/platform/basic_return_build.c src/launcher/basic_return_runtime.c

# Deliberate fit gate for the next managed destination; not a product artifact.
MANAGED_PUBLISHER_RUNTIME_SOURCES := $(filter-out src/platform/code_overlay_bootstrap.c src/platform/module_arrival.c,$(MANAGED_EDITOR_RUNTIME_SOURCES)) src/platform/transfer_capsule_write.c
.PHONY: publisher-overlay-fit
publisher-overlay-fit:
	python3 ../tools/build_publisher_overlay_fit.py

.PHONY: managed-publisher-fit
managed-publisher-fit: $(PUBLISHER_SOURCES) $(MANAGED_PUBLISHER_RUNTIME_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -DS64_DIRECT_PUBLISHER=1 -m build/managed-publisher-fit.map -o build/managed-publisher-fit.prg $(PUBLISHER_SOURCES) $(MANAGED_PUBLISHER_RUNTIME_SOURCES)
	python3 ../tools/check_memory_headroom.py build/managed-publisher-fit.map --label managed-publisher-fit --min-headroom 1024

$(MANAGED_EDITOR_FIT_TARGET): $(C64U_SOURCES) $(MANAGED_EDITOR_RUNTIME_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/managed-editor-fit.map -o $@ $(C64U_SOURCES) $(MANAGED_EDITOR_RUNTIME_SOURCES)

editor-probe: $(EDITOR_PROBE_TARGET)

$(EDITOR_PROBE_TARGET): $(EDITOR_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/editor-probe.map -o $@ $(EDITOR_PROBE_SOURCES)

ui-probe: $(UI_PROBE_TARGET)

$(UI_PROBE_TARGET): $(UI_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/ui-probe.map -o $@ $(UI_PROBE_SOURCES)

input-repeat-probe: $(INPUT_REPEAT_PROBE_TARGET)

$(INPUT_REPEAT_PROBE_TARGET): $(INPUT_REPEAT_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/input-repeat-probe.map -o $@ $(INPUT_REPEAT_PROBE_SOURCES)

publisher-charset-probe: $(PUBLISHER_CHARSET_PROBE_TARGET)

$(PUBLISHER_CHARSET_PROBE_TARGET): $(PUBLISHER_CHARSET_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/publisher-charset-probe.map -o $@ $(PUBLISHER_CHARSET_PROBE_SOURCES)

uci-absence-probe: $(UCI_ABSENCE_PROBE_TARGET)

$(UCI_ABSENCE_PROBE_TARGET): $(UCI_ABSENCE_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/uci-absence-probe.map -o $@ $(UCI_ABSENCE_PROBE_SOURCES)

network-probe: $(NETWORK_PROBE_TARGET)

$(NETWORK_PROBE_TARGET): $(NETWORK_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/network-probe.map -o $@ $(NETWORK_PROBE_SOURCES)

https-probe: $(HTTPS_PROBE_TARGET)

$(HTTPS_PROBE_TARGET): $(HTTPS_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/https-probe.map -o $@ $(HTTPS_PROBE_SOURCES)

direct-site-probe: $(DIRECT_SITE_PROBE_TARGET)

$(DIRECT_SITE_PROBE_TARGET): $(DIRECT_SITE_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/direct-site-probe.map -o $@ $(DIRECT_SITE_PROBE_SOURCES)

direct-auth-probe: $(DIRECT_AUTH_PROBE_TARGET)

$(DIRECT_AUTH_PROBE_TARGET): $(DIRECT_AUTH_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/direct-auth-probe.map -o $@ $(DIRECT_AUTH_PROBE_SOURCES)

hmac-probe: $(HMAC_PROBE_TARGET)

$(HMAC_PROBE_TARGET): $(HMAC_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/hmac-probe.map -o $@ $(HMAC_PROBE_SOURCES)

platform-preflight-probe: $(PLATFORM_PREFLIGHT_PROBE_TARGET)

$(PLATFORM_PREFLIGHT_PROBE_TARGET): $(PLATFORM_PREFLIGHT_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/platform-preflight-probe.map -o $@ $(PLATFORM_PREFLIGHT_PROBE_SOURCES)

reu-probe: $(REU_PROBE_TARGET)

$(REU_PROBE_TARGET): $(REU_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/reu-probe.map -o $@ $(REU_PROBE_SOURCES)

reu-buffer-probe: $(REU_BUFFER_PROBE_TARGET)

$(REU_BUFFER_PROBE_TARGET): $(REU_BUFFER_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-buffer-probe.map -o $@ $(REU_BUFFER_PROBE_SOURCES)

publisher: $(PUBLISHER_TARGET) $(PUBLISHER_CHAIN_TARGET) $(SETTINGS_TARGET) $(SETTINGS_CHAIN_TARGET)

$(PUBLISHER_TARGET): $(PUBLISHER_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -DS64_DIRECT_PUBLISHER=1 -m build/see64you-publisher.map -o $@ $(PUBLISHER_SOURCES)

$(PUBLISHER_CHAIN_TARGET): $(PUBLISHER_TARGET)
	cp $< $@

settings: $(SETTINGS_TARGET) $(SETTINGS_CHAIN_TARGET)

$(SETTINGS_TARGET): $(SETTINGS_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-settings.map -o $@ $(SETTINGS_SOURCES)

$(SETTINGS_CHAIN_TARGET): $(SETTINGS_TARGET)
	cp $< $@

publisher-companion-fit: $(PUBLISHER_COMPANION_FIT_TARGET)

$(PUBLISHER_COMPANION_FIT_TARGET): $(PUBLISHER_COMPANION_FIT_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -m build/publisher-companion-fit.map -o $@ $(PUBLISHER_COMPANION_FIT_SOURCES)

recovery-probe: $(RECOVERY_PROBE_TARGET)

$(RECOVERY_PROBE_TARGET): $(RECOVERY_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/recovery-probe.map -o $@ $(RECOVERY_PROBE_SOURCES)

reu-editor-probe: $(REU_EDITOR_PROBE_TARGET)

$(REU_EDITOR_PROBE_TARGET): $(REU_EDITOR_PROBE_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/reu-editor-probe.map -o $@ $(REU_EDITOR_PROBE_SOURCES)

clean:
	rm -rf build
