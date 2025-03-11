GLOBAL print
GLOBAL put_char
GLOBAL print_int

EXTERN div

  { current_process_ptr 0xF802 }
  { stdout_offset 0x06 }
  { screen_text_row_ptr 0xF80A }
  { screen_text_col_ptr 0xF80B }

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
-- crash [0xEFFA, _] if stdout ridirect
-- TODO: wrap screen if row too low
put_char:
  PUSHA

  RAM_B current_process_ptr rB_A RAM_BL stdout_offset SUM A_B rB_A
  INCA JMPRNZ $ridirect

  PEEKA RAM_BL 0x0A SUB JMPRNZ $no_new_line
  RAM_B screen_text_col_ptr RAM_AL 0x00 AL_rB
  RAM_B screen_text_row_ptr rB_AL INCA AL_rB
  INCSP RET
no_new_line:

  RAM_B screen_text_row_ptr rB_AL PUSHA
  RAM_B screen_text_col_ptr rB_AL POPB B_AH

  A_B POPA DRW

  RAM_B screen_text_col_ptr rB_AL INCA AL_rB
  RAM_BL 0x64 SUB JMPRNZ $end_new_line
  RAM_B screen_text_col_ptr RAM_AL 0x00 AL_rB
  RAM_B screen_text_row_ptr rB_AL INCA AL_rB
end_new_line:
  RET

ridirect:
  -- ^ char [stdout_ptr + 1, _]
  RAM_A 0xEFFA HLT

-- [char, _] -> [_, _]
-- crash [0xEFFA, _] if stdout is full
-- put_char:
--   PUSHA
--   RAM_B stdout_ptr_ptr rB_A PUSHA -- push &next_char_ptr
--   A_B rB_A -- next_char_ptr
--   -- ^ &next_char_ptr char [next_char_ptr, _]
--   A_B PEEKAR 0x04 AL_rB
-- 
--   B_A INCA PEEKB A_rB -- *(&next_char_ptr) ++
--   -- ^ &next_char_ptr char [next_char_ptr, _]
-- 
--   A_B POPA INCA INCA PUSHB 
--   -- ^ next_char_ptr char [&ending_char_ptr, _]
--   A_B rB_A POPB INCSP
--   -- ^ [ending_char_ptr, next_char_ptr]
--   SUB JMPRN $end -- if (next_char_ptr - ending_char_ptr < 0) goto end
-- 
--   -- stdout full
--   RAM_A 0xEFFA
-- end:
--   RET

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
