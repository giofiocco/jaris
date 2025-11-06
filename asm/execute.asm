GLOBAL execute

EXTERN open_file
EXTERN read_u8
EXTERN read_u16

  { stdlib_ptr_ptr 0xF800 }
  { page_size 0x0800 }
  { iup_ptr  0xF806 }
  { iup2_ptr 0xF808 }
  { process_table_start 0xF820 }
  { process_map_ptr 0xF804 }
  { current_process_ptr 0xF802 }
  { cwd_offset 0x02 }
  { SP_offset 0x04 }
  { stdout_offset 0x06 }
  { stdin_offset 0x08 }

ALIGN file: db 4

-- [cstr path, cstr argv] -> [_, _]
-- crash [0xFFFF, _] if file not found
-- crash [0xFFFE, _] if file not an exe
-- crash [0xFFFA, _] if no more space in ram
-- crash [0xFAFA, _] TODO: dynamic linking with not the stdlib
-- TODO: error instead of crash
execute:
  PUSHB PUSHA

  CALLR $allocate_page
  A_B POPA PUSHB PUSHB
  -- ^ ram ram_start argv [path, _]
  RAM_B file CALL open_file
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

  INCSP RAM_A file CALL read_u16
  CMPA JMPRZ $reloc_end
reloc:
  PUSHA
  -- ^ count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x04 SUM PUSHA -- where += ram_start
  -- ^ where count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x06 SUM -- what += ram_start
  POPB A_rB
  POPA DECA JMPRNZ $reloc
reloc_end:
  -- ^ ram_start argv

  RAM_A file CALL read_u16
dynamic_link:
  PUSHA
  -- ^ dynamic_count ram_start argv
  -- TODO: no stdlib
  RAM_A file CALL read_u16 RAM_B 0x0101 SUB JMPRNZ $not_stdlib
  RAM_B stdlib_ptr_ptr rB_A PUSHA -- stdlib
  RAM_A file CALL read_u8
dynamic_reloc:
  PUSHA
  -- ^ dynamic_reloc_count stdlib_start dynamic_count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x08 SUM PUSHA
  -- ^ where dynamic_reloc_count stdlib_start dynamic_count ram_start argv
  RAM_A file CALL read_u16 A_B PEEKAR 0x06 SUM
  POPB A_rB
  POPA DECA JMPRNZ $dynamic_reloc
  -- ^ stdlib_start dynamic_count ram_start argv
  INCSP
  POPA DECA JMPRNZ $dynamic_link

  -- ^ ram_start argv
  RAM_A process_table_start PUSHA
  RAM_B process_map_ptr rB_A
  -- TODO: check if no more processes
  SHL 
  RAM_B 0x8000 PUSHB
search_process:
  PUSHA
  -- ^ map process_mask process_ptr ram_start argv
  PEEKAR 0x06 RAM_BL 0x10 SUM PUSHAR 0x06
  PEEKAR 0x04 SHR PUSHAR 0x04
  POPA SHL JMPRC $search_process
  -- ^ process_mask process_ptr ram_start argv

  RAM_B process_map_ptr rB_A POPB SUM RAM_B process_map_ptr A_rB -- *process_map_ptr += process_mask
  RAM_B current_process_ptr rB_A PEEKB A_rB -- process_ptr->parent_process = current_process_ptr
  B_A RAM_B current_process_ptr A_rB -- *current_process_ptr = process_ptr
  -- ^ process_ptr ram_start argv

  PEEKB rB_A RAM_BL cwd_offset SUM A_B rB_A PUSHA
  PEEKAR 0x04 RAM_BL cwd_offset SUM A_B POPA A_rB -- process_ptr->cwd = process_ptr->parent_process->cwd

  PEEKAR 0x04 RAM_B page_size SUM DECA DECA
  A_B PEEKAR 0x06 A_rB -- *(ram_start + page_size - 2) = argv
  B_A DECA DECA PUSHA

  -- ^ new_sp process_ptr ram_start argv
  PEEKAR 0x04 RAM_BL SP_offset SUM A_B RAM_AL 0x00 A_rB -- process_ptr->SP = 0

  PEEKAR 0x04 A_B rB_A RAM_BL stdout_offset SUM A_B rB_A PUSHA
  PEEKAR 0x06 RAM_BL stdout_offset SUM A_B POPA A_rB -- process_ptr->stdout = process_ptr->parent_process->stdout

  PEEKAR 0x04 A_B rB_A RAM_BL stdin_offset SUM A_B rB_A PUSHA
  PEEKAR 0x06 RAM_BL stdin_offset SUM A_B POPA A_rB -- process_ptr->stdin = process_ptr->parent_process->stdin

  SP_A RAM_BL 0x08 SUM PUSHA -- sp before return ptr
  PEEKAR 0x06 A_B rB_A RAM_BL SP_offset SUM A_B POPA A_rB -- process_ptr->parent_process->SP = sp before return ptr

  -- ^ new_sp process_ptr ram_start argv
  POPA INCSP POPB -- [new_sp, ram_start]
  A_SP
  B_A POPB -- [ram_start, argv]
  JMPA

not_found:
  RAM_A 0xFFFF HLT
not_exe:
  RAM_A 0xFFFE HLT

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
  PEEKAR 0x06 RAM_B page_size SUM PUSHAR 0x06
  POPA SHL JMPRC $search_page

  RAM_B iup_ptr rB_A POPB SUM
  RAM_B iup_ptr A_rB

  POPA RET

allocate_page2:
  -- TODO: to implement
  RAM_A 0xFFFE HLT

not_stdlib:
  -- ^ dynamic_count ram_start argv
  RAM_A 0xFAFA HLT
