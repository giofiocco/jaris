GLOBAL load_font
EXTERN open_file

file: db 4

-- [cstr path, _] -> [_, _]
-- set patterns in the file
-- but not uset the others
-- ERRORS:
-- [0, 0xFFFF] if file not found
load_font:
  RAM_B file CALL open_file

  RAM_AL 0xAA HLT
