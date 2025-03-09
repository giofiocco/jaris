GLOBAL print
GLOBAL print_with_len
GLOBAL put_char
GLOBAL print_int

EXTERN div

  { stdout_ptr_ptr 0xF80A }

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

-- [char *ptr, u16 len] -> [0, _]
-- print str with known len
-- crash [0xFFFA, _] if too big
print_with_len:
  PUSHA PUSHB
  -- ^ len ptr

  RAM_B stdout_ptr_ptr rB_A A_B rB_A PUSHA PUSHB B_A -- next
  -- ^ &next next len ptr
  INCA INCA A_B rB_A A_B PEEKAR 0x06 -- [next, ending]
  SUB A_B PEEKAR 0x04 SUB -- ending - next - len
  JMPRN $too_big

  PEEKAR 0x04 A_B PEEKAR 0x06 SUM POPB A_rB -- *next += len

  PEEKAR 0x04
loop:
  -- ^ next len ptr [len, _]
  PUSHAR 0x04

  PEEKAR 0x06 A_B rB_AL PUSHA B_A INCA PUSHAR 0x08
  -- ^ char next len ptr
  POPA POPB AL_rB
  B_A INCA PUSHA

  PEEKAR 0x04 DECA JMPRNZ $loop
  -- ^ next len ptr
  INCSP INCSP INCSP RAM_AL 0x00 RET

too_big:
    -- ^ &next next len ptr
  RAM_A 0xFFFA
  HLT

-- [char, _] -> [_, _]
-- crash [0xEFFA, _] if stdout is full
put_char:
  PUSHA
  RAM_B stdout_ptr_ptr rB_A PUSHA -- push &next_char_ptr
  A_B rB_A -- next_char_ptr
  -- ^ &next_char_ptr char [next_char_ptr, _]
  A_B PEEKAR 0x04 AL_rB

  B_A INCA PEEKB A_rB -- *(&next_char_ptr) ++
  -- ^ &next_char_ptr char [next_char_ptr, _]

  A_B POPA INCA INCA PUSHB 
  -- ^ next_char_ptr char [&ending_char_ptr, _]
  A_B rB_A POPB INCSP
  -- ^ [ending_char_ptr, next_char_ptr]
  SUB JMPRN $end -- if (next_char_ptr - ending_char_ptr < 0) goto end

  -- stdout full
  RAM_A 0xEFFA 
end:
  RET


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

