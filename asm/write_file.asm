GLOBAL write_u8
GLOBAL write_u16

  { max_ndx_index 0x03 }

-- [FILE *file, u8 a] -> [file, _]
-- write u8 to the file
-- CRASH [3402, _] if new file sec needed
write_u8:
  PUSHA PUSHB
  -- ^ a file [file, _]
  A_B rB_A A_SEC
  INCB INCB rB_AL A_B RAM_AL 0xFF SUB JMPRZ $new_file_sec -- check if ndx == 0xFF
  -- ^ a file [_, ndx]
  B_A A_NDX
  POPA A_MEM
  -- ^ file
  PEEKB INCB INCB rB_AL INCA AL_rB -- inc ndx
  PUSHA
  RAM_NDX max_ndx_index MEM_A A_B POPA SUB JMPRNN $not_inc_max
  B_A INCA A_MEM -- inc max_ndx in file sec
not_inc_max:
  -- ^ file
  POPA RET

new_file_sec:
  -- ^ a file
  RAM_A 0x3402 HLT

-- [FILE *file, u16 a] -> [file, _]
-- write u16 to the file
write_u16:
  PUSHA PUSHB
  -- ^ a file
  CALLR $write_u8
  POPA SHR SHR SHR SHR SHR SHR SHR SHR A_B PEEKA CALLR $write_u8
  POPA RET

