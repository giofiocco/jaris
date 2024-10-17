GLOBAL _start
EXTERN get_delim
EXTERN exit

_start:
  CALL get_delim
  CALL exit
