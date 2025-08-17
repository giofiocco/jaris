-- A stack based language inspired on Forth
-- Usage: stack <file>

GLOBAL _start

EXTERN exit
EXTERN print
EXTERN open_file
EXTERN read_u8
EXTERN put_char
EXTERN open_create_file
EXTERN write_u8
EXTERN write_u16
EXTERN parse_int
EXTERN print_int

file: db 4
out_file: db 4
code_size_file_bak: db 4
buffer: db 32
code_size: 0x0000

  { stdlib_pos 0xF800 }

-- [_, _] -> [_, _]
-- if code_size even adds one 0x00
unalign:
  RAM_B code_size rB_A SHR JMPRC $dont_align
  SHL INCA A_rB
  RAM_A out_file RAM_BL 0x00 CALL write_u8
dont_align:
  RET

-- [_, _] -> [_, _]
-- reads the buffer
parse:
  RAM_A buffer CALL parse_int CMPA JMPRN $not_int
  PUSHA
  CALLR $unalign
  RAM_A out_file RAM_BL RAM_A CALL write_u8
  POPB CALL write_u16
  RAM_BL PUSHA CALL write_u8
  RAM_B code_size rB_A INCA INCA INCA INCA A_rB
not_int:
  RAM_B buffer rB_A RAM_BL "+" SUB JMPRNZ $not_sum
  RAM_A out_file RAM_BL POPA CALL write_u8
  RAM_BL POPB CALL write_u8
  RAM_BL SUM CALL write_u8
  RAM_BL PUSHA CALL write_u8
  RAM_B code_size rB_A INCA INCA INCA INCA A_rB
not_sum:
  RET

_start:
  CMPB JMPRZ $no_arg
  B_A RAM_B file CALL open_file

  RAM_A out_str RAM_B out_file CALL open_create_file
  RAM_B "EX" CALL write_u16
  RAM_BL "E" CALL write_u8

  RAM_B out_file rB_A RAM_B code_size_file_bak A_rB
  RAM_B out_file INCB INCB rB_A RAM_B code_size_file_bak INCB INCB A_rB

  RAM_A out_file RAM_BL 0x00 CALL write_u16 -- code_size for now

  RAM_A buffer PUSHA
loop:
  -- ^ bufferi
  RAM_A file CALL read_u8 A_B
  INCA JMPRZ $loop_end DECA
  RAM_AL " " SUB JMPRNZ $not_space
  PEEKB RAM_A buffer SUB JMPRZ $loop
  POPB RAM_AL 0x00 AL_rB
  RAM_A buffer PUSHA
  CALLR $parse
  JMPR $loop
not_space:
  -- bufferi [_, char]
  B_A PEEKB AL_rB
  POPA INCA PUSHA
  JMPR $loop

loop_end:
  -- ^ bufferi
  POPB RAM_AL 0x00 AL_rB
  CALLR $parse


  CALLR $unalign
  RAM_A out_file RAM_BL RAM_AL CALL write_u8
  RAM_BL 0x00 CALL write_u8
  RAM_BL CALL CALL write_u8
  RAM_BL 0x00 CALL write_u16 -- exit placeholder
  -- writing: unalign RAM_AL 0x00 CALL exit
  RAM_B code_size rB_A INCA INCA INCA PUSHA INCA INCA A_rB
  -- ^ exit_pos

  RAM_A out_file RAM_BL 0x00 CALL write_u16 -- reloc count
  RAM_BL 0x01 CALL write_u16 -- dynamic linking count
  RAM_B 0x0101 CALL write_u16 -- file name and size
  RAM_BL 0x01 CALL write_u8 -- reloc count

  PEEKB CALL write_u16 -- where to subst = exit ptr
  RAM_B stdlib_pos rB_A RAM_B exit SUB A_B RAM_A out_file CALL write_u16
  RAM_BL 0x01 CALL write_u16 -- symbol list count
  RAM_BL 0x04 CALL write_u8 -- label len
  RAM_B "ex" CALL write_u16
  RAM_B "it" CALL write_u16
  RAM_B 0xFFFF CALL write_u16 -- pos
  RAM_BL 0x01 CALL write_u8 -- reloc count
  PEEKB CALL write_u16 -- reloc
  RAM_BL 0x00 CALL write_u16 -- relreloc count
  INCSP

  RAM_B code_size_file_bak RAM_A code_size
  RAM_B code_size rB_A A_B RAM_A code_size_file_bak CALL write_u16

  RAM_AL 0x00 CALL exit

no_arg_str: "ERROR: expected file as arg" 0x0A 0x00
no_arg:
  RAM_A no_arg_str CALL print
  RAM_AL 0x01 CALL exit

out_str: "stack.out" 0x00
