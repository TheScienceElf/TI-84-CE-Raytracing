section .text
public _fp_mul
; Multiplies two Fixed24 values and returns the new value in HL.
; We only need bits 12 to 35 of the result, but for simplicity we take the
; bits 8 to 39 of the result and extract the middle 24 bits of that.
; This means we do a 24x24->40 multiplication with the low 8 bits discarded.
_fp_mul:
  ; Align iy to the arguments in the stack
  ld iy, 3
  add iy, sp

  ; Perform each of the component-wise multiplications
  ld de, (iy)
  ld bc, (iy + 3)

  ; First work with the multiplications scaled by 2^24 (only a 16-bit result)
  ; Multiply AE
  ld a, (iy + 2)
  ld h, a
  ld l, b
  mlt hl
  ; Check if x is negative
  bit 7, a
  jr z, x_is_positive
  ; Adjust upper result by -y
  sbc hl, bc
  or a, a
x_is_positive:

  ld b, (iy + 5)
  ; Check if y is negative
  bit 7, b
  jr z, y_is_positive
  ; Adjust upper result by -x
  sbc hl, de
y_is_positive:
  ; Multiply BD
  ld e, b
  mlt de
  add hl, de

  ; Multiply AD (only low 8 bits of result needed, this is scaled by 2^36)
  ld d, a
  ld e, b
  mlt de
  ld d, a
  ld a, e
  add a, h
  ld h, a
  ; Save upper 16 bits of result
  push hl

  ; Now work with the multiplications scaled by 2^16
  ; Multiply AF
  ld e, c
  mlt de

  ; Multiply CD
  ld h, b
  ld bc, (iy)
  ld l, c
  mlt hl
  add hl, de

  ; Multiply BE
  ld d, b
  ld iy, (iy + 3)
  ld e, iyh
  mlt de
  add hl, de

  ; Combine with upper 16 bits left-shifted by 8
  dec sp
  pop de
  inc sp
  ld e, 0
  add hl, de

  ; Go ahead and left-shift this intermediate result by 4
  ; That puts it in the correct position for the final result
  add hl, hl
  add hl, hl
  add hl, hl
  add hl, hl

  ; Now work with the multiplications scaled by 2^8
  ; Multiply CF and shift right by 8
  ld d, c
  ld a, b
  ld b, iyl
  mlt bc
  ld c, b
  ld b, e

  ; Multiply CE
  ld e, iyh
  mlt de
  ex de, hl
  ; CE + (CF >> 8) cannot exceed 16 bits
  add hl, bc

  ; Multiply BF
  ld b, a
  ld c, iyl
  mlt bc
  ; Detect carry from 16-bit addition
  add.s hl, bc

  ; Shift right by 4 for the final result positioning
  ld a, l
  rr h
  rra
  srl h
  rra
  srl h
  rra
  srl h
  rra
  ld l, a

  ; Add with prior intermediate result, rounding to nearest
  adc hl, de
  ret

section .text
public _fp_sqr
; Computes the square of a Fixed24 value and returns the new value in HL
; This code is just a specialized case of the above code with a few conditions
; and redundant calculations removed  
_fp_sqr:
  ; Grab the parameter from the stack
  ld hl, 3
  add hl, sp
  ld bc, (hl)
  inc hl
  inc hl
  ld a, (hl)

  ; First work with the multiplications scaled by 2^24 (only a 16-bit result)
  ; Multiply AB*2
  ld h, a
  ld l, b
  mlt hl
  ; Check if x is negative
  bit 7, a
  jr z, x_is_positive_sqr
  ; Adjust upper result by -x*2
  sbc hl, bc
x_is_positive_sqr:
  add hl, hl
  ; Multiply AA (only low 8 bits of result needed, this is scaled by 2^36)
  ld d, a
  ld e, a
  mlt de
  ld d, a
  ld a, e
  add a, h
  ld h, a
  ; Save upper 16 bits of result
  push hl

  ; Now work with the multiplications scaled by 2^16
  ; Multiply BB
  ld h, b
  ld l, b
  mlt hl
  ; Multiply AC*2
  ld e, c
  mlt de
  add hl, de
  add hl, de

  ; Combine with upper 16 bits left-shifted by 8
  dec sp
  pop de
  inc sp
  xor a, a
  ld e, a
  add hl, de

  ; Go ahead and left-shift this intermediate result by 4
  ; That puts it in the correct position for the final result
  add hl, hl
  add hl, hl
  add hl, hl
  add hl, hl
  ex de, hl

  ; Now work with the multiplications scaled by 2^8
  ; Multiply CC and shift right by 9
  ld h, c
  ld l, c
  mlt hl
  ld l, h
  ld h, a
  srl l

  ; Multiply BC*2, shifted right by 1
  mlt bc
  add hl, bc

  ; Shift right by 3 more for the final result positioning
  ld a, l
  srl h
  rra
  srl h
  rra
  srl h
  rra
  ld l, a

  ; Add with prior intermediate result, rounding to nearest
  adc hl, de
  ret
