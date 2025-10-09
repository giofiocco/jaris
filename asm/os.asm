GLOBAL _start

EXTERN execute
EXTERN load_font
EXTERN print
EXTERN mul
EXTERN shiftr

{ page_size 0x0800 }

stdout: db 128

_start:
  -- ^ [_, tot_size] (os code_size + stdlib code_size)
  B_A SHR SHR SHR SHR SHR SHR SHR SHR SHR SHR SHR
  A_B RAM_A 0x8000 CALL shiftr DECA
  RAM_B 0xFFFF SUB
  RAM_B 0xF806 A_rB

  -- ptr to stdlib set by the bootloader
  RAM_B 0xF802 RAM_A 0xF820 A_rB -- ptr to current process struct
  RAM_B 0xF804 RAM_A 0x8000 A_rB -- used process map
  -- already set used page map
  RAM_B 0xF808 RAM_AL 0x01  A_rB -- used page map
  -- os process
  RAM_B 0xF820 RAM_A 0xFFFF A_rB -- ptr to parent process
  RAM_B 0xF822 RAM_AL 0x01  A_rB -- cwd sec
  RAM_B 0xF824 RAM_A 0xFFFE A_rB -- SP
  RAM_B 0xF826 RAM_A 0xFFFF A_rB -- stdout unset
  -- RAM_B 0xF826 RAM_A stdout A_rB -- stdout redirect

  -- stdout
  RAM_AL 0x80 RAM_B stdout A_rB
  RAM_AL 0x02 SUM A_B INCA INCA A_rB

  RAM_A font_path CALL load_font

  RAM_A path RAM_BL 0x00 CALL execute

  RAM_B 0x1010
  HLT

font_path: "font" 0x00
path: "sh" 0x00
