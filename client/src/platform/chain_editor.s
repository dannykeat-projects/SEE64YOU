; Compact fixed-target return chain from the C64U publisher to the editor.
; Uses the same BASIC-chain technique as chain.s without pulling generic exec().

.export _s64_chain_editor
.export _s64_chain_settings
.import _exit

.segment "LOWCODE"

editor_chain_line:
    .byte $00
    .word editor_chain_line_end
    .word $0000
    .byte $9c            ; CLR
    .byte ':'
    .byte $93            ; LOAD
    .byte '"','"'
    .byte $aa            ; +
    .byte '"'
    ; CBM names omit the FAT .PRG suffix; lowercase source encodes unshifted keys.
    .byte "s64edit"
    .byte '"'
    .byte "   "
    .byte ','
editor_chain_unit:
    .byte '0','8',$00
editor_chain_line_end:

settings_chain_line:
    .byte $00
    .word settings_chain_line_end
    .word $0000
    .byte $9c
    .byte ':'
    .byte $93
    .byte '"','"'
    .byte $aa
    .byte '"'
    .byte "s64cfg"
    .byte '"'
    .byte "          "
    .byte ','
settings_chain_unit:
    .byte '0','8',$00
settings_chain_line_end:

.segment "CODE"

.proc _s64_chain_editor
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
    stx editor_chain_unit
    clc
    adc #'0'
    sta editor_chain_unit+1

    lda $37
    sta $2d
    lda $38
    sec
    sbc #1
    sta $2e

    lda #$8f
    sta $0200
    lda #$00
    sta $0201

    lda #<editor_chain_line
    sta $7a
    lda #>editor_chain_line
    sta $7b

    lda #$00
    jmp _exit
.endproc

.proc _s64_chain_settings
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
    stx settings_chain_unit
    clc
    adc #'0'
    sta settings_chain_unit+1

    lda $37
    sta $2d
    lda $38
    sec
    sbc #1
    sta $2e

    lda #$8f
    sta $0200
    lda #$00
    sta $0201

    lda #<settings_chain_line
    sta $7a
    lda #>settings_chain_line
    sta $7b

    lda #$00
    jmp _exit
.endproc
