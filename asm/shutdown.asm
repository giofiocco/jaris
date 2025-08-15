GLOBAL _start

EXTERN print

exit_string: "BYE" 0x0A 0x00

_start:
  RAM_A exit_string CALL print
  RAM_A 0xA0A0
  HLT
