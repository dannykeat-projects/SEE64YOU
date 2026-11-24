.setcpu "6502"
.export _s64_basic_return_capture_start
.import __MAIN_START__
.import __MAIN_SIZE__, __STACKSIZE__, __BSS_RUN__, __BSS_SIZE__
.assert __BSS_RUN__ + __BSS_SIZE__ <= $c800, lderror, "Return capture overlaps launcher resident state"
.assert __MAIN_START__ + __MAIN_SIZE__ - __STACKSIZE__ >= $ca88, lderror, "Return capture overlaps software stack"

; This object must precede stock crt0 in STARTUP. Falling through retains the
; toolchain's own startup/exit implementation without copying private crt0.
.segment "STARTUP"
_s64_basic_return_capture_start:
.assert _s64_basic_return_capture_start = __MAIN_START__, lderror, "SYS must enter original-context capture first"
    php
    sei
    pla
    sta $c822               ; original flags, before crt0 touches zero page
    tsx
    stx $c821
    lda $01
    sta $c820
    ldx #$19
@zp:
    lda $02,x
    sta $c823,x
    dex
    bpl @zp
    ldx #$00
@stack:
    lda $0100,x
    sta $c83d,x
    inx
    bne @stack
    ; Capture before the launcher/Editor changes the display. These bytes are
    ; included in the session-bound CRC and restored by the final exit path.
    lda $d020
    sta $c93d
    lda $d021
    sta $c93e
    lda $0286
    sta $c93f
    ; Fresh capture marker. Journal binding and CRC are added only after ACTIVE.
    lda #$53
    sta $c800
    lda #$36
    sta $c801
    lda #$34
    sta $c802
    lda #$42
    sta $c803
    lda #$02
    sta $c804
    lda #$01
    sta $c807
    lda #$1a
    sta $c805
    lda #$20
    sta $c806
    lda $c822
    pha
    plp
    ; Stock crt0 STARTUP immediately follows this object's STARTUP bytes.
