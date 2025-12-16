GLOBAL _start

EXTERN exit
EXTERN print
EXTERN open_create_file
EXTERN get_char
EXTERN put_char
EXTERN write_u8

file: db 4
error_no_file_string: "ERROR: no file name provided" 0x0A 0x00

_start:
  CMPB JMPRNZ $no_error
  RAM_A error_no_file_string CALL print
  RAM_AL 0x01 CALL exit
no_error:

  B_A RAM_B file CALL open_create_file
loop:
  CALL get_char PUSHA CALL put_char
  RAM_A file PEEKB CALL write_u8
  POPA INCA JMPRNZ $loop -- jmp if != 0xFFFF

  RAM_AL 0x00 CALL exit
