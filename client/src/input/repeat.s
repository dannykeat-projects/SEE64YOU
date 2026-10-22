; Keep normal text entry feeling like a modern editor while preventing
; shortcut chords from auto-repeating destructive commands.
;
; $028A RPTFLG:
;   $80 = all keys repeat
;   $40 = no keys repeat
;   $00 = stock C64 repeat set
;
; Shift is intentionally ignored so shifted printable characters repeat.
.export _s64_input_sync_repeat
.export _s64_input_restore_repeat

.segment "CODE"

.proc _s64_input_sync_repeat
    lda $028d
    and #$06
    beq @text
    lda #$40
    bne @store
@text:
    lda #$80
@store:
    sta $028a
    rts
.endproc

.proc _s64_input_restore_repeat
    lda #$00
    sta $028a
    rts
.endproc
