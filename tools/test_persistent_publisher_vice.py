#!/usr/bin/env python3
"""Execute the first real Publisher slice under one persistent runtime in VICE."""
from __future__ import annotations

import argparse
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
import build_editor_overlay_fit as fit

ROOT = Path(__file__).resolve().parents[1]
CLIENT = ROOT / "client"
OUT = CLIENT / "build" / "persistent-publisher-vice"

GROUPS = {
    "PCONTENT": ("src/publisher/destination.c", "src/publisher/state.c"),
    "PNETWORK": ("src/platform/network.c", "src/crypto/sha256.c", "src/publisher/direct_auth.c", "src/publisher/standalone_direct.c"),
    "PRUNTIME": ("src/publisher/settings_validate.c", "src/publisher/settings_exchange.c"),
}
GROUP_IDS = {"EDITOR": 1, "PCONTENT": 1, "PNETWORK": 2, "PRUNTIME": 3, "SETTINGS": 1}

EDITOR = r"""
#include <stdint.h>
#pragma code-name ("EDITOR_CODE")
#pragma rodata-name ("EDITOR_RODATA")
uint16_t __fastcall__ s64_overlay_editor_s64_probe_editor(uint16_t value)
{ return (uint16_t)(value ^ 0x64a5u); }
"""

SETTINGS = r"""
#include <stdint.h>
#pragma code-name ("SETTINGS_CODE")
#pragma rodata-name ("SETTINGS_RODATA")
uint16_t __fastcall__ s64_overlay_settings_s64_probe_settings(uint16_t value)
{ return (uint16_t)(value + 0x0064u); }
"""

PNETWORK_PING = r"""
#include <stdint.h>
#pragma code-name ("PNETWORK_CODE")
#pragma rodata-name ("PNETWORK_RODATA")
uint16_t __fastcall__ s64_overlay_pnetwork_s64_probe_pnetwork_ping(uint16_t value)
{ return (uint16_t)(value ^ 0x5aa5u); }
"""

PRUNTIME_PING = r"""
#include <stdint.h>
#pragma code-name ("PRUNTIME_CODE")
#pragma rodata-name ("PRUNTIME_RODATA")
uint16_t __fastcall__ s64_overlay_pruntime_s64_probe_pruntime_ping(uint16_t value)
{ return (uint16_t)(value ^ 0xa55au); }
"""

