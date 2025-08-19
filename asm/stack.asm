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
EXTERN str_eq

file: db 4
out_file: db 4
buffer: db 32
code_size: 0x0000

  { stdlib_pos 0xF800 }
  { inc_code_size RAM_B code_size rB_A INCA A_rB }

dup_str: "dup" 0x00
drop_str: "drop" 0x00
swap_str: "swap" 0x00

-- [func ptr (reloced), _] -> [_, _]
-- write out CALL 0x0000, increment the code_size, then call push_dynamic_reloc with the pointer to the function wanted
push_dynamic_reloc:
  PUSHA
  RAM_B dr_index rB_A PUSHA INCA INCA INCA INCA A_rB -- *dr_index += 4
  -- ^ dr_index(of where) func_ptr
  RAM_B code_size rB_A DECA DECA POPB A_rB INCB INCB PUSHB -- push to dynamic_reloc the code_size - 2 (the where)
  -- ^ dr_index(of what) func_ptr
  RAM_B stdlib_pos rB_A PUSHA PEEKAR 0x06 A_B POPA -- [stdlib_start, func_ptr]
  SUB POPB A_rB -- push to dynamic_reloc the func_ptr - stdlib_start
  -- ^ func_ptr
  INCSP RET

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
  RET
not_int:
  RAM_B buffer rB_A RAM_BL "+" SUB JMPRNZ $not_add
  RAM_A out_file RAM_BL POPA CALL write_u8
  RAM_BL POPB CALL write_u8
  RAM_BL SUM CALL write_u8
  RAM_BL PUSHA CALL write_u8
  RAM_B code_size rB_A INCA INCA INCA INCA A_rB
  RET
not_add:
  RAM_B buffer rB_A RAM_BL "-" SUB JMPRNZ $not_sub
  RAM_A out_file RAM_BL POPA CALL write_u8
  RAM_BL POPB CALL write_u8
  RAM_BL SUB CALL write_u8
  RAM_BL PUSHA CALL write_u8
  RAM_B code_size rB_A INCA INCA INCA INCA A_rB
  RET
not_sub:
  RAM_A buffer RAM_B dup_str CALL str_eq CMPA JMPRZ $not_dup
  RAM_A out_file RAM_BL PEEKA CALL write_u8
  RAM_BL PUSHA CALL write_u8
  RAM_B code_size rB_A INCA INCA A_rB
  RET
not_dup:
  RAM_A buffer RAM_B drop_str CALL str_eq CMPA JMPRZ $not_drop
  RAM_A out_file RAM_BL INCSP CALL write_u8
  inc_code_size
  RET
not_drop:
  RAM_A buffer RAM_B swap_str CALL str_eq CMPA JMPRZ $not_swap
  RAM_A out_file RAM_BL POPA CALL write_u8
  RAM_A out_file RAM_BL POPB CALL write_u8
  RAM_A out_file RAM_BL PUSHA CALL write_u8
  RAM_A out_file RAM_BL PUSHB CALL write_u8
  RAM_B code_size rB_A INCA INCA INCA INCA A_rB
  RET
not_swap:
  RAM_B buffer rB_A RAM_BL "." SUB JMPRNZ $not_print
  RAM_A out_file RAM_BL POPA CALL write_u8
  inc_code_size
  CALLR $unalign
  RAM_A out_file RAM_BL CALL CALL write_u8
  RAM_BL 0x00 CALL write_u16
  RAM_B code_size rB_A INCA INCA INCA A_rB
  RAM_A print_int CALLR $push_dynamic_reloc
  RET
not_print:
  RET

_start:
  CMPB JMPRZ $no_arg
  B_A RAM_B file CALL open_file
  CMPA JMPRZ $fail_open_file

  RAM_A dynamic_relocs RAM_B dr_index A_rB

  RAM_A out_str RAM_B out_file CALL open_create_file
  CMPA JMPRZ $fail_open_file
  RAM_B "EX" CALL write_u16
  RAM_BL "E" CALL write_u8

  RAM_B out_file rB_A PUSHA
  INCB INCB rB_AL PUSHA
  -- ^ code_size_file_ndx code_size_file_sec

  RAM_A out_file RAM_BL 0x00 CALL write_u16 -- code_size for now

  RAM_A buffer PUSHA
