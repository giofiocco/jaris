GLOBAL str_find_char
GLOBAL is_digit
GLOBAL path_find_name
GLOBAL parse_int
GLOBAL str_eq

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

-- [cstr path] -> [char *ptr, _]
-- returns the pointer to the start of the name (of file or dir) in path
-- ie: 'a/b/file.txt'
--          ^ ptr
path_find_name:
  PUSHA PUSHA
loop:
  -- ^ ptr path
  PEEKB rB_AL CMPA JMPRZ $end_loop
  A_B RAM_AL "/" SUB JMPRNZ $not_slash
  PEEKA INCA PUSHAR 0x04
not_slash:
  POPA INCA PUSHA JMPR $loop

end_loop:
  -- ^ ptr path
  INCSP POPA RET

-- [cstr str, _] -> [u16 num, _]
-- ERRORS:
-- [0xFFFF, 0] if invalid
parse_int:
  RAM_BL 0x00 PUSHB
parse_int_loop:
  PUSHA
  -- ^ str num
  A_B rB_AL CMPA JMPRZ $parse_int_end
  CALLR $is_digit CMPA JMPRN $parse_int_err
  PUSHA
  -- ^ new_digit str num
  PEEKAR 0x06 A_B SHL SHL SUM SHL -- num * 10
  POPB SUM PUSHAR 0x04 -- num = num * 10 + new_digit
  -- ^ str num
  POPA INCA JMPR $parse_int_loop
parse_int_end:
  -- ^ str num
  INCSP POPA RET

parse_int_err:
  -- ^ _ _
  RAM_A 0xFFFF
  INCSP INCSP RET

-- [cstr a, cstr b] -> [res, _]
-- compares 2 str (by pointers)
-- returns 0 if are different, 1 if equal
str_eq:
  PUSHB PUSHA
  -- ^ a b
  rB_AL CMPA JMPRZ $str_eq_null
  -- ^ a b [*b, _]
  PUSHA PEEKAR 0x04
  -- ^ *b a b [a, _]
  A_B rB_AL POPB SUB JMPRNZ $str_eq_diff
  -- ^ a b
  POPA INCA POPB INCB
  -- [a+1, b+1]
  JMPR $str_eq

str_eq_null:
  -- ^ a b [*b, _]
  PEEKB rB_AL CMPA JMPRNZ $str_eq_diff
  INCSP INCSP RAM_AL 0x01 RET
str_eq_diff:
  -- ^ a b
  INCSP INCSP RAM_AL 0x00 RET

