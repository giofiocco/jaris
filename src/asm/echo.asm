GLOBAL _start
EXTERN print
EXTERN put_char
EXTERN exit

_start:
  B_A CALL print
  RAM_AL 0x0A CALL put_char
  RAM_AL 0x00 CALL exit
