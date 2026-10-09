.setcpu "6502"
.destructor reset_basic_workspace, 1

.segment "CODE"
; A PRG loaded above BASIC's ceiling leaves its variable/array end beyond
; string memory. Reclaim that application's BASIC header only in this invalid
; state; ordinary valid BASIC workspace is left untouched.
reset_basic_workspace:
    lda $32
    cmp $38
    bcc done
    bne reclaim
    lda $31
    cmp $37
    bcc done
    beq done
reclaim:
    lda #0
    tay
    sta ($2b),y
    iny
    sta ($2b),y
    lda $2b
    clc
    adc #2
    sta $2d
    sta $2f
    sta $31
    lda $2c
    adc #0
    sta $2e
    sta $30
    sta $32
    lda $37
    sta $33
    lda $38
    sta $34
done:
    rts
