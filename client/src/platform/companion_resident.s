.setcpu "6502"

UCI_CONTROL = $df1c
UCI_COMMAND = $df1d
UCI_RESPONSE = $df1e
UCI_STATUS = $df1f

PUSH_COMMAND = $01
ACCEPT_DATA = $02
ABORT_COMMAND = $04

STATE_MASK = $30
STATE_LAST = $20
STATE_MORE = $30
STATUS_AVAILABLE = $40

DEST_LO = $fb
DEST_HI = $fc
REMAIN_LO = $fd
REMAIN_HI = $fe

TARGET_MARKER = $02a7
SCREEN = $0400
COLOR = $d800
BORDER = $d020
BACKGROUND = $d021
TARGET_START = $080d

.segment "CODE"

entry:
    jmp start
payload_lo:
    .byte $00
payload_hi:
    .byte $00
failure:
    .byte $00
block_state:
    .byte $00
timeout_lo:
    .byte $00
timeout_mid:
    .byte $00
timeout_hi:
    .byte $00
close_count:
    .byte $00
close_first:
    .byte $00
close_second:
    .byte $00

start:
    sei
    lda #$00
    sta failure
    sta TARGET_MARKER
    lda #$01
    sta DEST_LO
    lda #$08
    sta DEST_HI
    lda payload_lo
    sta REMAIN_LO
    lda payload_hi
    sta REMAIN_HI

    jsr wait_idle
    bcc send_read
    lda #$01
    jmp fail

send_read:
    lda #$01
    sta UCI_COMMAND
    lda #$04
    sta UCI_COMMAND
    lda payload_lo
    sta UCI_COMMAND
    lda payload_hi
    sta UCI_COMMAND
    lda #PUSH_COMMAND
    sta UCI_CONTROL

    jsr read_all
    bcc check_length
    lda failure
    jmp fail

check_length:
    lda REMAIN_LO
    ora REMAIN_HI
    beq close_open_file
    lda #$05
    jmp fail

close_open_file:
    jsr close_file
    bcc launch_target
    lda failure
    jmp fail

launch_target:
    cli
    jsr TARGET_START
    sei
    lda TARGET_MARKER
    cmp #$5a
    beq pass
    lda #$08
    jmp fail

read_all:
next_read_block:
    jsr wait_response
    bcc drain_read_data
    lda #$02
    sta failure
    sec
    rts

drain_read_data:
    lda UCI_CONTROL
    bpl drain_read_status
    lda REMAIN_LO
    ora REMAIN_HI
    bne store_read_byte
    lda #ABORT_COMMAND
    sta UCI_CONTROL
    lda #$03
    sta failure
    sec
    rts

store_read_byte:
    lda UCI_RESPONSE
    ldy #$00
    sta (DEST_LO),y
    inc DEST_LO
    bne dec_remaining
    inc DEST_HI
dec_remaining:
    lda REMAIN_LO
    bne dec_remaining_lo
    dec REMAIN_HI
dec_remaining_lo:
    dec REMAIN_LO
    jmp drain_read_data

drain_read_status:
    lda UCI_CONTROL
    and #STATUS_AVAILABLE
    beq accept_read_block
    lda UCI_STATUS
    lda #$04
    sta failure
    jmp drain_read_status

accept_read_block:
    lda #ACCEPT_DATA
    sta UCI_CONTROL
    lda failure
    beq continue_read
    sec
    rts
continue_read:
    lda block_state
    cmp #STATE_MORE
    beq next_read_block
    clc
    rts

close_file:
    jsr wait_idle
    bcc send_close
    lda #$06
    sta failure
    sec
    rts

send_close:
    lda #$00
    sta close_count
    sta close_first
    sta close_second
    lda #$01
    sta UCI_COMMAND
    lda #$03
    sta UCI_COMMAND
    lda #PUSH_COMMAND
    sta UCI_CONTROL

next_close_block:
    jsr wait_response
    bcc drain_close_data
    lda #$06
    sta failure
    sec
    rts

drain_close_data:
    lda UCI_CONTROL
    bpl drain_close_status
    lda UCI_RESPONSE
    lda #$07
    sta failure
    jmp drain_close_data

drain_close_status:
    lda UCI_CONTROL
    and #STATUS_AVAILABLE
    beq accept_close_block
    lda UCI_STATUS
    ldx close_count
    cpx #$00
    bne close_second_check
    sta close_first
close_second_check:
    cpx #$01
    bne close_count_next
    sta close_second
close_count_next:
    inc close_count
    jmp drain_close_status

accept_close_block:
    lda #ACCEPT_DATA
    sta UCI_CONTROL
    lda block_state
    cmp #STATE_MORE
    beq next_close_block

    lda failure
    bne close_bad
    lda close_count
    cmp #$02
    bcc close_bad
    lda close_first
    cmp #'0'
    bne close_bad
    lda close_second
    cmp #'0'
    bne close_bad
    clc
    rts

close_bad:
    lda #$07
    sta failure
    sec
    rts

wait_idle:
    jsr reset_timeout
wait_idle_loop:
    lda UCI_CONTROL
    and #STATE_MASK
    beq wait_ok
    jsr tick_timeout
    bcc wait_idle_loop
    sec
    rts

wait_response:
    jsr reset_timeout
wait_response_loop:
    lda UCI_CONTROL
    and #STATE_MASK
    cmp #STATE_LAST
    beq response_ok
    cmp #STATE_MORE
    beq response_ok
    jsr tick_timeout
    bcc wait_response_loop
    sec
    rts

response_ok:
    sta block_state
wait_ok:
    clc
    rts

reset_timeout:
    lda #$00
    sta timeout_lo
    sta timeout_mid
    lda #$04
    sta timeout_hi
    rts

tick_timeout:
    dec timeout_lo
    bne tick_ok
    dec timeout_mid
    bne tick_ok
    dec timeout_hi
    bne tick_ok
    sec
    rts
tick_ok:
    clc
    rts

pass:
    lda #$12
    sta SCREEN+40
    lda #$05
    sta SCREEN+41
    lda #$14
    sta SCREEN+42
    lda #$15
    sta SCREEN+43
    lda #$12
    sta SCREEN+44
    lda #$0e
    sta SCREEN+45
    lda #$20
    sta SCREEN+46
    lda #$10
    sta SCREEN+47
    lda #$01
    sta SCREEN+48
    lda #$13
    sta SCREEN+49
    sta SCREEN+50
    lda #$01
    ldx #$0a
pass_color:
    sta COLOR+40,x
    dex
    bpl pass_color
    lda #$05
    sta BORDER
    lda #$00
    sta BACKGROUND
    cli
pass_spin:
    jmp pass_spin

fail:
    sta failure
    lda #ABORT_COMMAND
    sta UCI_CONTROL
    lda #$06
    sta SCREEN
    lda #$01
    sta SCREEN+1
    lda #$09
    sta SCREEN+2
    lda #$0c
    sta SCREEN+3
    lda #$20
    sta SCREEN+4
    lda #$30
    sta SCREEN+5
    lda failure
    and #$0f
    clc
    adc #$30
    sta SCREEN+6
    lda #$01
    ldx #$06
fail_color:
    sta COLOR,x
    dex
    bpl fail_color
    lda #$02
    sta BORDER
    lda #$00
    sta BACKGROUND
    cli
fail_spin:
    jmp fail_spin

.assert (* - entry) <= $0400, error, "resident loader exceeds 512-byte slot"
.res $0400 - (* - entry), $ea
