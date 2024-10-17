GLOBAL _start
EXTERN put_char
EXTERN exit

  { current_process 0xF802 }
  { cwd_offset 0x02 }
  { entry_start 0x0C }

_start:
  RAM_B current_process rB_A RAM_BL cwd_offset SUM A_B rB_A -- cwd
  A_SEC
  RAM_NDX entry_start

entry:
  MEM_A CMPA JMPRZ $end
loop:
  MEM_A INCNDX
  CMPA JMPRZ $next
  CALL put_char
  JMPR $loop

next:
  RAM_AL 0x0A CALL put_char -- new line
  INCNDX INCNDX -- skip ptr
  JMPR $entry

end:
  CALL exit
