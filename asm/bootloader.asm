-- expects in the last 4 bytes of sec 0 the ptr to os and stdlib sec 

{ entries_start 0x0C }
{ next_index 0x01 }
{ max_index 0x03 }
{ data_start 0x04 }
{ ptr_to_stdlib_ptr 0xF800 }

RAM_A 0xFEFE A_SP -- 0xFF00 - 2
RAM_AL 0x00 A_SEC 
RAM_NDX 0xFC 
MEM_A INCNDX MEM_AH INCNDX PUSHA -- os_sec
MEM_A INCNDX MEM_AH INCNDX PUSHA -- stdlib_sec
-- ^ stdlib_sec os_sec
PEEKAR 0x04 A_SEC 
RAM_NDX 0x07 -- 4 + 3 

CALLR $get_16 PUSHA
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
  CALLR $get_16 SHL SHL -- reloc_count * 4
  A_B NDX_A SUM A_NDX -- ndx += reloc_count * 4

  CALLR $get_8 CMPA JMPRZ $done -- if no dynamic linking goto done
  CALLR $get_8 -- if the name is 0x01 0x00 the 0x01 is already read so skip the 0x00 
  CALLR $get_16 PUSHA -- dynamic_reloc_count
  NDX_A PUSHA -- os_ndx

  -- ^ os_ndx dynamic_reloc_count os_size stdlib_sec os_sec
  PEEKAR 0x08 A_SEC -- stdlib
  RAM_NDX 0x06 -- 4 + 2
  CALLR $get_16 PUSHA -- stdlib code_size
  -- ^ code_size os_ndx dynamic_reloc_count os_size stdlib_sec os_sec
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
  CALLR $get_16
  CMPA JMPRZ $reloced_stdlib

  RAM_A 0xFFFE HLT -- TODO

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
  NDX_A CMPA JMPRZ $get_8_next_subsector
  A_B
  -- if ndx - maxndx - 1 >= 0 { goto nextsec }
  RAM_NDX max_index MEM_A SUB DECA JMPRNN $get_8_next_subsector
  B_A A_NDX MEM_A INCNDX
  RET

get_8_next_subsector:
  RAM_NDX next_index
  MEM_A INCNDX MEM_AH A_SEC
  RAM_NDX data_start MEM_A INCNDX
  RET
 

get_16:
  CALLR $get_8 PUSHA
  CALLR $get_8 A_B POPA B_AH
  RET

