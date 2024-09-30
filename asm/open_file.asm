GLOBAL open_file
EXTERN solve_path

  { data_start 0x04 }
  { max_ndx_index 0x03 }

-- [cstr path, FILE *file] -> [_, _]
-- the file struct has to be aligned
-- ERRORS:
-- [0, 0xFFFF] if file not found
-- [0, 0xFFFE] if path is not a file
open_file:
  PUSHB

  CALL solve_path
  CMPA JMPRZ $not_found
  A_SEC
  RAM_NDX 0x00
  MEM_A RAM_BL "F" SUB JMPRNZ $is_not_file

  POPB SEC_A A_rB 
  B_A INCA INCA A_B 
  RAM_AL data_start AL_rB 
  B_A INCA A_B 
  RAM_NDX max_ndx_index MEM_A AL_rB

  RET

not_found:
  -- ^ file [0, _]
  RAM_B 0xFFFF
  INCSP RET

is_not_file:
  -- ^ file
  RAM_AL 0x00
  RAM_B 0xFFFE
  INCSP RET
