GLOBAL execute
EXTERN open_file
EXTERN read_char

  { iup_ptr  0xF806 }
  { iup2_ptr 0xF808 }

page_index: 0x00
ALIGN file: db 4

-- [cstr path, cstr argv[] (null terminated)] -> [_, _]
-- [0xFFFF, _] if file not found
-- [0xFFFE, _] if file not an exe
-- [0xFFFA, _] if no more space in ram
execute:
  PUSHB PUSHA

  CALLR $allocate_page
  A_B POPA PUSHB
  -- ^ ram_start argv [path, _]
  RAM_B file
  CALL open_file
  CMPA JMPRNZ $not_found

  RAM_A file CALL read_char RAM_BL "E" SUB JMPRNZ $not_exe
  RAM_A file CALL read_char RAM_BL "X" SUB JMPRNZ $not_exe
  RAM_A file CALL read_char RAM_BL "E" SUB JMPRNZ $not_exe

  HLT

not_found
  RAM_A 0xFFFF
  HLT
not_exe:
  RAM_A 0xFFFE
  HLT

-- [_, _] -> [page_start, _]
allocate_page:
  RAM_AL 0x00 PUSHA
  RAM_A 0x8000 PUSHA
  RAM_B iup_ptr rB_A
  INCA JMPRZ $allocate_page2 -- if (iup == 0xFFFF) goto allocate_page2 
  DECA 
  SHL
search_page:
  -- ^ mask page_start [iup, _]
  PUSHA
  PEEKAR 0x04 SHR PUSHAR 0x04 
  PEEKAR 0x06 RAM_B 0x0800 SUM PUSHAR 0x06 -- 2048
  POPA SHL JMPRC $search_page

  RAM_B iup_ptr rB_A POPB SUM
  RAM_B iup_ptr A_rB

  POPA RET 

allocate_page2:
  RAM_AL 0x00 PUSHA
  RAM_A 0x8000 PUSHA
  RAM_B iup_ptr rB_A
  INCA JMPRZ $no_more_space -- if (iup == 0xFFFF) goto no_more_space 
  DECA 
  SHL
search_page2:
  -- ^ mask page_start [iup, _]
  PUSHA
  PEEKAR 0x04 SHR PUSHAR 0x04 
  PEEKAR 0x06 RAM_B 0x0800 SUM PUSHAR 0x06 -- 2048
  POPA SHL JMPRC $search_page2

  RAM_B iup2_ptr rB_A POPB SUM
  RAM_B iup2_ptr A_rB

  POPA RET 

no_more_space:
  -- ^ _ _
  RAM_A 0xFFFE
  INCSP INCSP RET

