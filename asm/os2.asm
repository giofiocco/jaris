GLOBAL _start
EXTERN mul

_start:
  RAM_AL 0x03 
  RAM_BL 0x02
  CALL mul
  HLT
