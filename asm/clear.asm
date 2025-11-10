GLOBAL _start

EXTERN exit
EXTERN put_char
EXTERN set_cursor

_start:
  RAM_AL 0x00 A_B CALL set_cursor
  RAM_A 0x0D70 -- number of 8x8 px
loop:
  PUSHA
  RAM_AL 0x00 CALL put_char
  POPA DECA JMPRNZ $loop

  RAM_AL 0x00 CALL exit
