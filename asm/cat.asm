GLOBAL _start
EXTERN print
EXTERN exit
EXTERN open_file
EXTERN read_u8
EXTERN put_char

no_arg_string: "ERROR: expected an arg" 0x0A 0x00
not_found_string: "ERROR: file not found" 0x0A 0x00
file: db 4

-- Usage: cat <file>
-- prints the file content
_start:
  CMPB JMPRZ $no_arg

  RAM_B file CALL open_file
  CMPB JMPRN $not_found
loop:
  RAM_A file CALL read_u8
  INCA JMPRZ $end
  DECA CALL put_char
  JMPR $loop

end:
  CALL exit

no_arg:
  RAM_A no_arg_string CALL print
  CALL exit

not_found:
  RAM_A not_found_string CALL print
  CALL exit
