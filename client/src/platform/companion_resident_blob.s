.export _s64_resident_loader_image

.segment "RODATA"

_s64_resident_loader_image:
    .incbin "build/companion-resident.bin"
