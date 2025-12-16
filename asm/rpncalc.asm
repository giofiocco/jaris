GLOBAL _start

EXTERN exit
EXTERN print

-- [char *buffer, _] -> [_, _]
next_token:
  A_B rB_AL A_B
  RAM_AL " " SUB JMPRNZ $not_space
not_space:
  RET


_start:
  B_A CALLR $next_token




  RAM_AL 0x00 CALL exit
