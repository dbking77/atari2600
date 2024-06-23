; https://dasm-assembler.github.io/
    processor 6502

    ;seg		main
    org 	$F000

reset:
    sei
    cld

    ; store 1 at 81
    lda #1
    sta $81

    ; load 1 from 81, shift left by 1, and store to 82
    ; 81 = 2
    lda $81
    asl
    sta $82

    ; use pull and push to read 82 and write 83
    ; 83 = 3
    ldx #$81
    txs
    pla
    ora #1
    ldx #$83
    txs
    pha

    ; branch should be taken
    ; 84 = 4
    lda $83
    cmp #2
    bpl l83
    jmp fail_loop
l83:
    adc #0
    sta $84

    ; test sbc and clc
    ; 16 - 10 - (~C) = 16 - 10 - 1 = 5
    ; 85 = 5
    lda #16
    clc
    sbc #10
    sta $85
    bcs l85      ; carry should be set
    jmp fail_loop
l85:

    ; test sbc and sec
    ; 0x10 + ~0x0A + C
    ; 0x10 + 0xF5 + 1 = 0x106 -> 0x06
    ; 86 = 6
    lda #16
    sec
    sbc #10
    sta $86
    bcs l86       ; carry should be set
    jmp fail_loop
l86:

    ; test ldy and sty
    ; 87 = 7
    ldy #7
    sty $87

    ; test transfer instructions
    lda #$88
    tax
    txs
    ldx #8
    txa
    tay
    lda #0
    tya
    tsx
    sta 0,x

    ; test load store X zeropage instructions
    ; 89 = 9
    ldx $82
    txa
    clc
    adc #7
    tax
    stx $89

    ; test load store A zeropage instructions
    ; 8A = A
    lda $83
    clc
    adc #7
    sta $8A

    ; test load store Y zeropage instructions
    ; 8B = B
    ldy $84
    tya
    clc
    adc #7
    tay
    sty $8B

    ; test JSR and RTS
    ldx #$FF
    txs
    jsr load8c
load8c_ret_addr:
    sta $8C
    jmp l8c
load8c:
    lda #$C
    rts
    jmp fail_loop
l8c:
    ; TODO somehow check value stored on
    ; stack to verify it is return addr - 1
    ; also make sure SP is still 0xFF
    lda $FF
    cmp #((load8c_ret_addr-1) >> 8)
    bne fail_jump1
    lda $FE
    cmp #((load8c_ret_addr-1) & 0xFF)
    bne fail_jump1

    ; test zero page eor, ora, and
    ; 8d = d -> 1101b
    lda #1
    ora $86  ; A = 7 = 0111b
    and $85  ; A = 5 = 0101b
    eor $88  ; A = D = 1101b
    sta $8D

    ; test immediate eor, ora, and
    ; 8e = e = 1110
    lda #1
    ora #$18  ; 11001
    and #$0F  ; 01001
    eor #7    ; 1110
    sta $8E

    ; test lda abs,X
    ; 8f = f
    ldx #1
    lda values_load_absolute,x
    sta $8f

    ; test lda abs,Y
    ; 90 = 0x10
    ldy #2
    lda values_load_absolute,Y
    sta $90

    ; test lda abs
    ; 91 = 0x11
    lda values_load_absolute+3
    sta $91

    ; test lda zpg,x
    ; 0x92 = 0x12
    ldx #1
    lda $8f,x   ; load 0x10 into A
    ora #2      ; A = 0x12
    sta $92

    ; test adc (without carry)
    ; 93 = $13
    lda #$10
    clc
    adc #3
    sta $93
    ; carry should not be set
    bcs fail_jump1

    ; test adc (with carry)
    ; 94 = $14
    lda #$90
    sec
    adc #$83
    sta $94
    ; carry should be set
    bcc fail_jump1

    ; trampoline
    jmp no_fail_jump1
fail_jump1:
    jmp fail_loop
