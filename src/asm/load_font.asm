GLOBAL load_font
EXTERN open_file
EXTERN read_u16
EXTERN read_u8

file: db 4

-- [cstr path, _] -> [_, _]
-- set patterns in the file
-- but nt uset the others
-- ERRORS:
-- crash [0, 0xFFFF] if file not found
-- crash [0, 0xFFFA] if file is not a font
load_font:
  RAM_B file CALL open_file
  CMPA JMPRZ $not_found

  RAM_A file CALL read_u16 RAM_B "FO" SUB JMPRNZ $not_font
  RAM_A file CALL read_u16 RAM_B "NT" SUB JMPRNZ $not_font

  RAM_A file CALL read_u16
pattern_loop:
  PUSHA
  -- ^ count
  RAM_A file CALL read_u8 SHL SHL SHL RAM_B 0x8000 SUM
  PUSHA RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW INCB
  PUSHB RAM_A file CALL read_u8 POPB DRW
  POPA DECA JMPRNZ $pattern_loop

  RET

not_found:
  HLT

not_font:
  RAM_B 0xFFFA
  HLT
