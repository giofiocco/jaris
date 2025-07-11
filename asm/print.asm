GLOBAL print
GLOBAL put_char
GLOBAL print_int

EXTERN div

  { current_process_ptr 0xF802 }
  { stdout_offset 0x06 }
  { max_cols 0x4A }

ALIGN
pos_ptr:
col_ptr: 0x00
row_ptr: 0x00

-- [cstr ptr, _] -> [_, _]
print:
  PUSHA A_B rB_AL
  CMPA JMPRZ $print_end
  CALL put_char
  POPA INCA
  JMPR $print

print_end:
  -- ^ ptr
  INCSP RET

-- [char, _] -> [_, _]
-- crash [0xEFFA, _] if stdout full
-- TODO: wrap screen if row too low
put_char:
  PUSHA

  RAM_B current_process_ptr rB_A RAM_BL stdout_offset SUM A_B rB_A
  INCA JMPRNZ $ridirect

  PEEKA RAM_BL 0x0A SUB JMPRNZ $no_new_line

  RAM_B col_ptr RAM_AL 0x00 AL_rB
  RAM_B row_ptr rB_AL INCA AL_rB
  INCSP RET
no_new_line:
  RAM_B pos_ptr rB_A
  A_B POPA DRW

  RAM_B col_ptr rB_AL INCA AL_rB
  RAM_BL max_cols SUB JMPRNN $end_new_line
  RAM_B col_ptr RAM_AL 0x00 AL_rB
  RAM_B row_ptr rB_AL INCA AL_rB
end_new_line:
  RET

ridirect:
  -- ^ char [stdout_ptr + 1, _]
  DECA A_B rB_A CMPA JMPRZ $stdout_full
  DECA A_rB
  RAM_AL 0x02 SUM PUSHA A_B rB_A
  -- ^ &next_ptr char
  A_B PEEKAR 0x04 AL_rB -- *next_char = char
  B_A INCA POPB A_rB
  INCSP RET

stdout_full:
  RAM_A 0xEFFA HLT

-- [u16 num, _] -> [_, _]
-- crash [0xEFFA, _] if stdout is full
print_int:
  CMPA JMPRNZ $print_digit
  RAM_AL "0" CALLR $put_char
  RET

print_digit:
  -- [num, _]
  CMPA JMPRNZ $digit_to_char
  RET
digit_to_char:
  A_B RAM_AL 0x0A CALL div
  PUSHB
  CALLR $print_digit
  POPA RAM_BL "0" SUM
  CALLR $put_char
  RET
