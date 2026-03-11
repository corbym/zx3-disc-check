SECTION code_user

PUBLIC _set_motor_on
PUBLIC _set_motor_off
PUBLIC _inportb
PUBLIC _outportb

defc BANK678 = $5B67

SECTION bss_user
paging_value:           defs 1  ; Paging bits captured at first call
paging_initialized:     defs 1  ; Flag: 0 = not yet read, 1 = ready

SECTION code_user

; void set_motor_on(void)
; Enable motor (set bit 3), preserving other paging bits.

_set_motor_on:
        call    init_paging
        ld      a,(paging_value)
        or      $08                 ; set bit 3 (motor on)
        call    write_1ffd
        ld      (paging_value),a
        ret

; void set_motor_off(void)
; Disable motor (clear bit 3), preserving other paging bits.

_set_motor_off:
        call    init_paging
        ld      a,(paging_value)
        and     $F7                 ; clear bit 3 (motor off)
        call    write_1ffd
        ld      (paging_value),a
        ret

; init_paging: capture paging bits once from BANK678.
init_paging:
        ld      a,(paging_initialized)
        or      a
        ret     nz                  ; already initialized
        ld      a,(BANK678)
        ld      (paging_value),a
        ld      a,1
        ld      (paging_initialized),a
        ret

; write_1ffd: write A to port $1FFD and both shadows.
; Preserves interrupt state: if IFFs were off on entry, they stay off on return.

write_1ffd:
        push    bc
        ld      d,a                 ; save value in D
        ld      a,i                 ; capture IFF2 into P/V flag
        push    af
        ld      bc,$1FFD
        di
        ld      a,d                 ; restore value
        ld      (BANK678),a         ; update ROM shadow
        out     (c),a               ; write port $1FFD
        pop     af
        jp      po,write_1ffd_no_ei ; P/V=0 means interrupts were off
        ei
write_1ffd_no_ei:
        pop     bc
        ret

; unsigned char inportb(unsigned short port)
; __smallc call convention: arg word at sp+2
_inportb:
        pop     de                  ; return address
        pop     bc                  ; port
        in      l,(c)
        ld      h,0
        push    bc
        push    de
        ret

; void outportb(unsigned short port, unsigned char value)
; __smallc call convention: value word at sp+2, port word at sp+4
_outportb:
        pop     de                  ; return address
        pop     hl                  ; value in L
        pop     bc                  ; port
        out     (c),l
        push    bc
        push    hl
        push    de
        ret