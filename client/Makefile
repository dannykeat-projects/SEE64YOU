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
MANAGED_SETTINGS_TARGET := build/managed-settings.prg
INDEPENDENT_SETTINGS_TARGET := build/independent/S64CFG.PRG
INDEPENDENT_PUBLISHER_TARGET := build/independent/S64PUB.PRG
INDEPENDENT_EDITOR_TARGET := build/independent/SEE64YOU.PRG
MODULE_MANIFEST_TARGET := build/S64MODULE.MNF
SOURCES := src/main.c src/ui_exit.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/menu.c src/input/input.c src/input/repeat.s src/platform/uci.c src/platform/dos.c src/storage/local.c
C64U_SOURCES := $(filter-out src/buffer.c src/history.c,$(SOURCES)) src/buffer_reu.c src/history_reu.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/editor_handoff.c src/publisher/editor_handoff_record.s
# Destinations validate existing state; allocation and manifest parsing stay in the launcher.
MANAGED_EDITOR_RUNTIME_SOURCES := src/platform/code_overlay_bootstrap.c src/platform/module_arrival.c src/platform/module_runtime.c src/platform/transfer_capsule.c src/platform/module_session.c src/platform/module_session_read.c src/platform/module_manifest.c src/platform/reu_runtime.c src/platform/session_lifecycle.c src/platform/session_journal.c src/platform/system_state.c src/platform/turbo.c
MANAGED_SETTINGS_RUNTIME_SOURCES := $(filter-out src/platform/code_overlay_bootstrap.c,$(MANAGED_EDITOR_RUNTIME_SOURCES)) src/platform/transfer_capsule_write.c src/platform/module_transition.c src/platform/module_transfer.c src/platform/module_transfer_stub.s src/publisher/settings_session.c
# Settings config validation needs no document storage or publishing URL builders.
MANAGED_SETTINGS_SOURCES = $(filter-out src/platform/chain.s src/buffer_reu.c src/document.c,$(SETTINGS_SOURCES))
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
DIRECT_AUTH_PROBE_SOURCES := diagnostics/direct_auth_probe.c src/charset.c src/crypto/sha256.c src/platform/uci.c src/platform/network.c src/platform/dos.c src/platform/system_state.c src/platform/system_state_start.c src/platform/turbo.c src/platform/turbo_capture.c src/platform/reu.c src/platform/reu_dma.s src/publisher/direct_key.c src/publisher/direct_auth.c src/publisher/settings_exchange.c src/publisher/settings_validate.c src/publisher/settings_store.c
HMAC_PROBE_SOURCES := diagnostics/hmac_probe.c src/crypto/sha256.c
SETTINGS_IO_PROBE_SOURCES := diagnostics/settings_io_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/publisher/settings_exchange.c src/publisher/settings_validate.c src/platform/reu.c src/platform/reu_dma.s
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
PUBLISHER_SOURCES := src/publisher/main.c src/platform/chain_editor.s $(filter-out src/platform/http.c src/publisher/relay.c src/publisher/standalone.c,$(PUBLISHER_CORE_SOURCES)) src/platform/network.c src/crypto/sha256.c src/publisher/direct_key.c src/publisher/direct_key_store.c src/publisher/direct_auth.c src/publisher/standalone_direct.c
STANDARD_KEY_SOURCES := src/publisher/standard_keyfile.c src/publisher/standard_publish_names.c src/publisher/direct_key.c
STANDARD_PROTOCOL_SOURCES := src/publisher/standard_transport.c src/publisher/standard_publish.c src/publisher/standard_reply.c src/publisher/standard_request.c src/publisher/direct_auth.c src/crypto/sha256.c
INDEPENDENT_PUBLISHER_SOURCES := src/publisher/standard_main.c src/platform/dos_path.c src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/platform/network.c src/storage/local.c src/publisher/settings_exchange.c src/publisher/settings_validate.c src/publisher/settings_store.c src/platform/basic_workspace_reset.s src/publisher/standard_publish_names.c src/publisher/direct_key.c $(STANDARD_PROTOCOL_SOURCES) src/platform/dos_dir.c src/publisher/local_picker.c
INDEPENDENT_SETTINGS_MIGRATION_SOURCES := src/platform/journal_migration.c src/platform/session_journal.c src/platform/dos_file.c src/platform/system_state.c src/platform/system_state_start.c src/platform/turbo.c src/platform/turbo_capture.c
INDEPENDENT_EDITOR_SOURCES := $(filter-out src/platform/chain.s src/publisher/editor_handoff.c src/publisher/editor_handoff_record.s,$(C64U_SOURCES))
SETTINGS_SOURCES := src/publisher/settings_main.c src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/destination.c src/publisher/settings_exchange.c src/publisher/settings_validate.c src/publisher/settings_store.c src/platform/dos_path.c src/publisher/standard_key_store.c $(STANDARD_KEY_SOURCES)
LAUNCHER_SOURCES := src/platform/module_session.c src/platform/module_session_read.c src/platform/module_session_build.c src/platform/transfer_capsule.c src/platform/transfer_capsule_build.c src/platform/transfer_capsule_write.c src/platform/module_transfer.c src/platform/module_arrival.c src/platform/module_transfer_stub.s src/launcher/main.c src/platform/module_integrity.c src/platform/crc32.c src/platform/module_preload.c src/platform/dos_reu.c src/platform/dos_file.c src/launcher/recovery_runtime.c src/launcher/recovery_flow.c src/platform/preflight.c src/platform/requirements.c src/platform/module_manifest.c src/platform/module_manifest_build.c src/platform/module_manifest_parse.c src/platform/reu_runtime.c src/platform/reu_runtime_build.c src/platform/reu_runtime_allocate.c src/platform/reu_runtime_write.c src/platform/session_lifecycle.c src/platform/session_lifecycle_start.c src/platform/session_recovery.c src/platform/session_journal.c src/platform/session_journal_start.c src/platform/system_state.c src/platform/system_state_start.c src/platform/dos.c src/platform/turbo.c src/platform/turbo_capture.c src/platform/uci.c src/platform/reu.c src/platform/reu_dma.s src/charset.c

