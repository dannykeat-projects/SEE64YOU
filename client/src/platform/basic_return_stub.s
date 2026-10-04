.setcpu "6502"
.export _s64_basic_return_stub_image, _s64_basic_return_stub_end
.export _s64_basic_return_stub_enter
.import donelib

; Managed diagnostic exit; normal/public product transfer remains gated.
; Caller must first complete shutdown/restore/verify/CLEAN and validate a
; session-bound original SYS context before staging these 285 bytes at $0400:
; original $01, incoming SP, processor flags, $02-$1B, full $0100-$01FF.
; Copy this image to $0700. Neither code nor context uses the restored stack.
CONTEXT = $0400
CONTEXT_ZP = CONTEXT + 3
CONTEXT_STACK = CONTEXT_ZP + $1a

.segment "RODATA"
_s64_basic_return_stub_image:
    sei
    ldx #$00
@stack:
    lda CONTEXT_STACK,x
    sta $0100,x
    inx
    bne @stack
    ldx #$19
@zp:
    lda CONTEXT_ZP,x
    sta $02,x
    dex
    bpl @zp
    lda #$00
    sta $90                 ; BASIC status, as in cc65 crt0 exit
    lda CONTEXT
    sta $01
    ldx CONTEXT+1
    txs
    lda CONTEXT+2
    pha
    plp
    rts                     ; original SYS return frame, not a module caller
_s64_basic_return_stub_end:
.assert (_s64_basic_return_stub_end - _s64_basic_return_stub_image) <= $d0, error, "BASIC return stub exceeds transfer scratch"

.segment "CODE"
_s64_basic_return_stub_enter:
    ; Preserve stock crt0's destructor phase, including original IRQ vector.
    ; Current C zero page/stack remain live until these calls finish.
    jsr donelib
    jmp $0700
