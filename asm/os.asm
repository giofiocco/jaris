GLOBAL _start

EXTERN execute
EXTERN load_font
EXTERN print

_start:
  -- ptr to stdlib set by the bootloader
  RAM_B 0xF802 RAM_A 0xF820 A_rB -- ptr to current process struct 
  RAM_B 0xF804 RAM_A 0x8000 A_rB -- used process map
  RAM_B 0xF806 RAM_A 0x8000 A_rB -- used page map
  RAM_B 0xF808 RAM_A 0x0000 A_rB -- used page map
  RAM_B 0xF80A RAM_A 0x0000 A_rB -- screen text row and col
  -- os process
  RAM_B 0xF820 RAM_A 0xFFFF A_rB -- ptr to parent process
  RAM_B 0xF822 RAM_A 0x0001 A_rB -- cwd sec
  RAM_B 0xF824 RAM_A 0xFFFE A_rB -- SP
  RAM_B 0xF826 RAM_A 0xFFFF A_rB -- stdout struct ptr

  RAM_A font_path CALL load_font

  -- RAM_A test_font CALL print HLT

  RAM_A path RAM_BL 0x00 CALL execute

  RAM_B 0x0000
  HLT

font_path: "font" 0x00
path: "sh" 0x00

test_font: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" 0x0A
           "abcdefghijklmnopqrstuvwxyz" 0x0A
           "0123456789" 0x0A
           "!" 0x22 "#$%&'()*+,-./:;<=>?@[\]^_`{|}"
           0x00
