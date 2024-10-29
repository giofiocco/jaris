GLOBAL _start
EXTERN solve_path
EXTERN get_char
EXTERN get_delim
EXTERN put_char
EXTERN print
EXTERN print_with_len
EXTERN execute
EXTERN exit

input: db 128
not_found_string: "command not found" 0x0A 0x00

_start:
  RAM_A input PUSHA
echo:
  -- ^ inputi
  CALL get_char
  CMPA JMPRZ $quit
  PUSHA CALL put_char
  POPB RAM_AL 0x0A SUB JMPRZ $execute_program
  B_A
  POPB AL_rB INCB PUSHB
  JMPR $echo

execute_program:
  -- ^ inputi
  POPB INCB RAM_AL 0x00 AL_rB -- *inputi = 0
  RAM_A input CALL solve_path
  CMPB JMPRN $not_found

  RAM_A input CALL execute

  JMPR $_start

not_found:
  RAM_A not_found_string CALL print
  JMPR $_start

quit:
  RAM_AL 0x00 CALL exit