loop:
  -- ^ bufferi code_size_file_ndx code_size_file_sec
  RAM_A file CALL read_u8 A_B
  INCA JMPRZ $loop_end DECA
  RAM_AL 0x0A SUB JMPRNZ $not_newline
  RAM_BL " "
not_newline:
  -- ^ bufferi code_size_file_ndx code_size_file_sec [_, char]
  RAM_AL " " SUB JMPRNZ $not_space
  PEEKB RAM_A buffer SUB JMPRZ $loop
  POPB RAM_AL 0x00 AL_rB
  RAM_A buffer PUSHA
  CALLR $parse
  JMPR $loop
not_space:
  -- ^ bufferi code_size_file_ndx code_size_file_sec [_, char]
  B_A PEEKB AL_rB
  POPA INCA PUSHA
  JMPR $loop

loop_end:
  -- ^ bufferi code_size_file_ndx code_size_file_sec 
  POPB RAM_A buffer SUB JMPRZ $not_unparsed
  -- ^ code_size_file_ndx code_size_file_sec [_, bufferi]
  RAM_AL 0x00 AL_rB
  CALLR $parse
not_unparsed:

  CALLR $unalign
  RAM_A out_file RAM_BL RAM_AL CALL write_u8
  RAM_BL 0x00 CALL write_u8
  RAM_BL CALL CALL write_u8
  RAM_BL 0x00 CALL write_u16 -- exit placeholder
  -- writing: unalign RAM_AL 0x00 CALL exit
  RAM_B code_size rB_A INCA INCA INCA INCA INCA A_rB
  -- ^ code_size_file_ndx code_size_file_sec
  RAM_A exit CALLR $push_dynamic_reloc

  RAM_A out_file RAM_BL 0x00 CALL write_u16 -- reloc count
  RAM_BL 0x01 CALL write_u16 -- dynamic linking count
  RAM_B 0x0101 CALL write_u16 -- file name and size

  -- copy the dynamic_reloc list to the file
  RAM_B dr_index rB_A A_B RAM_A dynamic_relocs SUB SHR SHR -- reloc_count = (*dr_index - dynamic_relocs) / 4
  PUSHA A_B RAM_A out_file CALL write_u8 -- reloc count
  -- ^ reloc_count code_size_file_ndx code_size_file_sec
  POPA SHL
  RAM_B dynamic_relocs PUSHB
copy_dynamic_reloc:
  PUSHA
  -- ^ n_iterations entry_ptr ...
  PEEKAR 0x04 A_B rB_A A_B RAM_A out_file CALL write_u16
  PEEKAR 0x04 INCA INCA PUSHAR 0x04
  POPA DECA JMPRNZ $copy_dynamic_reloc
  INCSP
  -- ^ code_size_file_ndx code_size_file_sec

  RAM_A out_file RAM_BL 0x00 CALL write_u16 -- symbol list count

  -- ^ code_size_file_ndx code_size_file_sec
  POPA RAM_B out_file INCB INCB AL_rB
  POPA RAM_B out_file A_rB
  RAM_B code_size rB_A A_B RAM_A out_file CALL write_u16

  RAM_AL 0x00 CALL exit

out_str: "stack.out" 0x00

no_arg_str: "ERROR: expected file as arg" 0x0A 0x00
no_arg:
  RAM_A no_arg_str CALL print
  RAM_AL 0x01 CALL exit

fail_open_file_str: "ERROR: failed opening file" 0x00
fail_open_file:
  RAM_A fail_open_file_str CALL print
  RAM_AL 0x01 CALL exit

ALIGN
dr_index: 0x0000
dynamic_relocs:
