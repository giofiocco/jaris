GLOBAL read_u8
GLOBAL read_u16

  { next_sec_index 0x01 }
  { max_ndx_index 0x03 }
  { data_start 0x04 }

-- [FILE *file, _] -> [char, _]
-- read a u8 from the file
-- [0xFFFF, _] if EOF
read_u8:
  A_B INCA INCA PUSHA 
  -- ^ &ndx [_, file]
  rB_A A_SEC
  PEEKB rB_AL A_NDX PUSHA 
  B_A INCA A_B rB_AL POPB
  -- ^ &ndx [max_ndx, ndx]
  SUB DECA JMPRZ $return_null -- if (ndx - max_ndx - 1 == 0) goto return_null
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
  RAM_NDX next_sec_index MEM_A INCNDX MEM_AH 
  INCA JMPRNZ $next_file
  JMPR $eof

next_file:
  -- &ndx [next_sec + 1, _]
  DECA A_SEC
  RAM_NDX max_ndx_index 

  RAM_AL data_start POPB AL_rB
  B_A INCA A_B
  MEM_A AL_rB
  RAM_AL 0x03 SUB A_B
  SEC_A A_rB
  -- ^ [_, &file]

  B_A JMPR $read_u8

-- [FILE *file, _] -> [char, _]
-- read a u16 from the file
-- if the file has only one char left that is returned
-- [0xFFFF, _] if EOF
read_u16:
  PUSHA
  CALLR $read_u8
  INCA JMPRZ $eof
  DECA
  PUSHA
  PEEKAR 0x04
  CALLR $read_u8
  INCA JMPRZ $only_first
  DECA 
  A_B POPA B_AH
  INCSP RET

only_first:
  -- ^ first_char _
  POPA INCSP RET

eof:
  -- ^ _
  RAM_A 0xFFFF
  INCSP RET
