-- expects in the last 4 bytes of sec 0 the ptr to os and stdlib sec
-- [0xFFFF, 0xFFFF] if os is dynamically linked with not the stdlib
-- when jumping to 0x0000 in B there is the tot_size of os code + stdlib code

{ os_sec_pos 0xFC }
{ next_index 0x01 }
{ max_index 0x03 }
{ data_start 0x04 }
{ page_size 0x0800 }
{ ptr_to_stdlib_ptr 0xF800 }
{ ptr_to_used_pages_map 0xF806 }

RAM_A 0xFEFE A_SP -- 0xFF00 - 2
RAM_AL 0x00 A_SEC
RAM_NDX os_sec_pos
MEM_A INCNDX MEM_AH INCNDX PUSHA -- os_sec
MEM_A INCNDX MEM_AH INCNDX PUSHA -- stdlib_sec
-- ^ stdlib_sec os_sec
PEEKAR 0x04 A_SEC
RAM_NDX 0x07

CALLR $get_16 PUSHA PUSHA
RAM_BL 0x00 PUSHB
-- fall-through
copy_code:
  -- ^ mar size code_size stdlib_sec os_sec
  CALLR $get_16 POPB A_rB -- *mar = get_16
  POPA DECA DECA JMPRZ $copied PUSHA -- if ((size -= 2) == 0) goto copied
  B_A INCA INCA PUSHA -- mar += 2
  JMPR $copy_code

copied:
  -- ^ os_size stdlib_sec os_sec
  CALLR $get_16 SHL SHL -- reloc_count * 4
reloc:
  PUSHA CALLR $get_8 POPA DECA JMPRNZ $reloc

  CALLR $get_16 DECA JMPRNZ $not_stdlib -- dynamic_count
  CALLR $get_16 RAM_B 0x0101 SUB JMPRNZ $not_stdlib -- file_name len and filename == 0x01 0x01
  CALLR $get_8
dynamic_reloc:
  PUSHA
  -- ^ dynamic_reloc_count os_size stdlib_sec os_sec
  CALLR $get_16 PUSHA
  -- ^ where dynamic_reloc_count os_size stdlib_sec os_sec
  CALLR $get_16 A_B PEEKAR 0x06 SUM -- what += os_size
  POPB A_rB
  POPA DECA JMPRNZ $dynamic_reloc

  -- ^ os_size stdlib_sec os_sec
  PEEKAR 0x04 A_SEC -- stdlib
  RAM_NDX 0x06 -- 4 + 2
  CALLR $get_16 PUSHA -- stdlib code_size

  -- ^ code_size os_size stdlib_sec os_sec
  PEEKAR 0x04 A_B POPA
copy_stdlib:
  -- ^ os_size stdlib_sec os_sec [code_size, mar]
  PUSHA PUSHB
  -- ^ mar code_size os_size stdlib_sec os_sec
  CALLR $get_16
  POPB A_rB
  B_A INCA INCA A_B
  POPA DECA DECA JMPRNZ $copy_stdlib

  PUSHB
  -- ^ tot_size os_size stdlib_sec os_sec

  CALLR $get_16
  CMPA JMPRZ $reloced_stdlib
reloc_stdlib:
  PUSHA
  -- ^ count tot_size os_size stdlib_sec os_sec
  CALLR $get_16 A_B PEEKAR 0x06 SUM PUSHA -- where += os_size
  -- ^ where count tot_size os_size stdlib_sec os_sec 
  CALLR $get_16 A_B PEEKAR 0x08 SUM -- what += os_size
  POPB A_rB
  POPA DECA JMPRNZ $reloc_stdlib
  -- fall-throuh
reloced_stdlib:
  -- ^ tot_size os_size stdlib_sec os_sec
  PEEKAR 0x04 RAM_B ptr_to_stdlib_ptr A_rB

  POPB RAM_AL 0x00 JMPA -- TODO: JMP 0x0000

not_stdlib:
  RAM_A 0xFFFF A_B HLT

get_8:
  MEM_A PUSHA
  NDX_A A_B RAM_NDX max_index MEM_A SUB JMPRN $no_next_subsec
  -- if next subsec
  RAM_NDX next_index MEM_A INCNDX MEM_AH A_SEC
  RAM_BL 0x03 -- data-start - 1
  -- fall-through
no_next_subsec:
  B_A A_NDX INCNDX
  POPA RET

get_16:
  CALLR $get_8 PUSHA
  CALLR $get_8 A_B POPA B_AH
  RET
