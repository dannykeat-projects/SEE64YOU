TARGET := build/see64you.prg
C64U_TARGET := build/see64you-c64u.prg
C64U_CHAIN_TARGET := build/SEE64YOU-C64U.PRG
C64U_SHORT_CHAIN_TARGET := build/S64EDIT.PRG
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
REU_PROBE_TARGET := build/reu-probe.prg
REU_BUFFER_PROBE_TARGET := build/reu-buffer-probe.prg
REU_EDITOR_PROBE_TARGET := build/reu-editor-probe.prg
RECOVERY_PROBE_TARGET := build/recovery-probe.prg
PUBLISHER_COMPANION_FIT_TARGET := build/publisher-companion-fit.prg
PUBLISHER_TARGET := build/see64you-publisher.prg
PUBLISHER_CHAIN_TARGET := build/S64PUB.PRG
SETTINGS_TARGET := build/see64you-settings.prg
SETTINGS_CHAIN_TARGET := build/S64CFG.PRG
SOURCES := src/main.c src/charset.c src/buffer.c src/history.c src/clipboard.c src/document.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s src/input/input.c src/input/repeat.s src/platform/uci.c src/platform/dos.c src/storage/local.c
C64U_SOURCES := $(filter-out src/buffer.c src/history.c,$(SOURCES)) src/buffer_reu.c src/history_reu.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/editor_handoff.c src/publisher/editor_handoff_record.s
EDITOR_PROBE_SOURCES := diagnostics/editor_probe.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c
UI_PROBE_SOURCES := diagnostics/ui_probe.c diagnostics/ui_conio_shim.c src/buffer.c src/history.c src/clipboard.c src/editor.c src/editor_format.c src/ui.c src/ui_status.s
INPUT_REPEAT_PROBE_SOURCES := diagnostics/input_repeat_probe.c src/input/repeat.s
PUBLISHER_CHARSET_PROBE_SOURCES := diagnostics/publisher_charset_probe.c src/buffer.c src/document.c src/charset.c
UCI_ABSENCE_PROBE_SOURCES := diagnostics/uci_absence_probe.c src/platform/uci.c src/charset.c
NETWORK_PROBE_SOURCES := diagnostics/network_probe.c src/platform/uci.c src/platform/network.c src/charset.c
HTTPS_PROBE_SOURCES := diagnostics/https_probe.c src/platform/uci.c src/platform/http.c src/charset.c
DIRECT_SITE_PROBE_SOURCES := diagnostics/direct_site_probe.c src/platform/uci.c src/platform/network.c src/charset.c
DIRECT_AUTH_PROBE_SOURCES := diagnostics/direct_auth_probe.c src/charset.c src/crypto/sha256.c src/platform/uci.c src/platform/network.c src/platform/dos.c src/platform/turbo.c src/platform/reu.c src/platform/reu_dma.s src/publisher/direct_auth.c src/publisher/settings_exchange.c src/publisher/settings_store.c
HMAC_PROBE_SOURCES := diagnostics/hmac_probe.c src/crypto/sha256.c
SETTINGS_IO_PROBE_SOURCES := diagnostics/settings_io_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/publisher/settings_exchange.c src/platform/reu.c src/platform/reu_dma.s
SETTINGS_EXCHANGE_PROBE_SOURCES := diagnostics/settings_exchange_probe.c src/charset.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/publisher/settings_store.c
REU_PROBE_SOURCES := diagnostics/reu_probe.c src/platform/reu.c src/platform/reu_dma.s
REU_BUFFER_PROBE_SOURCES := diagnostics/reu_buffer_probe.c src/buffer_reu.c src/platform/reu.c src/platform/reu_dma.s
REU_EDITOR_PROBE_SOURCES := diagnostics/reu_editor_probe.c src/buffer_reu.c src/history_reu.c src/clipboard.c src/editor.c src/platform/reu.c src/platform/reu_dma.s
RECOVERY_PROBE_SOURCES := diagnostics/recovery_probe.c src/buffer.c src/document.c src/charset.c src/platform/dos.c src/storage/local.c
PUBLISHER_CORE_SOURCES := src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/http.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/publisher/destination.c src/publisher/state.c src/publisher/workspace.c src/publisher/handoff.c src/publisher/plaintext.c src/publisher/preview.c src/publisher/relay.c src/publisher/settings_bridge.c src/publisher/standalone.c
PUBLISHER_DOCUMENT_SOURCES := src/publisher/standalone_documents.c
PUBLISHER_COMPANION_FIT_SOURCES := diagnostics/publisher_companion_fit.c $(PUBLISHER_CORE_SOURCES) $(PUBLISHER_DOCUMENT_SOURCES)
PUBLISHER_SOURCES := src/publisher/main.c src/platform/chain_editor.s $(PUBLISHER_CORE_SOURCES) $(PUBLISHER_DOCUMENT_SOURCES)
SETTINGS_SOURCES := src/publisher/settings_main.c src/charset.c src/buffer_reu.c src/document.c src/platform/uci.c src/platform/dos.c src/platform/reu.c src/platform/reu_dma.s src/platform/chain.s src/publisher/destination.c src/publisher/settings_exchange.c src/publisher/settings_store.c

CL65 ?= cl65
# Keep the active conventional-C64 editor inside base RAM. Basic formatting is
# linked in editor_format.c. The larger URL/link formatter remains host/module-tested
# but unlinked because it does not fit while retaining the 1 KiB software stack.
CFLAGS := -t c64 -Os -I src -DS64_NATIVE_PETSCII=1 -Wl -D,__STACKSIZE__=0x0400

.PHONY: all c64u publisher settings editor-probe ui-probe input-repeat-probe publisher-charset-probe uci-absence-probe network-probe https-probe direct-site-probe direct-auth-probe hmac-probe reu-probe reu-buffer-probe reu-editor-probe recovery-probe publisher-companion-fit clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p build
	$(CL65) $(CFLAGS) -m build/see64you.map -o $@ $(SOURCES)

c64u: $(C64U_TARGET) $(C64U_CHAIN_TARGET) $(C64U_SHORT_CHAIN_TARGET) $(SETTINGS_IO_PROBE_TARGET) $(SETTINGS_EXCHANGE_PROBE_TARGET)

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
	$(CL65) $(CFLAGS) -DS64_REU_BUFFER=1 -DS64_STANDALONE_ONLY=1 -m build/see64you-publisher.map -o $@ $(PUBLISHER_SOURCES)

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
