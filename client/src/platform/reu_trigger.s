.export _s64_reu_trigger

.segment "CODE"

.proc _s64_reu_trigger
    php
    sei
    ldy $01
    tya
    and #$f8
    sta $01
    lda $ff00
    sta $ff00
    sty $01
    plp
    rts
.endproc
