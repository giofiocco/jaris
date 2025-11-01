GLOBAL _start

EXTERN exit
EXTERN put_char

{ iterations 0x10 }
row: db 10
row_end:
table: 0x00 0x01 0x01 0x01 0x00 0x01 0x01 0x00

print_row:
  RAM_A row PUSHA
  RAM_B row_end SUB
print_loop:
  PUSHA
  -- ^ i rowi
  PEEKAR 0x04 A_B rB_AL CALL put_char
  PEEKAR 0x04 INCA PUSHAR 0x04
  POPA DECA JMPRNZ $print_loop
  RAM_AL "|" CALL put_char
  RAM_AL 0x0A CALL put_char
  INCSP RET

_start:
  RAM_A row_end DECA A_B RAM_AL 0x01 AL_rB

  RAM_B row_end RAM_A row SUB DECA PUSHA -- i: iteration for the row loop, i_ the persisten one, so is not computed every iteration
  RAM_AL iterations
iter:
  PUSHA
  -- ^ iter i_
  PEEKAR 0x04 PUSHA -- i
  RAM_A row_end DECA A_B rB_AL PUSHA -- prev
  RAM_B row rB_AL PUSHA PUSHA -- row0 and current
  PUSHB -- rowi
  -- ^ rowi current row0 prev i iter i_

  CALL print_row

loop:
  -- ^ rowi current row0 prev i iter i_
  PEEKAR 0x08 SHL SHL -- n = prev << 2
  A_B PEEKAR 0x04 SHL SUM PUSHA -- n += current << 1
  -- ^ n rowi current row0 prev i iter i_
  PEEKAR 0x04 INCA A_B rB_AL POPB SUM -- n += *(rowi + 1)
  -- ^ rowi current row0 prev i iter i_ [n, _]
  RAM_B table SUM A_B rB_AL PEEKB AL_rB -- *rowi = table[n]

  -- ^ rowi current row0 prev i iter i_
  PEEKAR 0x04 PUSHAR 0x08 -- prev = current
  POPA INCA PUSHA -- rowi ++
  A_B rB_AL PUSHAR 0x04 -- current = *rowi
  PEEKAR 0x0A DECA PUSHAR 0x0A -- i --
  CMPA JMPRNZ $loop

  -- ^ rowi current row0 prev i iter i_
  PEEKAR 0x08 SHL SHL -- n = prev << 2
  A_B PEEKAR 0x04 SHL SUM -- n += current << 1
  A_B PEEKAR 0x06 SUM -- n += row0
  RAM_B table SUM A_B rB_AL PEEKB AL_rB -- *rowi = table[n]

  -- ^ rowi current row0 prev i iter i_
  INCSP INCSP INCSP INCSP INCSP
  POPA DECA JMPRNZ $iter
  INCSP

  RAM_AL 0x00 CALL exit
