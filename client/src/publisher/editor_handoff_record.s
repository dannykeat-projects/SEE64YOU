; The editor handoff record is only needed immediately before leaving the
; editor and immediately after startup, before the UI is rendered. Reuse the
; normal C64 screen as transient staging instead of spending scarce C64U BSS.
.export _s64_editor_handoff_record := $0400
