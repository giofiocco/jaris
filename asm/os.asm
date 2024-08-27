GLOBAL _start
EXTERN mul
EXTERN div
EXTERN solve_path
EXTERN open_file
EXTERN get_file
EXTERN get_file16
EXTERN close_file
EXTERN execute
EXTERN exit
EXTERN put_char
EXTERN print
EXTERN get_char
EXTERN get_delim

stdout:
  db 256
stdout_end:

_start:
  -- stdout
  RAM_A stdout RAM_BL 0x04 SUM RAM_B stdout A_rB
  RAM_A stdout RAM_BL 0x02 SUM A_B RAM_A stdout_end A_rB
  -- os struct
  RAM_B 0xF800 RAM_A 0xF870 A_rB -- ptr to next process
  RAM_B 0xF802 RAM_A 0xF860 A_rB -- ptr to current process
  RAM_B 0xF804 RAM_A 0x8000 A_rB -- in-use-page map
  RAM_B 0xF806 RAM_A 0x0000 A_rB -- in-use-page map
  RAM_B 0xF808 RAM_A 0x0000 A_rB -- in-use-fd map
  RAM_B 0xF80A RAM_A 0x0000 A_rB -- in-use-fd map
  RAM_B 0xF80C RAM_A stdout A_rB -- stdout
  -- PIC table
  RAM_B mul_ptr RAM_A mul A_rB
  RAM_B div_ptr RAM_A div A_rB
  RAM_B solve_path_ptr RAM_A solve_path A_rB
  RAM_B open_file_ptr RAM_A open_file A_rB
  RAM_B get_file_ptr RAM_A get_file A_rB
  RAM_B get_file16_ptr RAM_A get_file16 A_rB
  RAM_B close_file_ptr RAM_A close_file A_rB
  RAM_B execute_ptr RAM_A execute A_rB
  RAM_B exit_ptr RAM_A exit A_rB
  RAM_B put_char_ptr RAM_A put_char A_rB
  RAM_B print_ptr RAM_A print A_rB
  RAM_B get_char_ptr RAM_A get_char A_rB
  RAM_B get_delim_ptr RAM_A get_delim A_rB
  -- os process 
  RAM_B 0xF860 RAM_A 0xFFFF A_rB -- parent process
  RAM_B 0xF862 RAM_A 0x0001 A_rB -- cwd
  RAM_B 0xF864 RAM_A 0x0000 A_rB -- SP
  RAM_B 0xF866 RAM_A 0x8000 A_rB -- iup mask
  RAM_B 0xF868 RAM_A 0x0000 A_rB -- iup mask
  -- file table
  ---------------------

  RAM_A path RAM_BL 0x00
  CALL execute

  RAM_B 0xAAAA
  HLT

path: "bash" 0x00
