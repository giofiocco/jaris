GLOBAL print
GLOBAL print_with_len
EXTERN put_char

  { stdout_ptr_ptr 0xF80A }

-- [cstr ptr, _] -> [_, _]
print:
  PUSHA A_B rB_AL
  CMPA JMPRZ $end
  CALL put_char
  POPA INCA
  JMPR $print

end:
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
