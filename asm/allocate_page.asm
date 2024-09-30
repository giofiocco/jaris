GLOBAL allocate_page

  { iup_ptr  0xF806 }
  { iup2_ptr 0xF808 }

-- [_, _] -> [page index, _]
allocate_page:
  RAM_AL 0x00 PUSHA
  RAM_B iup_ptr rB_A
search:
  -- ^ count [iup, _]
  A_B POPA INCA PUSHA B_A
  SHL JMPRC $search

  POPA
  A_B RAM_AL 0x11 SUB JMPRZ $second_half  -- 17: iup is 0xFFFF
  B_A

  RET 

second_half:
  RAM_B iup2_ptr rB_A
  CMPA JMPRZ $return_17

  RAM_BL 0x10 PUSHB
  JMPR $search

return_17:
  RAM_AL 0x11
  RET
