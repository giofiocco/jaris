GLOBAL open_file
GLOBAL open_create_file

EXTERN solve_path
EXTERN path_find_name

  { max_ndx_index 0x03 }
  { data_start 0x04 }
  { first_entry_ndx 0x0C }

-- [cstr path, FILE *file] -> [file, _]
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

  PEEKB SEC_A A_rB
  INCB INCB RAM_AL data_start AL_rB

  POPA RET

not_found:
  -- ^ file [0, _]
  RAM_B 0xFFFF
  INCSP RET

is_not_file:
  -- ^ file
  RAM_AL 0x00
  RAM_B 0xFFFE
  INCSP RET

-- [cstr path, FILE *file] -> [file, _]
-- the file struct has to be aligned
-- ERRORS:
-- [0, 0xFFFE] if path is a dir
open_create_file:
  PUSHB PUSHA
  -- ^ path file

  CALL solve_path
  CMPA JMPRNZ $found
  B_A POPB INCA JMPRNZ $is_not_file PUSHB
  -- ^ path file
  POPA CALL path_find_name
  PUSHA
  -- ^ name file
  RAM_NDX first_entry_ndx
loop:
  INCNDX MEM_A CMPA JMPRNZ $loop
  INCNDX INCNDX INCNDX MEM_A CMPA JMPRNZ $loop

  POPB rB_AL A_MEM INCNDX -- first char copied
copy_name:
  B_A INCA A_B
  rB_AL CMPA A_MEM INCNDX CMPA JMPRNZ $copy_name

  NDX_A PUSHA
  SEC_A PUSHA
  -- ^ current_sec current_ndx file

  RAM_AL 0x01 A_SEC
  RAM_NDX 0x00
search_sec:
  SEC_A INCA A_SEC
  MEM_A CMPA JMPRNZ $search_sec

  SEC_A A_B POPA A_SEC POPA A_NDX
  -- ^ file
  PUSHB
  B_A A_MEM INCNDX
  SHR SHR SHR SHR SHR SHR SHR SHR A_MEM INCNDX
  RAM_AL 0x00 A_MEM
  -- ^ found_sec file [_, found_sec]

  -- populate file sec
  B_A A_SEC RAM_NDX 0x00
  RAM_AL "F" A_MEM INCNDX
  RAM_AL 0xFF A_MEM INCNDX A_MEM INCNDX
  NDX_A A_MEM

  -- populate file struct
  -- ^ found_sec file [_, found_sec]
  POPA PEEKB A_rB -- sec
  B_A INCA INCA A_B
  -- SEC: found_sec NDX: index_of_max_ndx
  RAM_AL data_start AL_rB

  POPA RET

found:
  -- ^ path file [sec, _]
  A_SEC
  RAM_NDX max_ndx_index
  INCSP PEEKB A_rB -- set file->sec
  B_A INCA INCA A_B
  RAM_AL 0x03 A_MEM AL_rB

  POPA RET
