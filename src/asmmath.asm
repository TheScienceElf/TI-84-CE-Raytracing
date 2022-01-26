public _fp_mul
public _fp_sqr

; Multiplies two Fixed24 values and returns the new value in HL
_fp_mul:
  push ix
  push bc
  push iy

  ld iy, $0

  ; Align ix to below the stack. Zero-fill 48 bits
  ld ix, $FFFFFA
  add ix, sp
  ld (ix), iy
  ld (ix + $3), iy

  ; Align iy to the arguments in the stack
  ld iy, $C
  add iy, sp

  ; We use the E register to track whether we need to negate the result
  ld e, $0

  ; Take absolute value of the stack elements
  bit 7, (iy + $2)
  jp z, abs_x_end

  inc e

  ld a, $0
  sub a, (iy + $0)
  ld (iy + $0), a

  ld a, $0
  sbc a, (iy + $1)
  ld (iy + $1), a

  ld a, $0
  sbc a, (iy + $2)
  ld (iy + $2), a
abs_x_end:
  
  bit 7, (iy + $5)
  jp z, abs_y_end

  inc e

  ld a, $0
  sub a, (iy + $3)
  ld (iy + $3), a

  ld a, $0
  sbc a, (iy + $4)
  ld (iy + $4), a

  ld a, $0
  sbc a, (iy + $5)
  ld (iy + $5), a
abs_y_end:


  ; Perform each of the component-wise multiplications
  ; Multiply CF
  ld h, (iy + $0)
  ld l, (iy + $3)
  mlt hl

  ; Shift our answer over by 8 bits
  ld (ix), hl
  ld hl, (ix + $1)

  ; Multiply BF
  ld b, (iy + $1)
  ld c, (iy + $3)
  mlt bc
  add hl, bc

  ; Multiply CE
  ld b, (iy + $0)
  ld c, (iy + $4)
  mlt bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $1), hl
  ld hl, (ix + $2)

  ; Multiply AF
  ld b, (iy + $2)
  ld c, (iy + $3)
  mlt bc
  add hl, bc

  ; Multiply BE
  ld b, (iy + $1)
  ld c, (iy + $4)
  mlt bc
  add hl, bc

  ; Multiply CD
  ld b, (iy + $0)
  ld c, (iy + $5)
  mlt bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $2), hl
  ld hl, (ix + $3)

  ; Multiply BD
  ld b, (iy + $1)
  ld c, (iy + $5)
  mlt bc
  add hl, bc

  ; Multiply AE
  ld b, (iy + $2)
  ld c, (iy + $4)
  mlt bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $3), hl
  ld hl, (ix + $4)

  ; Multiply AD
  ld b, (iy + $2)
  ld c, (iy + $5)
  mlt bc
  add hl, bc

  ld (ix + $4), hl

  ; Grab the last 24 bits of our computation
  ld hl, (ix + $2)
  
  ; Shift hl left by 4 bits
  add hl, hl
  add hl, hl
  add hl, hl
  add hl, hl

  ld (ix + $2), hl
  ld bc, (ix + $2)
  
  ; Grab one byte below our 24 bit chunk
  ld hl, $0
  ld l, (ix + $1)
  
  ; Shift hl right by 4 bits
  srl l
  srl l
  srl l
  srl l

  add hl, bc

  ; Negate the result if necessary
  bit 0, e
  jp z, negate_end

  ld (ix), hl

  ld a, $0
  sub a, (ix + $0)
  ld (ix + $0), a

  ld a, $0
  sbc a, (ix + $1)
  ld (ix + $1), a

  ld a, $0
  sbc a, (ix + $2)
  ld (ix + $2), a

  ld hl, (ix)

negate_end:

  pop iy
  pop bc
  pop ix
  ret


; Computes the square of a Fixed24 value and returns the new value in HL
; This code is just a specialized case of the above code with a few conditions
; and redundant calculations removed  
_fp_sqr:
  push ix
  push bc
  push iy

  ld iy, $0

  ; Align ix to below the stack. Zero-fill 48 bits
  ld ix, $FFFFFA
  add ix, sp
  ld (ix), iy
  ld (ix + $3), iy

  ; Align iy to the arguments in the stack
  ld iy, $C
  add iy, sp

  ; Take absolute value of the stack elements
  bit 7, (iy + $2)
  jp z, abs_x_end_sqr

  ld a, $0
  sub a, (iy + $0)
  ld (iy + $0), a

  ld a, $0
  sbc a, (iy + $1)
  ld (iy + $1), a

  ld a, $0
  sbc a, (iy + $2)
  ld (iy + $2), a
abs_x_end_sqr:


  ; Perform each of the component-wise multiplications
  ; Multiply CC
  ld h, (iy + $0)
  ld l, (iy + $0)
  mlt hl

  ; Shift our answer over by 8 bits
  ld (ix), hl
  ld hl, (ix + $1)

  ; Multiply 2 * BC
  ld b, (iy + $1)
  ld c, (iy + $0)
  mlt bc
  add hl, bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $1), hl
  ld hl, (ix + $2)

  ; Multiply 2 * AC
  ld b, (iy + $2)
  ld c, (iy + $0)
  mlt bc
  add hl, bc
  add hl, bc

  ; Multiply BB
  ld b, (iy + $1)
  ld c, (iy + $1)
  mlt bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $2), hl
  ld hl, (ix + $3)

  ; Multiply 2 * AB
  ld b, (iy + $1)
  ld c, (iy + $2)
  mlt bc
  add hl, bc
  add hl, bc

  ; Shift our answer over by 8 bits
  ld (ix + $3), hl
  ld hl, (ix + $4)

  ; Multiply AA
  ld b, (iy + $2)
  ld c, (iy + $2)
  mlt bc
  add hl, bc

  ld (ix + $4), hl

  ; Grab the last 24 bits of our computation
  ld hl, (ix + $2)
  
  ; Shift hl left by 4 bits
  add hl, hl
  add hl, hl
  add hl, hl
  add hl, hl

  ld (ix + $2), hl
  ld bc, (ix + $2)
  
  ; Grab one byte below our 24 bit chunk
  ld hl, $0
  ld l, (ix + $1)
  
  ; Shift hl right by 4 bits
  srl l
  srl l
  srl l
  srl l

  add hl, bc

  pop iy
  pop bc
  pop ix
  ret