MAIN = r"""
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "document.h"
#include "platform/code_overlay.h"
#include "platform/code_overlay_namespace.h"
#include "platform/network.h"
#include "platform/reu.h"
#include "platform/reu_runtime.h"
#include "publisher/direct_auth.h"
#include "publisher/standalone_direct.h"
#include "publisher/persistent_mode.h"
#include "publisher/settings_exchange.h"

extern const uint8_t editor_image[], editor_image_end[];
extern const uint8_t pcontent_image[], pcontent_image_end[];
extern const uint8_t pnetwork_image[], pnetwork_image_end[];
extern const uint8_t pruntime_image[], pruntime_image_end[];
extern const uint8_t settings_image[], settings_image_end[];

extern uint16_t __fastcall__ s64_probe_editor(uint16_t);
extern uint16_t __fastcall__ s64_probe_settings(uint16_t);
extern uint16_t __fastcall__ s64_probe_pnetwork_ping(uint16_t);
extern uint16_t __fastcall__ s64_probe_pruntime_ping(uint16_t);

static S64ReuRuntimeControl control;
static S64ReuAllocation assets[5];
static S64CodeOverlayNamespace editor_namespace;
static S64CodeOverlayNamespace publisher_namespace;
static S64CodeOverlayNamespace settings_namespace;
static uint16_t overlay_reads;

static uint8_t key[32];
static uint8_t body_digest[32];
static char signature[65];
static char url[S64_DESTINATION_URL_CAPACITY];
static S64NetTransport network;
static uint8_t network_handle;
static uint8_t network_payload[4];
static uint16_t network_payload_length;
static uint16_t network_received;
static S64HttpTransport direct_http;
static uint8_t fake_http_open_count;
static S64StandaloneResult direct_result;
static S64Buffer body;
static S64Document document;
static S64PersistentPublisher publisher;
static S64SettingsExchange settings;
static S64SettingsExchange loaded;
static uint8_t cycle;
static uint16_t before;

static uint8_t overlay_reader(uint8_t bank, uint16_t offset, void* target, uint16_t length)
{
    ++overlay_reads;
    return s64_reu_load(bank, offset, target, length);
}

static void require(uint8_t passed, uint8_t step)
{
    *(volatile uint8_t*)0xc060 = step;
    if (!passed) {
        *(volatile uint8_t*)0xc062 = step;
        *(volatile uint8_t*)0xc061 = 0x5a;
        for (;;) {}
    }
}

static uint8_t allocate_asset(
    uint8_t index, const uint8_t* first, const uint8_t* last, uint8_t flags)
{
    uint16_t length = (uint16_t)(last - first);
    uint8_t slot;
    if (!length || length > S64_CODE_OVERLAY_CAPACITY
        || !s64_reu_runtime_allocate(
            &control, S64_REU_OWNER_MODULE, length, 256u, &assets[index])) {
        return 0u;
    }
    slot = (uint8_t)(control.allocation_count - 1u);
    assets[index].flags = flags;
    control.allocations[slot].flags = flags;
    return s64_reu_store(
        (uint8_t)(assets[index].address >> 16),
        (uint16_t)assets[index].address,
        first,
        length
    );
}

static uint8_t hex64(const char* value)
{
    uint8_t i;
    if (!value || value[64] != 0) return 0u;
    for (i = 0u; i < 64u; ++i) {
        char c = value[i];
        if (!((c >= '0' && c <= '9')
            || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F'))) return 0u;
    }
    return 1u;
}

static uint8_t capture_network_payload(
    const uint8_t* data, uint16_t length, void* context)
{
    uint16_t i;
    (void)context;
    if (network_payload_length + length > sizeof(network_payload)) return 0u;
    for (i = 0u; i < length; ++i) {
        network_payload[network_payload_length++] = data[i];
    }
    return 1u;
}

static S64UciResult fake_network_uci(
    const S64UciCommandPart* parts, uint8_t part_count, S64UciReply* reply)
{
    uint8_t command;
    uint8_t data[4];
    uint16_t sent;

    if (!parts || !part_count || !reply || !parts[0].data
        || parts[0].length < 2u || parts[0].data[0] != 0x03u
        || !reply->status || reply->status_capacity < 3u) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    command = parts[0].data[1];
    reply->status[0] = '0';
    reply->status[1] = '0';
    reply->status[2] = 0;
    reply->status_length = 2u;

    if (command == 0x07u) {
        data[0] = 0x2au;
        if (!reply->data_sink
            || !reply->data_sink(data, 1u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x11u) {
        if (part_count != 2u || !reply->data_sink) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        sent = parts[1].length;
        data[0] = (uint8_t)sent;
        data[1] = (uint8_t)(sent >> 8);
        if (!reply->data_sink(data, 2u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x10u) {
        data[0] = 2u;
        data[1] = 0u;
        data[2] = 'O';
        data[3] = 'K';
        if (!reply->data_sink
            || !reply->data_sink(data, 4u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x09u) return S64_UCI_OK;
    return S64_UCI_PROTOCOL_ERROR;
}

static S64UciResult fake_http_uci(
    const S64UciCommandPart* parts, uint8_t part_count, S64UciReply* reply)
{
    static const uint8_t challenge_response[] =
        "\x48\x54\x54\x50\x2f\x31\x2e\x30\x20\x32\x30\x30\x20\x4f\x4b"
        "\x0d\x0a\x43\x6f\x6e\x74\x65\x6e\x74\x2d\x4c\x65\x6e\x67\x74"
        "\x68\x3a\x20\x34\x38\x0d\x0a\x0d\x0a\x7b\x22\x63\x68\x61\x6c"
        "\x6c\x65\x6e\x67\x65\x22\x3a\x22\x30\x31\x32\x33\x34\x35\x36"
        "\x37\x38\x39\x61\x62\x63\x64\x65\x66\x30\x31\x32\x33\x34\x35"
        "\x36\x37\x38\x39\x61\x62\x63\x64\x65\x66\x22\x7d";
    static const uint8_t ack_response[] =
        "\x48\x54\x54\x50\x2f\x31\x2e\x30\x20\x32\x30\x30\x20\x4f\x4b"
        "\x0d\x0a\x43\x6f\x6e\x74\x65\x6e\x74\x2d\x4c\x65\x6e\x67\x74"
        "\x68\x3a\x20\x31\x31\x0d\x0a\x0d\x0a\x7b\x22\x6f\x6b\x22\x3a"
        "\x74\x72\x75\x65\x7d";
    const uint8_t* response;
    uint16_t response_length;
    uint16_t sent;
    uint8_t count[2];
    uint8_t handle = 0x31u;
    uint8_t command;

    if (!parts || !part_count || !reply || !parts[0].data
        || parts[0].length < 2u || parts[0].data[0] != 0x03u
        || !reply->status || reply->status_capacity < 3u) {
        return S64_UCI_PROTOCOL_ERROR;
    }

    command = parts[0].data[1];
    reply->status[0] = '0';
    reply->status[1] = '0';
    reply->status[2] = 0;
    reply->status_length = 2u;

    if (command == 0x07u) {
        ++fake_http_open_count;
        if (!reply->data_sink
            || !reply->data_sink(&handle, 1u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x11u) {
        if (part_count != 2u || !reply->data_sink) {
            return S64_UCI_PROTOCOL_ERROR;
        }
        sent = parts[1].length;
        count[0] = (uint8_t)sent;
        count[1] = (uint8_t)(sent >> 8);
        if (!reply->data_sink(count, 2u, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x10u) {
        if (!reply->data_sink) return S64_UCI_PROTOCOL_ERROR;
        if (fake_http_open_count & 1u) {
            response = challenge_response;
            response_length = (uint16_t)(sizeof(challenge_response) - 1u);
        } else {
            response = ack_response;
            response_length = (uint16_t)(sizeof(ack_response) - 1u);
        }
        count[0] = (uint8_t)response_length;
        count[1] = (uint8_t)(response_length >> 8);
        if (!reply->data_sink(count, 2u, reply->data_context)
            || !reply->data_sink(response, response_length, reply->data_context)) {
            return S64_UCI_CONSUMER_ABORTED;
        }
        return S64_UCI_OK;
    }

    if (command == 0x09u) return S64_UCI_OK;
    return S64_UCI_PROTOCOL_ERROR;
}

int main(void)
{
    static const char key_hex[] =
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f";
    static const char expected_signature[] =
        "bd550bf6a8184b7cf73837292476b31d"
        "23e4c7269dd66ab2507598bfe0eec4b9";
    *(volatile uint8_t*)0xc061 = 0u;
    require(s64_reu_probe() && s64_reu_probe_banks() == 256u, 1u);

    s64_reu_runtime_init(&control, 0x0202u, 0x44556677UL);
    require(allocate_asset(0u, editor_image, editor_image_end, 0x61u), 2u);
    require(allocate_asset(1u, pcontent_image, pcontent_image_end, 0x62u), 3u);
    require(allocate_asset(2u, pnetwork_image, pnetwork_image_end, 0x63u), 4u);
    require(allocate_asset(3u, pruntime_image, pruntime_image_end, 0x64u), 5u);
    require(allocate_asset(4u, settings_image, settings_image_end, 0x65u), 6u);

    require(s64_code_overlay_namespace_capture(
        &editor_namespace, &control, &assets[0], 1u), 7u);
    require(s64_code_overlay_namespace_capture(
        &publisher_namespace, &control, &assets[1], 3u), 8u);
    require(s64_code_overlay_namespace_capture(
        &settings_namespace, &control, &assets[4], 1u), 9u);

    memset(&body, 0, sizeof(body));
    memset(&document, 0, sizeof(document));
    document.body = &body;
    document.type = S64_DOCUMENT_POST;
    s64_persistent_publisher_init(&publisher, &document);
    require(s64_persistent_publisher_valid(&publisher), 10u);

    memset(body_digest, 0, sizeof(body_digest));

    for (cycle = 1u; cycle <= 8u; ++cycle) {
        require(s64_code_overlay_namespace_activate(
            &editor_namespace, &control, overlay_reader), 11u);
        require(s64_probe_editor(cycle) == (uint16_t)(cycle ^ 0x64a5u), 12u);

        require(s64_code_overlay_namespace_activate(
            &publisher_namespace, &control, overlay_reader), 13u);

        require(s64_destination_configure(
            &publisher.destination,
            S64_DESTINATION_STANDALONE,
            "http://example.test/base/",
            1u
        ) == S64_DESTINATION_OK, 42u);
        require(!strcmp(publisher.destination.base_url,
            "http://example.test/base"), 43u);
        require(s64_destination_open_url(
            &publisher.destination,
            S64_DOCUMENT_POST,
            "entry-1",
            url,
            sizeof(url)
        ) == S64_DESTINATION_OK, 44u);
        require(!strcmp(url,
            "http://example.test/base/api.php?action=get&type=post&slug=entry-1&format=s64"),
            45u);

        s64_publish_state_init(&publisher.publish_state);
        s64_publish_state_set_connected(&publisher.publish_state, 1u);
        require(s64_destination_configure(
            &publisher.destination,
            S64_DESTINATION_WORDPRESS,
            "http://example.test/wp",
            1u
        ) == S64_DESTINATION_OK, 46u);
        require(s64_publish_state_bind_remote(
            &publisher.publish_state, &publisher.destination,
            S64_DOCUMENT_POST, "12345"), 14u);
        require(s64_publish_state_can_delete(
            &publisher.publish_state, &publisher.destination), 15u);

        require(s64_probe_pnetwork_ping(cycle)
            == (uint16_t)(cycle ^ 0x5aa5u), 16u);
        require(s64_direct_key_parse(key_hex, key), 17u);
        require(s64_direct_sign(
            key,
            "0123456789abcdef0123456789abcdef",
            "POST",
            "/api/save",
            body_digest,
            signature
        ), 18u);
        require(hex64(signature) && !strcmp(signature, expected_signature), 19u);

        s64_net_init(&network, fake_network_uci);
        require(s64_net_open_tcp(
            &network, "example.test", 80u, &network_handle
        ) == S64_NET_OK && network_handle == 0x2au, 51u);
        require(s64_net_write_text(
            &network, network_handle, "PING", 4u
        ) == S64_NET_OK, 52u);
        network_payload_length = 0u;
        network_received = 0u;
        require(s64_net_read(
            &network,
            network_handle,
            16u,
            capture_network_payload,
            0,
            &network_received
        ) == S64_NET_OK
            && network_received == 2u
            && network_payload_length == 2u
            && network_payload[0] == 'O'
            && network_payload[1] == 'K', 53u);
        require(s64_net_close(
            &network, network_handle
        ) == S64_NET_OK, 54u);

        require(s64_direct_publisher_key(key_hex), 55u);
        fake_http_open_count = 0u;
        direct_http.execute = fake_http_uci;
        direct_result = s64_direct_publisher_test(
            &direct_http,
            "http://example.test/api.php?action=settings",
            1u
        );
        require(direct_result == S64_STANDALONE_OK,
            (uint8_t)(80u + (uint8_t)(fake_http_open_count * 10u)
                + (uint8_t)direct_result));

        require(s64_probe_pruntime_ping(cycle)
            == (uint16_t)(cycle ^ 0xa55au), 40u);
        s64_settings_exchange_init(&settings);
        require(settings.version == S64_SETTINGS_VERSION
            && settings.state == S64_SETTINGS_STATE_READY
            && settings.result == S64_SETTINGS_RESULT_OK, 41u);
        require(s64_settings_exchange_validate(&settings), 20u);
        require(s64_settings_exchange_store(&settings), 21u);
        memset(&loaded, 0, sizeof(loaded));
        require(s64_settings_exchange_load(&loaded), 22u);
        require(!memcmp(&settings, &loaded, sizeof(settings)), 23u);

        s64_persistent_publisher_mark_changed(&publisher, cycle == 1u);
        require(publisher.document_changed
            && publisher.document_replaced == 1u, 24u);

        require(s64_code_overlay_namespace_activate(
            &settings_namespace, &control, overlay_reader), 25u);
        require(s64_probe_settings(cycle) == (uint16_t)(cycle + 0x64u), 26u);

        require(s64_code_overlay_namespace_activate(
            &publisher_namespace, &control, overlay_reader), 27u);
        before = overlay_reads;
        require(s64_publish_state_can_delete(
            &publisher.publish_state, &publisher.destination), 28u);
        require(overlay_reads == (uint16_t)(before + 1u), 29u);
        require(publisher.document_changed
            && publisher.document_replaced == 1u, 30u);

        require(s64_code_overlay_namespace_activate(
            &editor_namespace, &control, overlay_reader), 31u);
        require(s64_probe_editor((uint16_t)(cycle + 100u))
            == (uint16_t)((cycle + 100u) ^ 0x64a5u), 32u);
        require(document.body == &body && document.type == S64_DOCUMENT_POST, 33u);
    }

    s64_persistent_publisher_reset_change_flags(&publisher);
    require(!publisher.document_changed && !publisher.document_replaced, 34u);

    *(volatile uint8_t*)0xc060 = 0x80u;
    *(volatile uint8_t*)0xc061 = 0xa5u;
    *(volatile uint8_t*)0xc062 = 0u;
    for (;;) {}
}
"""


