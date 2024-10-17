GLOBAL put_char

  { stdout_ptr_ptr 0xF80A }

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
