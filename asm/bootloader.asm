{ entries_start 0x0C }
{ next_index 0x01 }
{ max_index 0x03 }
{ data_start 0x04 }
{ ptr_to_stdlib_ptr 0xF800 }

RAM_A 0xFEFA A_SP -- 0xFF00 - 2 - 2 - 2 (stdlib_sec os_sec) 
RAM_AL 0x01 A_SEC 
PUSHA
RAM_NDX entries_start

check_entry:
  -- ^ search_stdlib stdlib-sec(unset) so_sec(unset)
  MEM_A CMPA JMPRZ $crash

  MEM_A INCNDX MEM_AH A_B
  
  PEEKA CMPA JMPRZ $search_os -- if (!search_stdlib) goto search_os

  RAM_A 0x0002 SUB JMPRNZ $neq

  INCNDX MEM_A INCNDX MEM_AH PUSHAR 0x04 -- found stdlib-sec
  RAM_AL 0x00 INCSP PUSHA -- set search_stdlib
  INCNDX
  JMPR $check_entry

crash:
  RAM_A 0xFFFF
  HLT

neq:
  INCNDX MEM_A CMPA JMPRNZ $neq -- while (*(mem++) != 0); 
  INCNDX INCNDX INCNDX -- skip null char and sec-ptr 
  JMPR $check_entry

search_os:
  -- ^ search_stdlib stdlib_sec os_sec [_, MEM_A MEM_AH]
  INCSP
  -- ^ stdlib_sec os_sec [_, MEM_A MEM_AH]

  RAM_A 0x0001 SUB JMPRNZ $neq
  
-- found os-sec
  INCNDX MEM_A INCNDX MEM_AH PUSHAR 0x04
  A_SEC
  RAM_NDX 0x07 -- 4 + 3 

  MEM_A INCNDX MEM_AH INCNDX PUSHA
  RAM_BL 0x00 PUSHB
  -- fall-through
copy_code:
  -- ^ mar code_size stdlib_sec os_sec
  CALLR $get_16
  POPB A_rB
  PEEKA SUB INCA INCA JMPRNN $copied -- if (mar + 2 - code_size >= 0) goto copied
  B_A INCA INCA PUSHA
  JMPR $copy_code

copied:
  -- ^ os_size stdlib_sec os_sec 
  MEM_A INCNDX MEM_AH INCNDX 
  SHL SHL -- reloc_count * 4
  A_B NDX_A SUM A_NDX -- ndx += reloc_count * 4

  MEM_A CMPA JMPRZ $done -- if no dynamic linking goto done
  INCNDX INCNDX 
  MEM_A INCNDX MEM_AH INCNDX PUSHA -- dynamic_reloc_count
  NDX_A PUSHA -- os_ndx

  -- ^ os_ndx dynamic_reloc_count os_size stdlib_sec os_sec

  PEEKAR 0x08 A_SEC -- stdlib-sec
  RAM_NDX 0x06 -- 4 + 2: code size ndx 
  MEM_A INCNDX MEM_AH INCNDX -- code_size

  PUSHA
  PEEKAR 0x08 A_B POPA 
copy_stdlib:
  -- ^ os_ndx dynamic_reloc_count os_size stdlib_sec os_sec [code_size, mar]
  PUSHA PUSHB
  -- ^ mar code_size os_ndx dynamic_reloc_count os_size stdlib_sec os_sec
  CALLR $get_16
  POPB A_rB
  B_A INCA INCA A_B 
  POPA DECA DECA JMPRNZ $copy_stdlib
  
  -- ^ os_ndx dynamic_reloc_count os_size stdlib_sec os_sec 
  MEM_A INCNDX MEM_AH INCNDX
  CMPA JMPRZ $reloced_stdlib

  RAM_A 0xFEFE HLT -- TODO

reloced_stdlib:
  -- ^ os_ndx dynamic_reloc_count os_size stdlib_sec os_sec
  PEEKAR 0x0A A_SEC -- os_sec
  POPA A_NDX -- os_ndx
  POPA
  -- fall-through
dynamic_reloc:
  -- ^ os_size stdlib_sec os_sec [dynamic_reloc_count, _]
  PUSHA
  CALLR $get_16 PUSHA 
  -- ^ where dynamic_reloc_count os_size stdlib_sec os_sec
  CALLR $get_16 A_B PEEKAR 0x06 SUM -- what += os_size
  POPB A_rB
  POPA DECA JMPRNZ $dynamic_reloc
  -- fall-through
done:
  -- ^ os_size stdlib_sec os_sec 
  POPA RAM_B ptr_to_stdlib_ptr A_rB

  RAM_AL 0x00 JMPA

get_8:
  NDX_A A_B
  -- if (ndx - maxndx - 1 >= 0) goto next_filesec
  RAM_NDX max_index MEM_A SUB DECA JMPRNN $next_filesec
  B_A A_NDX
  MEM_A INCNDX
  RET

next_filesec:
  RAM_NDX next_index
  MEM_A INCNDX MEM_AH
  A_SEC
  RAM_NDX data_start
  MEM_A INCNDX 
  RET

get_16:
  CALLR $get_8 PUSHA
  CALLR $get_8 A_B POPA B_AH
  RET

