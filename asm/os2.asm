GLOBAL _start
EXTERN mul

_start:
  RAM_A 0x1001
  RAM_BL 0x02
  CALL mul
  HLT
