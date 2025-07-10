GLOBAL _start
EXTERN exit

ALIGN
code: db 16

_start:
  RAM_B code RAM_AL RAM_AL AL_rB
  INCB RAM_AL 0x01 AL_rB
  INCB INCB RAM_AL CALL AL_rB
  INCB RAM_A exit A_rB

  JMP code

  RAM_AL 0x00 CALL exit
