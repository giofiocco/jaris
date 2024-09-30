GLOBAL solve_path

{ current_process 0xF802 }
{ cwd_offset 0x02 }
{ entry_start 0x03 }
{ next_dir_index 0x01 }

-- [cstr path, _] -> [u16 sec, _]
-- returns the sec of the path (dir or file)
-- "/": separator
-- "..": parent dir
-- ".": cwd
-- ERRORS:
-- [0, 0xFFFF] if entry not found 
-- [0, 0xFFFE] if in "a/b" "a" is not a dir sec
solve_path:
  PUSHA PUSHA
  -- ^ pathi path
  RAM_B current_process rB_A RAM_BL cwd_offset SUM A_B rB_A A_SEC
  RAM_NDX entry_start
check_entry:
  -- ^ pathi path
  PEEKB rB_AL CMPA JMPRZ $path_end
  A_B RAM_AL "/" SUB JMPRZ $sub_dir
  MEM_A INCNDX CMPA JMPRZ $not_found
  SUB JMPRNZ $neq

  POPA INCA PUSHA -- pathi ++
  JMPR $check_entry

neq:
  -- ^ pathi path
  MEM_A INCNDX CMPA JMPRNZ $neq -- skip while *mem != 0
  INCNDX INCNDX -- skip the ptr
  INCSP PEEKA PUSHA -- pathi = path
  JMPR $check_entry

sub_dir:
  -- ^ pathi path
  MEM_A INCNDX CMPA JMPRNZ $neq -- check that the entry is ended
  MEM_A INCNDX MEM_AH A_SEC
  RAM_NDX 0x00
  MEM_A RAM_BL "D" SUB JMPRNZ $is_not_dir
  RAM_NDX entry_start

  POPA INCA 
  INCSP PUSHA PUSHA -- path = pathi + 1; pathi = path
  -- so when pathi = path in neq the search starts from after the "/" of the original path
  JMPR $check_entry

path_end:
  -- ^ pathi path
  MEM_A INCNDX CMPA JMPRNZ $neq -- check that the entry is ended
  MEM_A INCNDX MEM_AH -- ptr
  INCSP INCSP RET

not_found:
  -- TODO: search next dir sec 
  -- ^ pathi path
  RAM_AL 0x00
  RAM_B 0xFFFF
  INCSP INCSP RET

is_not_dir:
  -- ^ pathi path
  RAM_AL 0x00
  RAM_B 0xFFFE
  INCSP INCSP RET
