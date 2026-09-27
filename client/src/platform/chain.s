; Compact fixed-target C64 program chaining for the C64U editor.
; Mirrors cc65's BASIC chaining approach without generic file-I/O runtime state.

.export _s64_chain_publisher
.import _exit

.segment "LOWCODE"

chain_line:
    .byte $00
    .word chain_line_end
    .word $0000
    .byte $9c            ; CLR
    .byte ':'
    .byte $93            ; LOAD
    .byte '"','"'
    .byte $aa            ; +
    .byte '"'
    .byte "S64PUB.PRG"
    .byte '"'
    .byte "          "
    .byte ','
chain_unit:
    .byte '0','8',$00
chain_line_end:

.segment "CODE"

.proc _s64_chain_publisher
    ; Use the device that loaded the current program when sensible.
    lda $ba
    cmp #8
    bcc @default_device
    cmp #31
    bcc @device_ok
@default_device:
    lda #8
@device_ok:
    ldx #'0'
@tens:
    cmp #10
    bcc @digits_ready
    sec
    sbc #10
    inx
    bne @tens
@digits_ready:
    stx chain_unit
    clc
    adc #'0'
    sta chain_unit+1

    ; Match the essential cc65 exec() BASIC-state preparation.
    lda $37
    sta $2d
    lda $38
    sec
    sbc #1
    sta $2e

    lda #$8f            ; REM token: empty command line
    sta $0200
    lda #$00
    sta $0201

    lda #<chain_line
    sta $7a
    lda #>chain_line
    sta $7b

    lda #$00
    jmp _exit
.endproc
