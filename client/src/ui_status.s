;
; Fast status-number updates for the C64 editor.
; Values are written directly to the default C64 screen RAM so normal
; append typing does not disturb the conio cursor or invoke printf.
;

        .export _s64_ui_status_line_fast
        .export _s64_ui_status_column_fast
        .export _s64_ui_status_dirty_fast

        .importzp ptr1, ptr2

STATUS_BASE     = $07C0
STATUS_LINE     = STATUS_BASE + 3
STATUS_COLUMN   = STATUS_BASE + 12
STATUS_DIRTY    = STATUS_BASE + 20
REVERSE_ZERO    = $B0
REVERSE_SPACE   = $A0

.code

; __fastcall__: unsigned int arrives in A/X.
_s64_ui_status_line_fast:
        sta     ptr1
        stx     ptr1+1
        lda     #<STATUS_LINE
        sta     ptr2
        lda     #>STATUS_LINE
        sta     ptr2+1
        jmp     format4

; __fastcall__: unsigned int arrives in A/X.
_s64_ui_status_column_fast:
        sta     ptr1
        stx     ptr1+1
        lda     #<STATUS_COLUMN
        sta     ptr2
        lda     #>STATUS_COLUMN
        sta     ptr2+1

; Render a zero-padded four-digit unsigned value from ptr1 to ptr2.
; SEE64YOU's 4 KiB document capacity keeps line/column values <= 4097.
format4:
        ldx     #0
thousands:
        lda     ptr1+1
        cmp     #$03
        bcc     thousands_done
        bne     subtract_1000
        lda     ptr1
        cmp     #$E8
        bcc     thousands_done
subtract_1000:
        sec
        lda     ptr1
        sbc     #$E8
        sta     ptr1
        lda     ptr1+1
        sbc     #$03
        sta     ptr1+1
        inx
        bne     thousands
thousands_done:
        txa
        ora     #REVERSE_ZERO
        ldy     #0
        sta     (ptr2),y

        ldx     #0
hundreds:
        lda     ptr1+1
        bne     subtract_100
        lda     ptr1
        cmp     #100
        bcc     hundreds_done
subtract_100:
        sec
        lda     ptr1
        sbc     #100
        sta     ptr1
        lda     ptr1+1
        sbc     #0
        sta     ptr1+1
        inx
        bne     hundreds
hundreds_done:
        txa
        ora     #REVERSE_ZERO
        iny
        sta     (ptr2),y

        ldx     #0
tens:
        lda     ptr1+1
        bne     subtract_10
        lda     ptr1
        cmp     #10
        bcc     tens_done
subtract_10:
        sec
        lda     ptr1
        sbc     #10
        sta     ptr1
        lda     ptr1+1
        sbc     #0
        sta     ptr1+1
        inx
        bne     tens
tens_done:
        txa
        ora     #REVERSE_ZERO
        iny
        sta     (ptr2),y

        lda     ptr1
        ora     #REVERSE_ZERO
        iny
        sta     (ptr2),y
        rts

; The dirty flag is fixed-width in the status line: " MOD".
_s64_ui_status_dirty_fast:
        lda     #REVERSE_SPACE
        sta     STATUS_DIRTY
        lda     #$8D            ; reverse M (screen code 13)
        sta     STATUS_DIRTY+1
        lda     #$8F            ; reverse O (screen code 15)
        sta     STATUS_DIRTY+2
        lda     #$84            ; reverse D (screen code 4)
        sta     STATUS_DIRTY+3
        rts
