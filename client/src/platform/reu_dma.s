.export _s64_reu_kick

.segment "CODE"

; void __fastcall__ s64_reu_kick(unsigned char command);
; A contains the command byte. This follows cc65's c64-reu.emd transfer
; sequence: arm the REU, bank out ROM/I/O, then trigger via $FF00.
_s64_reu_kick:
    sta $df01
    php
    sei
    lda $01
    pha
    and #$f8
    sta $01
    lda $ff00
    sta $ff00
    pla
    sta $01
    plp
    rts
