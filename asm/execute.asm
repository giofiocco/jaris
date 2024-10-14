GLOBAL execute
EXTERN open_file
EXTERN read_u8
EXTERN read_u16

  { iup_ptr  0xF806 }
  { iup2_ptr 0xF808 }
  { process_table_start 0xF820 }
  { process_map_ptr 0xF804 }
  { current_process_ptr 0xF802 }

page_index: 0x00
ALIGN file: db 4

-- [cstr path, cstr argv[] (null terminated)] -> [_, _]
-- [0xFFFF, _] if file not found
-- [0xFFFE, _] if file not an exe
-- [0xFFFA, _] if no more space in ram
-- [0xFAFA, _] dynamic linking TODO: 
execute:
  PUSHB PUSHA

  CALLR $allocate_page
  A_B POPA PUSHB PUSHB
  -- ^ ram ram_start argv [path, _]
  RAM_B file
  CALL open_file
  CMPA JMPRZ $not_found

  RAM_A file CALL read_u16 RAM_B "EX" SUB JMPRNZ $not_exe
  RAM_A file CALL read_u8 RAM_BL "E" SUB JMPRNZ $not_exe

  RAM_A file CALL read_u16 
copy_code:
  PUSHA
  -- ^ code_size(even) ram ram_start argv
  RAM_A file CALL read_u16 PUSHA
  -- ^ 2insts code_size ram ram_start argv
  PEEKAR 0x06 A_B POPA A_rB
  -- ^ code_size ram ram_start argv
  PEEKAR 0x04 INCA INCA PUSHAR 0x04
  POPA DECA DECA JMPRNZ $copy_code
  -- ^ ram ram_start argv

  INCSP
  RAM_A file CALL read_u16 
reloc:
  PUSHA
  -- ^ count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x04 SUM PUSHA -- where += ram_start
  -- ^ where count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x06 SUM -- what += ram_start
  POPB A_rB
  POPA DECA JMPRNZ $reloc
  -- ^ ram_start argv

  RAM_A file CALL read_u16
  CMPA JMPRNZ $dynamic_linking

  RAM_A process_table_start PUSHA
  RAM_B process_map_ptr rB_A
  -- TODO: check if no more processes
  SHL
search_process:
  PUSHA
  -- ^ map process_ptr ram_start argv
  PEEKAR 0x04 RAM_BL 0x10 SUM PUSHAR 0x04
  POPA SHL JMPRC $search_process
  -- ^ process_ptr ram_start argv

  RAM_B current_process_ptr rB_A PEEKB A_rB

  HLT

dynamic_linking:
  RAM_A 0xFAFA
  HLT

not_found:
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

