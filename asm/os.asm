GLOBAL _start
EXTERN open_file
EXTERN read_char

_start:
  -- ptr to stdlib set by the bootloader
  RAM_B 0xF802 RAM_A 0xF820 A_rB -- ptr to current process struct 
  RAM_B 0xF804 RAM_A 0x8000 A_rB -- used process map
  RAM_B 0xF806 RAM_A 0x8000 A_rB -- used page map
  RAM_B 0xF808 RAM_A 0x0000 A_rB -- used page map
  -- os process
  RAM_B 0xF820 RAM_A 0xFFFF A_rB -- ptr to parent process  
  RAM_B 0xF822 RAM_A 0x0001 A_rB -- cwd sec 
  RAM_B 0xF824 RAM_A 0xFFFE A_rB -- SP 
  RAM_B 0xF826 RAM_A 0x8000 A_rB
  RAM_B 0xF828 RAM_A 0x0000 A_rB

  RAM_A path
  RAM_B file
  CALL open_file

  RAM_A file CALL read_char
  RAM_A file CALL read_char
  RAM_A file CALL read_char
  RAM_A file CALL read_char
  RAM_A file CALL read_char
  RAM_A file CALL read_char

  HLT

path: "a/b" 0x00
ALIGN file: db 4