no_fail_jump1:

    ; test adc zpg
    ; 95 = $15
    lda #$05
    clc
    adc $90  ; 90 = 0x10
    sta $95

    ; test adc zpg,x
    ; 96 = $16
    ldx #1
    lda #$05
    clc
    adc $90,x  ; 90+1 = 0x11
    sta $96

    ; test sty zpg,x
    ; 97 = $17
    ldy #$17
    ldx #$10
    sty $87,x

    ; test sty absolute (need to write to mirror of ram bank at higher addresses
    ; D80:DFF -> 080:0FF
    ; 98 = 18
    ldy #$18
    sty $D98

    ; test lda (indirect),y
    ; $99 = $19
    ; use FE and FF as storage for indirect address
    lda #(value_load_indirect_y >> 8)
    sta $FF
    lda #(value_load_indirect_y & 0xFF)
    sta $FE
    ldy #1
    lda ($FE),y
    sta $99

    ; test dec,zpg
    ; $9A = $1A
    lda #$1B
    sta $9A
    dec $9A

    ; test sta abs
    ; $9b = $1B
    lda #$1B
    sta $D9B   ; use mirror ram address

    ; test sta abs,x
    ; $9c = $1C
    lda #$1C
    ldx #1
    sta $D9B,x   ; use mirror ram address

test_9D: ; $9D = $1C
    ; test sta abs,y
    lda #$1D
    ldx #2
    sta $D9C,y   ; use mirror ram address

test_9E: ; 9E = 1E
    ; test ROR A
    lda #(0x1E << 1)
    clc
    ror
    sta $9E

test_9F:
    ; test overflow from adc
    ; -128 + 127 -> -1 (no overflow)
    clc
    lda #$80
    adc #$7F
    bvs fail_jump2

    ; -128 + -128 -> -256 (overflow)
    clc
    lda #$80
    adc #$80
    bvc fail_jump2

    ; ff = -1   -1 + -1 -> -2 (no overflow)
    clc
    lda #$ff
    adc #$ff
    bvs fail_jump2

    ; 80 = -128 + -1 -> -129 (overflow)
    clc
    lda #$80
    adc #$ff
    bvc fail_jump2

    ; 127 + 127 -> 254 (overflow)
    clc
    lda #$7F
    adc #$7F
    bvc fail_jump2

    ; 127 + 1 -> 128 (overflow)
    clc
    lda #$7F
    adc #$1
    bvc fail_jump2

    ; adc with overflow is okay
    lda #$1F
    sta $9F

    jmp no_fail_jump2
fail_jump2:
    jmp fail_loop
no_fail_jump2:


test_A0: ; $A0 = $20
    ; test inc zpg
    lda #$1F
    sta $A0
    inc $A0

test_A1: ; $A1 = $21
    ; test inc zpg+x
    lda #$20
    sta $A1
    ldx #1
    inc $A0,x

test_A2:  ; $A2 = $22
    ; test inc abs, use mirror address D80-DFF
    lda #$21
    sta $DA2
    inc $DA2

test_A3:  ; $A3 = $23
    ; test inc abs, use mirror address D80-DFF
    lda #$22
    sta $DA3
    ldx #1
    inc $DA2,x


test_A4:  ; $A4 = $24
    ; test overflow and subtract from sbc
    ; 126 - 127 - 0 -> -1 (no overflow)
    sec
    lda #$7E
    sbc #$7F
    bvs fail_loop
    cmp #$FF
    bne fail_loop

    ; test overflow from sbc
    ; 126 - 127 - 1 -> -2 (no overflow)
    clc
    lda #$7E
    sbc #$7F
    bvs fail_loop
    cmp #$FE
    bne fail_loop

    ; test overflow from sbc
    ; -128 - -128 - 0 -> 0 (no overflow)
    sec
    lda #$80
    sbc #$80
    bvs fail_loop
    cmp #$0
    bne fail_loop

    ; test overflow from sbc
    ; -128 - -128 - 1 -> -1 (no overflow)
    clc
    lda #$80
    sbc #$80
    bvs fail_loop
    cmp #$ff
    bne fail_loop

    ; -128 + -128 -> -256 (overflow)
    clc
    lda #$80
    adc #$80
    bvc fail_loop

    ; ff = -1   -1 + -1 -> -2 (no overflow)
    clc
    lda #$ff
    adc #$ff
    bvs fail_loop

    ; 80 = -128 + -1 -> -129 (overflow)
    clc
    lda #$80
    adc #$ff
    bvc fail_loop

    ; 127 + 127 -> 254 (overflow)
    clc
    lda #$7F
    adc #$7F
    bvc fail_loop

    ; 127 + 1 -> 128 (overflow)
    clc
    lda #$7F
    adc #$1
    bvc fail_loop

    ; adc with overflow is okay
    lda #$A4
    sta $24

    ; TODO test dey, dex, bmi, iny
    ; TODO test cpy
    ; TODO cmp indirect,x, cpx
    ; TODO test BIT
    ; TODO test overflow bit (ADC, SBC, BIT, CLV)
    ; TODO test ASL abs
    ; TODO test ROL (and flags)

forever_loop:
    lda #$00
    sta $80
    jmp forever_loop

fail_loop:
    lda #$FF
    sta $80
    jmp fail_loop

values_load_absolute:
    .byte 0, 0xF, 0x10, 0x11

value_load_indirect_y:
    .byte 0, 0x19

    org 	$FFFA
irqvectors:
				.word reset          	; NMI
				.word reset          	; RESET
				.word reset          	; IRQ
