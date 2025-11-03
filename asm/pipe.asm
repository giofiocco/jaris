GLOBAL _start

EXTERN exit
EXTERN print
EXTERN str_find_char
EXTERN solve_path
EXTERN put_char
EXTERN execute

  { current_process_ptr 0xF802 }
  { stdout_offset 0x06 }

stdout: db 128
stdout_end:

no_arg_str: "ERROR: expected arg" 0x0A 0x00
not_found_str: ": command not found" 0x0A 0x00

-- Usage: pipe command1 args1 | command2 args2 | ...
_start:
  RAM_AL 0x00 PUSHA
  B_A CMPA JMPRNZ $has_cmd
  RAM_A no_arg_str CALL print
  RAM_AL 0x01 CALL exit
has_cmd:
  -- [cmd, _]
  PUSHA
  RAM_BL "|" CALL str_find_char
  -- ^ &pipe cmd
  CMPA JMPRZ $found_pipe
  A_B RAM_AL 0x00 AL_rB B_A
found_pipe:
  POPB PUSHA PUSHB
  -- ^ cmd &cmd2

  -- setup stdout
  RAM_A stdout RAM_B stdout_end SUB RAM_B stdout A_rB
  RAM_AL 0x02 SUM A_B INCA INCA A_rB
  -- set stdout redirect
  RAM_B current_process_ptr rB_A
  RAM_BL stdout_offset SUM
  A_B RAM_A stdout A_rB

  POPA CALLR $execute_cmd

skip_space:
  -- ^ &cmd2
  POPA INCA PUSHA
  A_B rB_AL RAM_BL " " SUB JMPRZ $skip_space
  -- ^ &cmd2

  POPA CALLR $execute_cmd

  RAM_AL 0x00 CALL exit

-- [cstr cmd, _] -> [u16 exitcode, _]
-- exit if error
execute_cmd:
  PUSHA
  RAM_BL " " CALL str_find_char
  CMPA JMPRZ $execute_no_arg
  A_B RAM_AL 0x00 AL_rB B_A INCA
execute_no_arg:
  PUSHA
  -- ^ arg cmd

  PEEKAR 0x04 CALL solve_path
  CMPB JMPRNN $found
  PEEKAR 0x04 CALL print
  RAM_A not_found_str CALL print
  RAM_AL 0x01 CALL exit
found:
  POPB POPA CALL execute
  RET
