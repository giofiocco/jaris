GLOBAL _start
EXTERN solve_path
EXTERN get_char
EXTERN get_delim
EXTERN put_char
EXTERN print
EXTERN print_with_len
EXTERN execute
EXTERN exit
EXTERN str_find_char

  { current_process 0xF802 }
  { cwd_offset 0x02 }

input: db 128
not_found_string: ": command not found" 0x0A 0x00

_start:
  RAM_AL "$" CALL put_char
  RAM_AL " " CALL put_char

  RAM_A input PUSHA
echo:
  -- ^ inputi
  CALL get_char
  CMPA JMPRZ $quit
  PUSHA CALL put_char
  POPB RAM_AL 0x0A SUB JMPRZ $enter
  B_A
  POPB AL_rB INCB PUSHB
  JMPR $echo

enter:
  -- ^ inputi
  POPB RAM_AL 0x00 AL_rB -- *inputi = 0
  RAM_A input RAM_BL " " CALL str_find_char
  CMPA JMPRZ $no_args
  A_B RAM_AL 0x00 AL_rB B_A -- separate command name and args
  INCA
no_args:
  PUSHA
  -- ^ args

  RAM_A input CALL solve_path
  CMPB JMPRN $not_found
  RAM_A input POPB CALL execute
  JMPR $_start

not_found:
  -- ^ args
  INCSP
  RAM_A input CALL print
  RAM_A not_found_string CALL print
  JMPR $_start

quit:
  RAM_AL 0x00 CALL exit
