GLOBAL execute
EXTERN open_file
EXTERN read_char
EXTERN allocate_page

-- [cstr path, cstr argv[] (null terminated)] -> [_, _]
execute:
  PUSHB PUSHA

  CALL allocate_page
  HLT



