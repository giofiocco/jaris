{ root_A RAM_AL 0x01 }
{ entry_start_NDX RAM_NDX 0x0C }
{ data_start_NDX RAM_NDX 0x04 }
{ max_index_NDX RAM_NDX 0x03 }
{ next_subsector_index_NDX RAM_NDX 0x01 }

  RAM_A 0xFFFE A_SP

  root_A A_SEC 
  entry_start_NDX
  -- (fall-through)
check_entry:
  MEM_A JMPRZ $file_not_found

  RAM_BL "_" SUB JMPRNZ $neq
  INCNDX MEM_A RAM_BL "_" SUB JMPRNZ $neq
  INCNDX MEM_A RAM_BL "o" SUB JMPRNZ $neq
  INCNDX MEM_A RAM_BL "s" SUB JMPRZ $found
  -- (fall-through)
neq:
  MEM_A CMPA JMPRZ $next_entry
  INCNDX MEM_A CMPA JMPRNZ $neq

next_entry:
  INCNDX INCNDX INCNDX
  JMPR $check_entry

file_not_found:
  RAM_A 0xFAAA
  HLT
is_not_file:
  RAM_A 0xFAAB
  HLT
is_not_exe:
  RAM_A 0xFAAC
  HLT

found:
  INCNDX INCNDX MEM_A INCNDX MEM_AH A_SEC
  RAM_NDX 0x00
  MEM_A RAM_BL "F" SUB JMPRNZ $is_not_file
  data_start_NDX
  MEM_A RAM_BL "E" SUB JMPRNZ $is_not_exe INCNDX
  MEM_A RAM_BL "X" SUB JMPRNZ $is_not_exe INCNDX
  MEM_A RAM_BL "E" SUB JMPRNZ $is_not_exe INCNDX

  MEM_A INCNDX MEM_AH INCNDX 
  A_B RAM_A 0x0800 SUB JMPRNN $toobig -- if (codesize - 2048 > 0) {  } (2048 page size)
  PUSHB
  RAM_AL 0x00 PUSHA
  -- (fall-through)
copy_code:
  -- ^ mar codesize
  CALLR $get_u8
  POPB AL_rB -- RAM[mar] = *mem
  B_A INCA PUSHA -- mar = mar+1
  INCSP POPA DECA JMPRZ $end_copy
  PUSHA DECSP
  JMPR $copy_code

end_copy:
  -- ^
  CALLR $get_u8
  PUSHA
  CALLR $get_u8
  A_B POPA B_AH
  -- (fall-through)
reloc:
  -- ^ [reloc_count, _]
  PUSHA

  CALLR $get_u8
  PUSHA
  CALLR $get_u8
  A_B POPA B_AH
  PUSHA

  CALLR $get_u8
  PUSHA
  CALLR $get_u8
  A_B POPA B_AH

  POPB A_rB

  POPA DECA JMPRNZ $reloc
  -- end reloc
  JMP 0x0000

get_u8:
  NDX_A CMPA JMPRZ $get_u8_next_subsector
  A_B
  -- if ndx - maxndx - 1 >= 0 { goto nextsec }
  max_index_NDX MEM_A SUB DECA JMPRNN $get_u8_next_subsector
  B_A A_NDX MEM_A INCNDX
  RET

get_u8_next_subsector:
  next_subsector_index_NDX
  MEM_A INCNDX MEM_AH A_SEC
  data_start_NDX MEM_A INCNDX
  RET
  
toobig:
  -- TODO
  RAM_A 0xFA11
  HLT
