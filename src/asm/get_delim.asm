GLOBAL get_delim
EXTERN get_char

-- [char delim, u8 *ptr] -> [u16 length, _]
-- (length without the trailing \0)
-- ERROR: -> if char is end-of-file [0xFFFF, _] 
get_delim:
  PUSHA PUSHB PUSHB
loop:
  -- ^ ptr ptr_start delim
  CALL get_char 
  CMPA JMPRZ $end
  INCA JMPRZ $end
  DECA A_B 
  PEEKAR 0x06 SUB JMPRZ $delim
  RAM_AL 0x0A SUB JMPRZ $delim
  B_A POPB AL_rB
  B_A INCA PUSHA
  JMPR $loop

delim:
  -- ^ ptr ptr_start delim
  RAM_AL 0x00
  POPB AL_rB
  POPA SUB
  INCSP RET

end:
  -- ^ ptr ptr_start delim
  RAM_A 0xFFFF
  INCSP INCSP INCSP RET
