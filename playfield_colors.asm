; https://www.masswerk.at/6502/assembler.html
; https://manpages.ubuntu.com/manpages/jammy/man1/xa.1.html
; xa playfield_colors.asm -bt61440  -o playfield_colors.bin

; xa -bt61440 playfield_colors.asm -o playfield_colors.bin && ./set_start_address.py playfield_colors.bin && stella playfield_colors_out.bin

VSYNC = $00
VBLANK = $01
WSYNC = $02
RSYNC = $03
NUSZ0 = $04
NUSZ1 = $05

COLUP0 = $06
COLUP1 = $07
COLUPF = $08
COLUBK = $09
CTRLPF = $0A

PF0 = $0D
PF1 = $0E
PF2 = $0F

RESP0 = $10
RESP1 = $11

GRP0 = $1B
GRP1 = $1C

lda $1234,x

ldx #0
lda #0
clear_mem:
    sta 0,x
    dex
    bne clear_mem

lda #$8E    ; white
sta COLUPF

;lda #$B4  ; grey
;sta COLUBK

lda #$F0
sta PF0

lda #$75
sta PF1

lda #$35
sta PF1

lda #1
sta CTRLPF

lda #$C1
sta GRP0

lda #$1E  ; yellow NTSC
sta COLUP0

;lda #3
;sta NUSZ0

;lda #5
;sta NUSZ1

lda #$A3
sta GRP1

lda #$56  ; magenta NTSC
sta COLUP1

draw_screen_start:

lda #0
sta VBLANK

; start vertical sync
lda #2
sta VSYNC
; wait for 3 lines
sta WSYNC
sta WSYNC
sta WSYNC
; stop veritcal sync
lda #0
sta VSYNC

; 37 vertical
ldx #37
-vblank_wait:
    sta WSYNC
    dex
    bne vblank_wait

; P0 is yellow
; P1 is magenta
sta RESP1
sta RESP0

; 192 scan lines
; start with color value other than 0
ldx #16
-scan_line_loop:
    txa
    asl
    sta COLUBK
    sta WSYNC
    inx
    cpx #104  ; 192 + 16
    bne scan_line_loop

-scan_line_loop:
    txa
    asl
    sta COLUBK

    lda #$81
    sta GRP0
    nop
    lda #$99
    sta GRP0
    nop
    lda #$C3
    sta GRP0

    sta WSYNC
    inx
    cpx #208  ; 192 + 16
    bne scan_line_loop

lda #%01000010
sta VBLANK

; 30 overscan
ldx #30
-vblank_wait:
    sta WSYNC
    dex
    bne vblank_wait

jmp draw_screen_start

; start program at 0xF000
; this value is stored at 0xFFFC
* = $FFFC
.word $F000