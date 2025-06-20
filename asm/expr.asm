GLOBAL _start
EXTERN exit
EXTERN print
EXTERN put_char
EXTERN print_int
EXTERN is_digit

{ T_NONE  0x00 }
{ T_INT   0x01 }
{ T_PLUS  0x02 }
{ T_MINUS 0x03 }
{ T_MAX   0x03 }

-- Grammar:
-- {...} 0 or more times
--
-- E -> F { T_MUL F  | T_DIV F }
-- F -> T { T_PLUS T | T_MINUS T }
-- T -> A
-- A -> INT

string_NONE: "T_NONE" 0x00
string_INT: "T_INT" 0x00
string_PLUS: "T_PLUS" 0x00
string_MINUS: "T_MINUS" 0x00

-- [int kind, _] -> []
print_token_kind:
  RAM_BL T_MAX SUB JMPRN $unknown_kind
  A_B SHL SHL SUM SHL -- kind * 10
ALIGN
  NOP JMPAR
  NOP RAM_A string_NONE  NOP CALL print RET
  NOP RAM_A string_INT   NOP CALL print RET
  NOP RAM_A string_PLUS  NOP CALL print RET
  NOP RAM_A string_MINUS NOP CALL print RET

unknown_kind_string: "ERROR: unknown kind" 0x0A 0x00
unknown_kind:
  RAM_A unknown_kind_string CALL print
  RAM_AL 0x01 CALL exit

-- [int kind, int arg] -> []
print_token:
  PUSHB PUSHA
  RAM_AL "(" CALL put_char
  POPA CALLR $print_token_kind
  RAM_AL " " CALL put_char
  POPA CALL print_int
  RAM_AL ")" CALL put_char
  RET

-- [cstr *expr, _] -> [int kind, int arg]
token_next:
  PUSHA A_B rB_A A_B rB_AL CMPA JMPRZ $_0
  -- ^ &expr [char, _]
  POPB PUSHA
  rB_A INCA A_rB -- (&expr) ++
  POPA PUSHB
  -- ^ &expr [char, _]
  A_B

  RAM_AL " " SUB JMPRNZ $_3
  POPA CALLR $token_next RET
_3:
  -- ^ &expr
  RAM_AL "+" SUB JMPRNZ $_2
  INCSP RAM_AL T_PLUS RAM_BL 0x00 RET
_2:
  -- ^ &expr
  RAM_AL "-" SUB JMPRNZ $_1
  INCSP RAM_AL T_MINUS RAM_BL 0x00 RET
_1:
  -- ^ &expr [_, char]
  B_A CALL is_digit CMPA JMPRN $_0
  PUSHA
lex_int:
  -- ^ num &expr
  PEEKAR 0x04 A_B rB_A A_B rB_AL
  CALL is_digit CMPA JMPRN $end_int
  POPB PUSHA
  -- ^ digit &expr [_, num]
  B_A SHL SHL SUM SHL -- num *= 10
  POPB SUM PUSHA -- num += digit
  -- ^ num &expr
  PEEKAR 0x04 A_B rB_A INCA A_rB -- (&expr) ++
  JMPR $lex_int

end_int:
  -- ^ num &expr
  RAM_AL T_INT POPB INCSP RET
_0:
  -- ^ &expr
  INCSP RAM_AL T_NONE RAM_BL 0x00 RET

-- [cstr *expr, int kind] -> [int kind, int arg]
-- return kind=0 if token is not of the expected kind
token_next_if_kind:
  PUSHB PUSHA
  A_B rB_A PUSHA
  -- ^ expr &expr expected_kind
  PEEKAR 0x04 CALLR $token_next
  PUSHB
  -- ^ token_arg expr &expr expected_kind
  A_B PEEKAR 0x08 SUB JMPRZ $token_next_if_kind_ok
  INCSP POPA POPB A_rB INCSP RET
token_next_if_kind_ok:
  -- ^ token_arg expr &expr expected_kind
  B_A POPA INCSP INCSP INCSP RET

expected_token_string1: "ERROR: expected " 0x00
expected_token_string2: ", found " 0x00

-- [cstr *expr, int kind] -> [int kind, int arg]
-- throw error if wrong kind
token_expected:
  PUSHB
  CALLR $token_next
  CALLR $print_token HLT
  PUSHB
  A_B PEEKAR 0x04 SUB JMPRZ $token_expected_ok
  PUSHB
  RAM_A expected_token_string1 CALL print
  -- ^ foundkind arg kind
  PEEKAR 0x06 CALLR $print_token_kind
  RAM_A expected_token_string2 CALL print
  POPA CALLR $print_token_kind
  RAM_AL 0x0A CALL put_char
  RAM_AL 0x01 CALL exit
token_expected_ok:
  -- ^ arg kind
  POPB POPA RET

expected_int_string: "ERROR: expected int" 0x0A 0x00
expected_int:
  RAM_A expected_int_string CALL print
  RAM_AL 0x01 CALL exit

-- [cstr *expr, _] -> [int result, _]
parse_A:
  RAM_BL T_INT CALLR $token_expected
  RET

-- [cstr *expr, _] -> [int result, _]
parse_T:
  CALLR $parse_A RET

-- [cstr *expr, _] -> [int result, _]
parse_F:
  PUSHA
  CALLR $parse_T PUSHA
  PEEKAR 0x04 RAM_BL T_PLUS CALLR $token_next_if_kind
  CMPA JMPRZ $if_minus
  HLT
if_minus:
  HLT

  INCSP RET

-- [cstr *expr, _] -> [int result, _]
parse_E:
  CALLR $parse_F RET

-- Usage: expr <math expression>
-- compute the expression
-- supported:
--   int, plus, minus
_start:
  PUSHB
  -- ^ expr
  SP_A INCA INCA CALLR $parse_E
  -- TODO: check if something left
  CALL print_int

  RAM_AL 0x0A CALL put_char

  RAM_AL 0x00 CALL exit
