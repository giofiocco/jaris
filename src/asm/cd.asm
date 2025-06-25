GLOBAL _start
EXTERN solve_path
EXTERN print
EXTERN exit

  { current_process 0xF802 }
  { cwd_offset 0x02 }

not_found_string: ": dir not found" 0x0A 0x00
not_dir_string: ": not a dir" 0x0A 0x00

-- Usage: cd [dir]
-- if no `dir` is specified set dir to the root
_start:
  CMPB JMPRZ $no_arg
  PUSHB
  B_A CALL solve_path
  CMPA JMPRZ $not_found

  A_SEC RAM_NDX 0x00 MEM_A RAM_BL "D" SUB JMPRNZ $not_dir

  SEC_A
  CALLR $set_parent_cwd
  INCSP
  RAM_AL 0x00 CALL exit

no_arg:
  RAM_AL 0x01 CALLR $set_parent_cwd
  RAM_AL 0x00 CALL exit

not_found:
  -- ^ path
  POPA CALL print
  RAM_A not_found_string CALL print
  RAM_AL 0x01 CALL exit

not_dir:
  -- ^ path
  POPA CALL print
  RAM_A not_dir_string CALL print
  RAM_AL 0x01 CALL exit

-- [cwd, _]
set_parent_cwd:
  PUSHA
  RAM_B current_process rB_A A_B rB_A RAM_BL cwd_offset SUM A_B POPA A_rB -- parent->cwd = cwd
  RET

