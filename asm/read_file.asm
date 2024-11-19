GLOBAL read_u8
GLOBAL read_u16

  { next_sec_index 0x01 }
  { max_ndx_index 0x03 }
  { data_start 0x04 }

-- [FILE *file, _] -> [char, _]
-- read a u8 from the file
-- [0xFFFF, _] if EOF
read_u8:
  A_B rB_A A_SEC
  INCB INCB rB_AL PUSHB PUSHA
  -- ^ ndx &ndx
  INCB rB_AL
  POPB SUB DECA JMPRZ $next_file -- if next_char == max_char
  B_A A_NDX MEM_A PUSHA -- [char, ndx]
  -- ^ char &ndx
  PUSHB
  -- ^ ndx char &ndx
  PEEKAR 0x06 A_B POPA INCA AL_rB -- *ndx = ndx + 1

  POPA INCSP RET

next_file: -- next_char == max_char
  -- ^ &ndx [_, ndx]
  B_A A_NDX MEM_A PUSHA
  -- ^ char &ndx
  RAM_NDX next_sec_index MEM_A INCNDX MEM_AH
  INCA JMPRZ $eof DECA A_SEC -- if next_sec == 0xFFFF goto eof
  PEEKAR 0x04 DECA DECA A_B SEC_A A_rB -- set file->sec
  INCB INCB RAM_AL 0x04 AL_rB -- file->ndx = 4
  INCB RAM_NDX max_ndx_index MEM_A AL_rB -- set file->max_ndx

  POPA INCSP RET

-- [FILE *file, _] -> [char, _]
-- read a u16 from the file
-- if the file has only one char left that is returned
-- [0xFFFF, _] if EOF
read_u16:
  PUSHA
  CALLR $read_u8
  PUSHA
  INCA JMPRZ $eof
  DECA
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
  -- ^ _ _
  RAM_A 0xFFFF
  INCSP INCSP RET
