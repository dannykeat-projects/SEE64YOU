#include "uci.h"

#include <time.h>

#define S64_UCI_CONTROL      (*(volatile uint8_t*)0xDF1Cu)
#define S64_UCI_COMMAND      (*(volatile uint8_t*)0xDF1Du)
#define S64_UCI_RESPONSE     (*(volatile uint8_t*)0xDF1Eu)
#define S64_UCI_STATUS_DATA  (*(volatile uint8_t*)0xDF1Fu)

#define S64_UCI_IDENT_MASK       0x7Fu
#define S64_UCI_IDENT_VALUE      0x49u

#define S64_UCI_PUSH_COMMAND     0x01u
#define S64_UCI_ACCEPT_DATA      0x02u

#define S64_UCI_STATE_MASK       0x30u
#define S64_UCI_STATE_IDLE       0x00u
#define S64_UCI_STATE_BUSY       0x10u
#define S64_UCI_STATE_LAST       0x20u
#define S64_UCI_STATE_MORE       0x30u
#define S64_UCI_STATUS_AVAILABLE 0x40u
#define S64_UCI_DATA_AVAILABLE   0x80u

#define S64_UCI_CMD_IDENTIFY     0x01u
#define S64_UCI_TARGET_HTTP      0x06u

#define S64_UCI_WAIT_SECONDS     15UL
#define S64_UCI_IDENTIFY_BUFFER  40u

static uint8_t s64_uci_interface_present(void)
{
    /* Firmware 3.15 may clear bit 7 while a completion IRQ is active. */
    return (uint8_t)(S64_UCI_COMMAND & S64_UCI_IDENT_MASK) == S64_UCI_IDENT_VALUE;
}

static uint8_t s64_uci_wait_expired(clock_t started)
{
    clock_t elapsed = (clock_t)(clock() - started);
    return elapsed >= (clock_t)(S64_UCI_WAIT_SECONDS * CLOCKS_PER_SEC);
}

static uint8_t s64_uci_wait_until_idle(void)
{
    clock_t started = clock();

    while (!s64_uci_wait_expired(started)) {
        if ((S64_UCI_CONTROL & S64_UCI_STATE_MASK) == S64_UCI_STATE_IDLE) {
            return 1u;
        }
    }

    return 0u;
}

static uint8_t s64_uci_wait_for_response(void)
{
    clock_t started = clock();

    while (!s64_uci_wait_expired(started)) {
        uint8_t state = (uint8_t)(S64_UCI_CONTROL & S64_UCI_STATE_MASK);
        if (state == S64_UCI_STATE_LAST || state == S64_UCI_STATE_MORE) {
            return state;
        }
    }

    return S64_UCI_STATE_IDLE;
}

static uint8_t s64_uci_identify_http_target(void)
{
    uint8_t state;
    uint8_t data_length = 0u;

    if (!s64_uci_wait_until_idle()) {
        return 0u;
    }

    S64_UCI_COMMAND = S64_UCI_TARGET_HTTP;
    S64_UCI_COMMAND = S64_UCI_CMD_IDENTIFY;
    S64_UCI_CONTROL = S64_UCI_PUSH_COMMAND;

    for (;;) {
        state = s64_uci_wait_for_response();
        if (state != S64_UCI_STATE_LAST && state != S64_UCI_STATE_MORE) {
            return 0u;
        }

        while ((S64_UCI_CONTROL & S64_UCI_DATA_AVAILABLE) != 0u) {
            (void)S64_UCI_RESPONSE;
            if (data_length < S64_UCI_IDENTIFY_BUFFER) {
                ++data_length;
            }
        }

        while ((S64_UCI_CONTROL & S64_UCI_STATUS_AVAILABLE) != 0u) {
            (void)S64_UCI_STATUS_DATA;
        }

        S64_UCI_CONTROL = S64_UCI_ACCEPT_DATA;

        if (state == S64_UCI_STATE_LAST) {
            break;
        }
    }

    return data_length != 0u;
}

void s64_uci_probe_capabilities(S64UciCapabilities* capabilities)
{
    if (capabilities == 0) {
        return;
    }

    capabilities->interface_present = 0u;
    capabilities->http_target = 0u;

    if (!s64_uci_interface_present()) {
        return;
    }

    capabilities->interface_present = 1u;
    capabilities->http_target = s64_uci_identify_http_target();
}
