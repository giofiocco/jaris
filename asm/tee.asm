GLOBAL _start

EXTERN exit
EXTERN print
EXTERN open_create_file
EXTERN get_char
EXTERN put_char

file: db 4

_start:
  CMPB JMPRZ $error_no_file

  B_A RAM_B file CALL open_create_file

loop:
  CALL get_char PUSHA CALL put_char
  POPA RAM_BL 0x0A SUB JMPRNZ $loop

  RAM_AL 0x00 CALL exit

error_no_file_string: "ERROR: no file name provided" 0x0A 0x00
error_no_file:
  RAM_A error_no_file_string CALL print
  RAM_AL 0x01 CALL exit