CL65 ?= cl65
CA65 ?= ca65
LD65 ?= ld65
STORAGE_ROOT ?= /sd/
ifneq ($(words $(STORAGE_ROOT)),1)
$(error STORAGE_ROOT must be one absolute mount path ending in /)
endif
ifneq ($(filter /%/,$(STORAGE_ROOT)),$(STORAGE_ROOT))
$(error STORAGE_ROOT must be an absolute mount path ending in /, for example /Usb0/)
endif
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400 '-DS64_STORAGE_ROOT_DIRECTORY="$(STORAGE_ROOT)"'

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

.PHONY: dos-context-probe
dos-context-probe: build/dos-context-probe.prg

build/dos-context-probe.prg: diagnostics/dos_context_probe.c src/platform/dos.c src/platform/uci.c src/charset.c src/platform/package_paths.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -o $@ diagnostics/dos_context_probe.c src/platform/dos.c src/platform/uci.c src/charset.c

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

$(SETTINGS_EXCHANGE_PROBE_TARGET): $(SETTINGS_EXCHANGE_PROBE_SOURCES) src/publisher/settings_exchange.c src/publisher/settings_validate.c
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/settings-exchange-probe.map -o $@ $(SETTINGS_EXCHANGE_PROBE_SOURCES)

$(C64U_TARGET): $(C64U_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-c64u.map -o $@ $(C64U_SOURCES)

# Direct, file-based Editor: no Publisher handoff or chaining.
.PHONY: independent-editor
independent-editor: $(INDEPENDENT_EDITOR_TARGET)

$(INDEPENDENT_EDITOR_TARGET): $(INDEPENDENT_EDITOR_SOURCES)
	@mkdir -p build/independent
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_INDEPENDENT_EDITOR=1 -m build/independent/editor.map -o $@ $(INDEPENDENT_EDITOR_SOURCES)
	python3 ../tools/check_memory_headroom.py build/independent/editor.map --label independent-editor --min-headroom 1024

$(C64U_CHAIN_TARGET): $(C64U_TARGET)
	cp $< $@

$(C64U_SHORT_CHAIN_TARGET): $(C64U_TARGET)
	cp $< $@

managed-editor-fit: $(MANAGED_EDITOR_FIT_TARGET)

.PHONY: editor-overlay-fit
editor-overlay-fit:
	python3 ../tools/build_editor_overlay_fit.py

# Existing Editor launcher/overlay bootstrap, with independent Publisher action.
# Publisher and Settings still launch as their own PRGs, never as Editor handoffs.
.PHONY: independent-editor-overlay-fit
independent-editor-overlay-fit:
	python3 ../tools/build_editor_overlay_fit.py --independent-editor

INDEPENDENT_PACKAGE_DIR := build/independent-package/SEE64YOU
INDEPENDENT_BROWSER_SOURCES := src/browser/main.c src/browser/protocol.c src/browser/http.c src/charset.c src/platform/uci.c src/platform/network.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/platform/reu_runtime.c src/platform/reu_runtime_build.c src/platform/reu_runtime_allocate.c src/platform/reu_runtime_write.c src/publisher/settings_exchange.c src/publisher/settings_validate.c src/publisher/settings_store.c src/platform/basic_workspace_reset.s

.PHONY: browser-palette-native-test
browser-palette-native-test:
	@mkdir -p build/browser-native-vice
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -o build/browser-native-vice/PALTEST.PRG tests/browser_palette_native_test.c $(filter-out src/browser/main.c,$(INDEPENDENT_BROWSER_SOURCES))
.PHONY: independent-browser
independent-browser: build/independent/S64WEB.PRG

.PHONY: storage-context-probe
storage-context-probe: build/storage-context-probe.prg
build/storage-context-probe.prg: diagnostics/storage_context_probe.c src/platform/dos.c src/platform/uci.c src/charset.c src/platform/basic_workspace_reset.s
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/storage-context-probe.map -o $@ $^

build/independent/S64WEB.PRG: $(INDEPENDENT_BROWSER_SOURCES) src/browser/protocol.h src/browser/http.h
	@mkdir -p build/independent
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/independent/browser.map -o $@ $(INDEPENDENT_BROWSER_SOURCES)
	python3 ../tools/check_memory_headroom.py build/independent/browser.map --label independent-browser --min-headroom 1024

.PHONY: independent-package
independent-package: independent-editor-overlay-fit independent-publisher independent-settings build/managed-handoff-launcher.prg
	python3 ../tools/check_memory_headroom.py build/managed-handoff-launcher.map --label independent-launcher --min-headroom 1024
	@mkdir -p $(INDEPENDENT_PACKAGE_DIR)/SYSTEM
	$(RM) $(INDEPENDENT_PACKAGE_DIR)/S64WEB.PRG $(addprefix $(INDEPENDENT_PACKAGE_DIR)/,S64MODEL.BIN S64FILE.BIN S64RUN.BIN S64PREP.BIN S64OVL.MNF S64MODULE.MNF)
	cp build/managed-handoff-launcher.prg $(INDEPENDENT_PACKAGE_DIR)/SEE64YOU.PRG
	cp build/editor-overlay-fit/editor-overlay-fit.prg $(INDEPENDENT_PACKAGE_DIR)/S64EDIT.PRG
	cp $(INDEPENDENT_PUBLISHER_TARGET) $(INDEPENDENT_PACKAGE_DIR)/S64PUB.PRG
	cp $(INDEPENDENT_SETTINGS_TARGET) $(INDEPENDENT_PACKAGE_DIR)/S64CFG.PRG
	cp build/editor-overlay-fit/S64MODEL.BIN build/editor-overlay-fit/S64FILE.BIN build/editor-overlay-fit/S64RUN.BIN build/editor-overlay-fit/S64PREP.BIN build/editor-overlay-fit/S64OVL.MNF $(INDEPENDENT_PACKAGE_DIR)/SYSTEM/
	python3 ../tools/generate_module_manifest.py $(INDEPENDENT_PACKAGE_DIR) --output $(INDEPENDENT_PACKAGE_DIR)/SYSTEM/S64MODULE.MNF
	python3 ../tools/generate_module_manifest.py $(INDEPENDENT_PACKAGE_DIR) --validate $(INDEPENDENT_PACKAGE_DIR)/SYSTEM/S64MODULE.MNF
	python3 ../tools/generate_code_asset_manifest.py $(INDEPENDENT_PACKAGE_DIR)/SYSTEM --root $(INDEPENDENT_PACKAGE_DIR)/S64EDIT.PRG --validate $(INDEPENDENT_PACKAGE_DIR)/SYSTEM/S64OVL.MNF

# Reviewable preload package only. Its launcher stops before executable DMA.
.PHONY: managed-package
managed-package: editor-overlay-fit publisher-overlay-fit code-assets-fit publisher settings managed-settings
	python3 ../tools/check_memory_headroom.py build/code-assets-fit.map --label managed-launcher --min-headroom 1024
	@mkdir -p build/managed-package/SYSTEM
	$(RM) $(addprefix build/managed-package/,S64MODEL.BIN S64FILE.BIN S64RUN.BIN S64PREP.BIN S64OVL.MNF S64MODULE.MNF S64PCNT.BIN S64PNET.BIN S64PRUN.BIN S64POVL.MNF)
	cp build/code-assets-fit.prg build/managed-package/SEE64YOU.PRG
	cp build/editor-overlay-fit/editor-overlay-fit.prg build/managed-package/S64EDIT.PRG
	cp build/publisher-overlay-fit/publisher-overlay-fit.prg build/managed-package/S64PUB.PRG
	cp $(MANAGED_SETTINGS_TARGET) build/managed-package/S64CFG.PRG
	cp build/editor-overlay-fit/S64MODEL.BIN build/editor-overlay-fit/S64FILE.BIN build/editor-overlay-fit/S64RUN.BIN build/editor-overlay-fit/S64PREP.BIN build/editor-overlay-fit/S64OVL.MNF build/managed-package/SYSTEM/
	cp build/publisher-overlay-fit/S64PCNT.BIN build/publisher-overlay-fit/S64PNET.BIN build/publisher-overlay-fit/S64PRUN.BIN build/publisher-overlay-fit/S64POVL.MNF build/managed-package/SYSTEM/
	python3 ../tools/generate_code_asset_manifest.py build/managed-package/SYSTEM --root build/managed-package/S64EDIT.PRG --validate build/managed-package/SYSTEM/S64OVL.MNF
	python3 ../tools/generate_code_asset_manifest.py build/managed-package/SYSTEM --module publisher --root build/managed-package/S64PUB.PRG --validate build/managed-package/SYSTEM/S64POVL.MNF
	python3 ../tools/generate_module_manifest.py build/managed-package --output build/managed-package/SYSTEM/S64MODULE.MNF
	python3 ../tools/generate_module_manifest.py build/managed-package --validate build/managed-package/SYSTEM/S64MODULE.MNF

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

# Current supported package: verified launcher plus managed Editor/Publisher/Settings
# destinations. Real-hardware module-cycle acceptance remains separately gated.
C64U_PACKAGE_FILES := SEE64YOU.PRG S64EDIT.PRG S64PUB.PRG S64CFG.PRG S64MODULE.MNF S64OVL.MNF S64MODEL.BIN S64FILE.BIN S64RUN.BIN S64PREP.BIN S64POVL.MNF S64PCNT.BIN S64PNET.BIN S64PRUN.BIN
C64U_PACKAGE_DIR := build/c64u-package/SEE64YOU
.PHONY: c64u-package
c64u-package: managed-handoff-package
	rm -rf build/c64u-package
	@mkdir -p $(C64U_PACKAGE_DIR)
	cp $(addprefix build/managed-package/,$(C64U_PACKAGE_FILES)) $(C64U_PACKAGE_DIR)/
	python3 ../tools/generate_module_manifest.py $(C64U_PACKAGE_DIR) --validate $(C64U_PACKAGE_DIR)/S64MODULE.MNF
	python3 ../tools/generate_code_asset_manifest.py $(C64U_PACKAGE_DIR) --root $(C64U_PACKAGE_DIR)/S64EDIT.PRG --validate $(C64U_PACKAGE_DIR)/S64OVL.MNF
	python3 ../tools/generate_code_asset_manifest.py $(C64U_PACKAGE_DIR) --module publisher --root $(C64U_PACKAGE_DIR)/S64PUB.PRG --validate $(C64U_PACKAGE_DIR)/S64POVL.MNF
	python3 ../tools/check_memory_headroom.py build/see64you-publisher.map --label publisher --min-headroom 1024
	python3 ../tools/check_memory_headroom.py build/see64you-settings.map --label settings --min-headroom 1024
	python3 ../tools/check_memory_headroom.py build/managed-settings.map --label managed-settings --min-headroom 1024

build/managed-handoff-launcher.prg: $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/package_version.h src/platform/basic_return_capture.s src/platform/basic_return.c src/platform/basic_return_build.c src/launcher/basic_return_runtime.c src/platform/basic_workspace_reset.s
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_CODE_ASSET_LAUNCHER=1 -DS64_EXECUTABLE_HANDOFF=1 -m build/managed-handoff-launcher.map -o $@ src/platform/basic_return_capture.s $(LAUNCHER_SOURCES) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/basic_return.c src/platform/basic_return_build.c src/launcher/basic_return_runtime.c src/platform/basic_workspace_reset.s

# Diagnostic only; the exact ordinary c64u-package Editor remains the destination.
.PHONY: resume-editor-gate
resume-editor-gate: c64u-package
	python3 ../tools/build_resume_editor_fixture.py --output build/resume-editor-fixture
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -I build/resume-editor-fixture -m build/resume-editor-gate.map -o build/resume-editor-gate.prg src/platform/basic_return_capture.s diagnostics/resume_editor_gate.c diagnostics/resume_editor_fixture.c $(filter-out src/launcher/main.c,$(LAUNCHER_SOURCES)) $(CODE_ASSET_SOURCES) src/launcher/code_assets_runtime.c src/platform/basic_return.c src/platform/basic_return_build.c src/launcher/basic_return_runtime.c src/platform/application_state.c
	python3 ../tools/check_memory_headroom.py build/resume-editor-gate.map --label resume-editor-gate --min-headroom 1024
	python3 ../tools/check_prg_extent.py build/resume-editor-gate.prg --label resume-editor-gate --load-address 0x0801 --max-end 0xC7FF

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

# Measured MVP candidate; no handoff or PRG chaining between programs.
.PHONY: independent-publisher
independent-publisher: $(INDEPENDENT_PUBLISHER_TARGET)

$(INDEPENDENT_PUBLISHER_TARGET): $(INDEPENDENT_PUBLISHER_SOURCES)
	@mkdir -p build/independent
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -DS64_DIRECT_PUBLISHER=1 -DS64_INDEPENDENT_PUBLISHER=1 -DS64_STORAGE_READ_ONLY=1 -m build/independent/publisher.map -o $@ $(INDEPENDENT_PUBLISHER_SOURCES)
	python3 ../tools/check_memory_headroom.py build/independent/publisher.map --label independent-publisher --min-headroom 1024

$(PUBLISHER_TARGET): $(PUBLISHER_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -DS64_DIRECT_PUBLISHER=1 -m build/see64you-publisher.map -o $@ $(PUBLISHER_SOURCES)

$(PUBLISHER_CHAIN_TARGET): $(PUBLISHER_TARGET)
	cp $< $@

settings: $(SETTINGS_TARGET) $(SETTINGS_CHAIN_TARGET)

# Candidate only: standalone Settings with authoritative disk configuration and
# ordinary BASIC return. The managed package remains untouched until acceptance.
.PHONY: independent-settings
independent-settings: $(INDEPENDENT_SETTINGS_TARGET)

# Fit-only measurement for Settings-owned upgrade service; not a product PRG.
.PHONY: journal-migration-settings-fit
journal-migration-settings-fit:
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_INDEPENDENT_SETTINGS=1 -m build/journal-migration-settings-fit.map -o build/journal-migration-settings-fit.prg $(filter-out src/platform/chain.s,$(SETTINGS_SOURCES)) src/platform/basic_workspace_reset.s src/platform/journal_migration.c src/platform/session_journal.c src/platform/dos_file.c
	python3 ../tools/check_memory_headroom.py build/journal-migration-settings-fit.map --label journal-migration-settings --min-headroom 1024

.PHONY: journal-migration-probe
journal-migration-probe:
	@mkdir -p build
	$(CL65) $(CFLAGS) -o build/journal-migration-probe.prg diagnostics/journal_migration_probe.c src/platform/journal_migration.c src/platform/session_journal.c src/platform/session_journal_start.c src/platform/dos.c src/platform/dos_file.c src/platform/uci.c src/platform/turbo.c src/platform/turbo_capture.c src/charset.c
	$(CL65) $(CFLAGS) -DS64_MIGRATION_PROBE_SEED=0 -o build/journal-migration-check.prg diagnostics/journal_migration_probe.c src/platform/journal_migration.c src/platform/session_journal.c src/platform/session_journal_start.c src/platform/dos.c src/platform/dos_file.c src/platform/uci.c src/platform/turbo.c src/platform/turbo_capture.c src/charset.c
	$(CL65) $(CFLAGS) -DS64_MIGRATION_PROBE_SEED=0 -DS64_MIGRATION_PROBE_RETIRE=1 -o build/journal-migration-retire.prg diagnostics/journal_migration_probe.c src/platform/journal_migration.c src/platform/session_journal.c src/platform/session_journal_start.c src/platform/dos.c src/platform/dos_file.c src/platform/uci.c src/platform/turbo.c src/platform/turbo_capture.c src/charset.c

$(INDEPENDENT_SETTINGS_TARGET): $(filter-out src/platform/chain.s,$(SETTINGS_SOURCES)) src/platform/basic_workspace_reset.s $(INDEPENDENT_SETTINGS_MIGRATION_SOURCES)
	@mkdir -p build/independent
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_INDEPENDENT_SETTINGS=1 -m build/independent/settings.map -o $@ $(filter-out src/platform/chain.s,$(SETTINGS_SOURCES)) src/platform/basic_workspace_reset.s $(INDEPENDENT_SETTINGS_MIGRATION_SOURCES)
	python3 ../tools/check_memory_headroom.py build/independent/settings.map --label independent-settings --min-headroom 1024

$(SETTINGS_TARGET): $(SETTINGS_SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -m build/see64you-settings.map -o $@ $(SETTINGS_SOURCES)

$(SETTINGS_CHAIN_TARGET): $(SETTINGS_TARGET)
	cp $< $@

.PHONY: managed-settings
managed-settings: $(MANAGED_SETTINGS_TARGET)

$(MANAGED_SETTINGS_TARGET): $(MANAGED_SETTINGS_SOURCES) $(MANAGED_SETTINGS_RUNTIME_SOURCES) src/platform/transfer_capsule_internal.h src/platform/module_session_internal.h
	@mkdir -p build
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_MANAGED_OVERLAY=1 -DS64_SETTINGS_CONFIG_ONLY=1 -m build/managed-settings.map -o $@ $(MANAGED_SETTINGS_SOURCES) $(MANAGED_SETTINGS_RUNTIME_SOURCES)
	python3 ../tools/check_memory_headroom.py build/managed-settings.map --label managed-settings --min-headroom 1024

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
