GLOBAL solve_path

  { current_process 0xF802 }
  { cwd_offset 0x02 }
  { entry_start 0x03 }
  { next_dir_index 0x01 }

-- [cstr path, _] -> [u16 sec, _]
-- "/": as separator
-- "..": parent directory
-- ".": cwd
-- ERROR: return [0, 0xFFFF] when not found
-- ERROR: return [0, 0xFFFA] when in "a/b" a is not a dir
solve_path:
  PUSHA PUSHA
  -- ^ pathi path
  RAM_B current_process rB_A RAM_BL cwd_offset SUM A_B rB_A A_SEC
  RAM_NDX entry_start
  -- (fall-through)
check_entry:
  -- ^ pathi path
  MEM_A CMPA JMPRZ $next_dir_sec
  -- (fall-through)
check_char:
  -- ^ pathi path
  PEEKB rB_AL CMPA JMPRZ $path_end
  A_B RAM_AL "/" SUB JMPRZ $sub_dir
  MEM_A CMPA JMPRZ $neq
  SUB JMPRNZ $neq

  INCNDX -- skip \0
  POPA INCA PUSHA -- pathi ++
  JMPR $check_char

neq:
  -- ^ pathi path
  INCNDX MEM_A CMPA JMPRNZ $neq -- skip untill \0
  INCNDX INCNDX INCNDX -- skip \0 and ptr
  INCSP PEEKA PUSHA -- pathi = path
  JMPR $check_entry

path_end:
  -- ^ pathi path
  MEM_A CMPA JMPRNZ $neq -- if *pathi == 0 && *mem != 0 { neq }
  INCNDX MEM_A INCNDX MEM_AH -- ptr
  INCSP INCSP RET

sub_dir:
  -- ^ pathi path
  MEM_A CMPA JMPRNZ $neq -- if *pathi == '/' && *mem != 0 { neq }
  INCNDX MEM_A INCNDX MEM_AH A_SEC
  RAM_NDX 0x00
  MEM_A RAM_BL "D" SUB JMPRNZ $is_not_dir  
  RAM_NDX entry_start
  POPA INCA PUSHA -- pathi ++
  JMPR $check_char

is_not_dir:
  RAM_AL 0x00
  RAM_B 0xFFFA
  INCSP INCSP RET

next_dir_sec:
  -- ^ pathi path
  RAM_NDX next_dir_index MEM_A INCNDX MEM_AH 
  INCA JMPRZ $not_found
  DECA A_SEC
  RAM_NDX entry_start 
  INCSP PEEKA PUSHA -- pathi = path
  JMPR $check_entry

not_found:
  -- ^ pathi path
  RAM_AL 0x00
  RAM_B 0xFFFF
  INCSP INCSP RET
