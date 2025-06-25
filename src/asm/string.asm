GLOBAL str_find_char
GLOBAL is_digit

-- [cstr str, char c] -> [char *ptr, _]
-- returns the pointer of the first occurrence of c in str
-- [0, _] if c not found
str_find_char:
  PUSHB PUSHA
search_char:
  -- ^ stri c
  POPB rB_AL -- A = *stri
  CMPA JMPRZ $char_not_found
  INCB PUSHB -- stri ++
  A_B PEEKAR 0x04 SUB JMPRNZ $search_char
  -- ^ stri c
  POPA DECA INCSP RET

char_not_found:
  -- ^ c
  RAM_AL 0x00
  INCSP RET

-- [u8 c, _] -> [int, _]
-- returns the number if is digit, negative otherwise
is_digit:
  A_B RAM_AL "0" SUB
  RAM_BL 0x09 SUB
  JMPRN $is_digit_end
  RAM_BL 0x09 SUB
is_digit_end:
  RET
