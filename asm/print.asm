GLOBAL print
GLOBAL put_char
GLOBAL print_int

EXTERN div
EXTERN stream_write

  { current_process_ptr 0xF802 }
  { stdout_offset 0x06 }
  { max_cols 0x4F }
  { max_rows 0x2C }

ALIGN
pos_ptr:
col_ptr: 0x00
row_ptr: 0x00

-- [char, _] -> [_, _]
put_char:
  PUSHA

  RAM_B current_process_ptr rB_A RAM_BL stdout_offset SUM A_B rB_A
  INCA JMPRZ $no_redirect
  -- ^ char [stdout_ptr + 1, _]
  DECA POPB CALL stream_write RET
no_redirect:
  PEEKA RAM_BL 0x0A SUB JMPRNZ $no_new_line
  -- ^ char
  INCSP JMPR $new_line
no_new_line:
  RAM_B pos_ptr rB_A
  A_B POPA DRW

  RAM_B col_ptr rB_AL INCA AL_rB
  RAM_BL max_cols SUB JMPRN $new_line
  RET
new_line:
  RAM_B row_ptr rB_AL INCA AL_rB
  A_B RAM_AL max_rows SUB JMPRN $end_new_line -- if (row < max_rows) goto end_new_line => row - max_rows
  RAM_B row_ptr RAM_AL 0x00 AL_rB
end_new_line:
  RAM_B col_ptr RAM_AL 0x00 AL_rB
  RAM_AL max_cols
clear_line:
  PUSHA
  -- ^ i
  RAM_B pos_ptr rB_A A_B RAM_AL 0x00 DRW
  RAM_B col_ptr rB_AL INCA AL_rB
  POPA DECA JMPRNZ $clear_line
  RAM_B col_ptr RAM_AL 0x00 AL_rB
  RET

-- [cstr ptr, _] -> [_, _]
print:
  A_B rB_AL CMPA JMPRNZ $not_null
  RET
not_null:
  PUSHB
  CALL put_char
  POPA INCA JMPR $print

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
