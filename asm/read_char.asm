GLOBAL read_char

-- [FILE *file, _] -> [char, _]
-- read a char from the file
-- [0xFFFF, _] if EOF
read_char:
  A_B INCA INCA PUSHA 
  -- ^ &ndx [_, &file]
  rB_A A_SEC
  PEEKB rB_AL A_NDX PUSHA 
  B_A INCA A_B rB_AL POPB
  -- ^ &ndx [max_ndx, ndx]
  SUB JMPRZ $return_null -- if (ndx == max_ndx) goto return_null
  -- ^ &ndx [_, ndx]
  MEM_A PUSHA
  PUSHB
  -- ^ ndx *mem &ndx 
  PEEKAR 0x06 A_B POPA INCA 
  -- ^ *mem &ndx [ndx+1, &ndx]
  AL_rB -- ndx += 1

  POPA INCSP RET

return_null:
  -- ^ &ndx
  RAM_A 0xFFFF
  INCSP RET
