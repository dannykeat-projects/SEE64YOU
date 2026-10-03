.setcpu "6502"

.export _s64_module_transfer_stub_image
.export _s64_module_transfer_stub_end
.export _s64_module_transfer_stub_enter

REU_COMMAND = $df01
REU_C64_LO = $df02
REU_C64_HI = $df03
REU_REU_LO = $df04
REU_REU_HI = $df05
REU_REU_BANK = $df06
REU_LENGTH_LO = $df07
REU_LENGTH_HI = $df08
REU_ADDR_CONTROL = $df0a

PARAM_SOURCE_LO = $07d0
PARAM_SOURCE_HI = $07d1
PARAM_SOURCE_BANK = $07d2
PARAM_DEST_LO = $07d3
PARAM_DEST_HI = $07d4
PARAM_LENGTH_LO = $07d5
PARAM_LENGTH_HI = $07d6
PARAM_START_LO = $07d7
PARAM_START_HI = $07d8
PARAM_SAVED_MEM = $07d9
PARAM_DESTINATION = $07da

ARRIVAL_0 = $02a7
ARRIVAL_1 = $02a8
ARRIVAL_DEST = $02a9

TRANSFER_FROM_REU_DELAYED = $ed

.segment "RODATA"

_s64_module_transfer_stub_image:
    php
    sei

    lda PARAM_DEST_LO
    sta REU_C64_LO
    lda PARAM_DEST_HI
    sta REU_C64_HI
    lda PARAM_SOURCE_LO
    sta REU_REU_LO
    lda PARAM_SOURCE_HI
    sta REU_REU_HI
    lda PARAM_SOURCE_BANK
    sta REU_REU_BANK
    lda PARAM_LENGTH_LO
    sta REU_LENGTH_LO
    lda PARAM_LENGTH_HI
    sta REU_LENGTH_HI
    lda #$00
    sta REU_ADDR_CONTROL

    lda #TRANSFER_FROM_REU_DELAYED
    sta REU_COMMAND

    lda $01
    sta PARAM_SAVED_MEM
    and #$f8
    sta $01
    lda $ff00
    sta $ff00
    lda PARAM_SAVED_MEM
    sta $01

    ; cl65's C64 target maps assembler character literals to PETSCII too.
    ; Keep this one-shot wire marker ASCII, matching the resident consumer.
    lda #$53
    sta ARRIVAL_0
    lda #$36
    sta ARRIVAL_1
    lda PARAM_DESTINATION
    sta ARRIVAL_DEST

    plp
    jmp (PARAM_START_LO)
_s64_module_transfer_stub_end:

.assert (_s64_module_transfer_stub_end - _s64_module_transfer_stub_image) <= $d0, error, "transfer stub exceeds screen-RAM slot"

.segment "CODE"

.proc _s64_module_transfer_stub_enter
    jmp $0700
.endproc
