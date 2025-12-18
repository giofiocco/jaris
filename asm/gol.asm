GLOBAL _start

EXTERN put_char
EXTERN print_int
EXTERN mul
EXTERN exit

{ N 0x05 }

grid:
  0x00 0x01 0x00 0x00 0x00
  0x00 0x00 0x01 0x00 0x00
  0x01 0x01 0x01 0x00 0x00
  0x00 0x00 0x00 0x00 0x00
  0x00 0x00 0x00 0x00 0x00
row:
  0x00 0x00 0x00 0x00 0x00

_start:
  CALLR $print_grid
  CALLR $iteration
  RAM_AL 0x00 CALL exit

-- [_, _] -> [_, _]
print_grid:
  RAM_A grid PUSHA
  RAM_AL N PUSHA
  RAM_AL N PUSHA
print_loop:
  -- ^ j i gridi
  PEEKAR 0x06 A_B INCA PUSHAR 0x06 rB_AL CALL put_char
  PEEKAR 0x04 DECA JMPRZ $print_newline
  PUSHAR 0x04 JMPR $print_loop
print_newline:
  RAM_AL 0x0A CALL put_char
  RAM_AL N PUSHAR 0x04
  POPA DECA PUSHA CMPA JMPRNZ $print_loop

  INCSP INCSP INCSP RET

-- [_, _] -> [_, _]
iteration:
  -- CLEAR ROW

  RAM_AL 0x00 PUSHA PUSHA
loop:
  -- ^ col row
  PEEKAR 0x04 RAM_BL N CALL mul PEEKB SUM PUSHA
  -- ^ i col row
  PEEKB RAM_A grid SUM rB_AL PUSHA
  -- ^ grid[i] i col row
  PEEKAR 0x06 RAM_B row SUM A_B PEEKA AL_rB -- row[col] = grid[i]
  -- ^ grid[i] i col row

  RAM_AL 0x00 PUSHA
  -- ^ n grid[i] i col row
  -- USE THE ROW IF ROW - 1
  PEEKAR 0x08 DECA A_B PEEKAR 0x06 DECA CALLR $at POPB SUM PUSHA -- n += at(row-1, col-1)
  PEEKAR 0x08 DECA A_B PEEKAR 0x06 CALLR $at POPB SUM PUSHA      -- n += at(row-1, col)
  PEEKAR 0x08 DECA A_B PEEKAR 0x06 INCA CALLR $at POPB SUM PUSHA -- n += at(row-1, col+1)
  PEEKAR 0x08 A_B PEEKAR 0x06 DECA CALLR $at POPB SUM PUSHA      -- n += at(row, col-1)
  PEEKAR 0x08 A_B PEEKAR 0x06 INCA CALLR $at POPB SUM PUSHA      -- n += at(row, col+1)
  PEEKAR 0x08 INCA A_B PEEKAR 0x06 DECA CALLR $at POPB SUM PUSHA -- n += at(row+1, col-1)
  PEEKAR 0x08 INCA A_B PEEKAR 0x06 CALLR $at POPB SUM PUSHA      -- n += at(row+1, col)
  PEEKAR 0x08 INCA A_B PEEKAR 0x06 INCA CALLR $at POPB SUM PUSHA -- n += at(row+1, col+1)

--   PEEKAR 0x04 DECA JMPRZ $alive
--   POPB RAM_AL 0x03 SUB JMPRNZ $next
--   PEEKAR 0x04 RAM_B grid SUM A_B RAM_AL 0x01 AL_rB
--   JMPR $ok
-- alive:
-- 
-- next:
-- 
--   POPA INCA PUSHA A_B RAM_AL N SUB JMPRN $loop
--   RAM_AL 0x0A CALL put_char
--   INCSP RAM_AL 0x00 PUSHA
--   PEEKAR 0x04 INCA PUSHAR 0x04 A_B RAM_AL N SUB JMPRN $loop

  RET

-- [row, col] -> [grid[row*N+col] or 0, _]
at:
  PUSHB
  RAM_BL N CALL mul POPB SUM
  A_B RAM_A grid SUM
  A_B rB_AL
  RET
