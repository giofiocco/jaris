GLOBAL _start

  { last_char 0xF80E }

buffer: db 256
ALIGN args: db 128
file_not_found_str: "command not found: '" 0x00
non_zero_exit_code_str: "non zero exit code" 0x0A 0x00

_start:
  RAM_A args PUSHA
  RAM_A buffer PUSHA
loop:
  -- ^ bufferi argsi
  RAM_AL " " PEEKB
  CALLrRAM get_delim_ptr
  CMPA JMPRZ $loop -- empty string
  INCA JMPRZ $quit -- end of file

  POPB SUM PUSHA -- bufferi += length
  -- ^ bufferi argsi [_, bufferi_old]
  B_A INCSP POPB A_rB -- *argsi = bufferi_old
  B_A INCA INCA PUSHA DECSP -- argsi += 2 

  RAM_B last_char rB_A RAM_BL 0x0A SUB JMPRNZ $loop

  INCSP POPB RAM_AL 0x00 A_rB -- *argsi = 0
  -- ^ 

  RAM_A args INCA INCA A_B
  RAM_A buffer
  CALLrRAM execute_ptr
  INCA JMPRZ $not_found
  DECA JMPRNZ $non_zero_exit_code

  JMPR $_start

quit:
  RAM_AL 0x00
  CALLrRAM exit_ptr

not_found:
  RAM_A file_not_found_str
  CALLrRAM print_ptr
  RAM_A buffer
  CALLrRAM print_ptr
  RAM_AL "'"
  CALLrRAM put_char_ptr
  RAM_AL 0x0A
  CALLrRAM put_char_ptr

  JMPR $_start

non_zero_exit_code:
  RAM_A non_zero_exit_code_str
  CALLrRAM print_ptr
  JMPR $_start