def config_text() -> str:
    groups = ("EDITOR", "PCONTENT", "PNETWORK", "PRUNTIME", "SETTINGS")
    memory = "\n".join(
        f'    {g}: file = "build/persistent-publisher-vice/{g.lower()}.bin", '
        'start = $8C00, size = $3C00;'
        for g in groups
    )
    segments = "\n".join(
        f"    {g}_CODE: load = {g}, type = ro, define = yes;\n"
        f"    {g}_RODATA: load = {g}, type = ro, define = yes;"
        for g in groups
    )
    return f"""FEATURES {{ STARTADDRESS: default = $0801; }}
SYMBOLS {{
    __LOADADDR__: type = import;
    __EXEHDR__: type = import;
    __STACKSIZE__: type = weak, value = $0400;
    __HIMEM__: type = weak, value = $D000;
}}
MEMORY {{
    ZP: file = "", define = yes, start = $0002, size = $001A;
    LOADADDR: file = %O, start = %S - 2, size = $0002;
    HEADER: file = %O, define = yes, start = %S, size = $000D;
    MAIN: file = %O, define = yes, start = __HEADER_LAST__, size = __HIMEM__ - __HEADER_LAST__;
    BSS: file = "", start = __ONCE_RUN__, size = $8C00 - __ONCE_RUN__;
{memory}
}}
SEGMENTS {{
    ZEROPAGE: load = ZP, type = zp;
    LOADADDR: load = LOADADDR, type = ro;
    EXEHDR: load = HEADER, type = ro;
    STARTUP: load = MAIN, type = ro;
    LOWCODE: load = MAIN, type = ro, optional = yes;
    CODE: load = MAIN, type = ro;
    RODATA: load = MAIN, type = ro;
    DATA: load = MAIN, type = rw;
    INIT: load = MAIN, type = rw;
    ONCE: load = MAIN, type = ro, define = yes;
    BSS: load = BSS, type = bss, define = yes;
{segments}
}}
FEATURES {{
    CONDES: type = constructor, label = __CONSTRUCTOR_TABLE__, count = __CONSTRUCTOR_COUNT__, segment = ONCE;
    CONDES: type = destructor, label = __DESTRUCTOR_TABLE__, count = __DESTRUCTOR_COUNT__, segment = RODATA;
    CONDES: type = interruptor, label = __INTERRUPTOR_TABLE__, count = __INTERRUPTOR_COUNT__, segment = RODATA, import = __CALLIRQ__;
}}
"""


def build() -> Path:
    OUT.mkdir(parents=True, exist_ok=True)
    generated = OUT / "generated"
    objects_dir = OUT / "objects"
    generated.mkdir(parents=True, exist_ok=True)
    objects_dir.mkdir(parents=True, exist_ok=True)

    cfg = OUT / "probe.cfg"
    cfg.write_text(config_text(), encoding="ascii", newline="\n")
    flags = [
        "-t", "c64", "-Os", "-I", "src", "-C", str(cfg),
        "-DS64_NATIVE_PETSCII=1",
        "-DS64_REU_BUFFER=1", "-DS64_PERSISTENT_RUNTIME_FIT=1",
    ]

    grouped_names: dict[str, list[str]] = {
        "EDITOR": ["s64_probe_editor"],
        "PCONTENT": [],
        "PNETWORK": [],
        "PRUNTIME": [],
        "SETTINGS": ["s64_probe_settings"],
    }
    sources: list[Path] = []

    editor = generated / "editor.c"
    editor.write_text(EDITOR)
    sources.append(editor)

    for group, paths in GROUPS.items():
        names: list[str] = []
        for path in paths:
            names.extend(fit.public_s64_definitions(
                (CLIENT / path).read_text(encoding="utf-8")
            ))
        grouped_names[group] = list(dict.fromkeys(names))
        for path in paths:
            generated_path = generated / f"{group.lower()}_{Path(path).stem}.c"
            generated_path.write_text(
                fit.generated_translation_unit(
                    group, path, grouped_names[group], generated, CLIENT, bridges={}
                )
            )
            sources.append(generated_path)

    pnetwork_ping = generated / "pnetwork_ping.c"
    pnetwork_ping.write_text(PNETWORK_PING)
    sources.append(pnetwork_ping)
    grouped_names["PNETWORK"].append("s64_probe_pnetwork_ping")

    pruntime_ping = generated / "pruntime_ping.c"
    pruntime_ping.write_text(PRUNTIME_PING)
    sources.append(pruntime_ping)
    grouped_names["PRUNTIME"].append("s64_probe_pruntime_ping")

    settings = generated / "settings.c"
    settings.write_text(SETTINGS)
    sources.append(settings)

    main = generated / "main.c"
    main.write_text(MAIN)
    sources.append(main)

    resident = [
        CLIENT / "src/platform/code_overlay.c",
        CLIENT / "src/platform/code_overlay_namespace.c",
        CLIENT / "src/platform/reu.c",
        CLIENT / "src/platform/reu_dma.s",
        CLIENT / "src/platform/reu_runtime.c",
        CLIENT / "src/platform/reu_runtime_build.c",
        CLIENT / "src/platform/reu_runtime_write.c",
        CLIENT / "src/publisher/persistent_mode.c",
        CLIENT / "diagnostics/persistent_publisher_services.c",
        CLIENT / "src/charset.c",
    ]
    sources.extend(resident)

    wrappers = generated / "wrappers.s"
    wrappers.write_text(
        fit.assembler_wrappers(grouped_names, group_ids=GROUP_IDS, bridges={})
    )
    sources.append(wrappers)

    objects: list[Path] = []
    for index, source in enumerate(sources):
        obj = objects_dir / f"{index:03d}_{source.stem}.o"
        fit.run(["cl65", *flags, "-c", "-o", str(obj), str(source)], cwd=CLIENT)
        objects.append(obj)

    blobs = generated / "blobs.s"
    blob_obj = objects_dir / "blobs.o"
    prg = OUT / "persistent-publisher-vice.prg"
    map_path = OUT / "persistent-publisher-vice.map"
    group_order = ("editor", "pcontent", "pnetwork", "pruntime", "settings")

    for initial in (True, False, False):
        lines = ['.segment "RODATA"']
        for group in group_order:
            lines += [
                f".export _{group}_image, _{group}_image_end",
                f"_{group}_image:",
                ".byte 0" if initial else f'.incbin "{OUT.as_posix()}/{group}.bin"',
                f"_{group}_image_end:",
            ]
        blobs.write_text("\n".join(lines) + "\n")
        fit.run(["cl65", *flags, "-c", "-o", str(blob_obj), str(blobs)], cwd=CLIENT)
        fit.run(
            ["cl65", *flags, "-m", str(map_path), "-o", str(prg),
             *(str(obj) for obj in objects), str(blob_obj)],
            cwd=CLIENT,
        )

    for group in group_order:
        size = (OUT / f"{group}.bin").stat().st_size
        if not size or size > fit.OVERLAY_LIMIT:
            raise fit.FitError(f"{group} asset size invalid: {size}")
        print(f"{group.upper()} executable asset: {size} / {fit.OVERLAY_LIMIT}")

    end = fit.bss_end(map_path)
    print(f"Persistent Publisher VICE probe BSS ends at 0x{end:04X}")
    if end >= fit.RESIDENT_BSS_LIMIT:
        raise fit.FitError("persistent Publisher VICE probe resident BSS overlaps slot")
    return prg


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--port", type=int, default=6532)
    parser.add_argument("--vice-bin", default="x64sc")
    parser.add_argument("--rom-dir", type=Path, default=Path("/tmp/see64you-roms"))
    args = parser.parse_args()

    prg = build()
    if args.build_only:
        print(prg)
        return 0

    sys.path.insert(0, str(ROOT / "tools" / "vice"))
    from test_reu_probe import wait_for_monitor

    command = [
        "xvfb-run", "-a", args.vice_bin, "+sound", "-warp",
        "-reu", "-reusize", "16384",
        "-autostartprgmode", "1",
        "-binarymonitor",
        "-binarymonitoraddress", f"ip4://127.0.0.1:{args.port}",
    ]
    for kind in ("kernal", "basic", "chargen"):
        rom = args.rom_dir / f"{kind}.rom"
        if not rom.is_file():
            raise fit.FitError(f"Missing pinned Open ROM: {rom}")
        command += [f"-{kind}", str(rom)]
    command += ["-autostart", str(prg)]

    with (OUT / "vice.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        monitor = None
        try:
            monitor = wait_for_monitor("127.0.0.1", args.port, 20)
            deadline = time.monotonic() + 90
            state = b""
            while time.monotonic() < deadline:
                state = monitor.read_memory(0xc060, 0xc062)
                if state[1] == 0xa5:
                    print(
                        "VICE real Publisher slice passed: Editor/Publisher/Settings "
                        "namespace switching, remote-state binding, HMAC signature, "
                        "REU settings round-trip, persistent owner state"
                    )
                    return 0
                if state[1] == 0x5a:
                    raise fit.FitError(
                        f"persistent Publisher VICE probe failed at step {state[2]}"
                    )
                monitor.resume()
                time.sleep(0.03)
            raise fit.FitError(f"persistent Publisher VICE probe timed out: {state.hex()}")
        finally:
            if monitor:
                monitor.quit()
                monitor.close()
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except fit.FitError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
