GLOBAL _start
EXTERN solve_path
EXTERN get_delim
EXTERN print
EXTERN print_with_len
EXTERN execute
EXTERN exit

input: db 128
not_found_string: "command not found" 0x00

path: "ls" 0x00

_start:
  -- TODO: do better echoing
  RAM_AL 0x0A RAM_B input CALL get_delim
  A_B RAM_A input CALL print_with_len

  RAM_A input CALL solve_path
  CMPA JMPRN $not_found

  RAM_B input rB_A HLT

  -- TODO: desn't work: even if add RAM_B input rB_A HLT
  -- RAM_A input CALL execute
  CALL exit

not_found:
  RAM_A not_found_string CALL print
  RAM_AL 0xFF CALL exit
