GLOBAL _start
EXTERN print
EXTERN put_char
EXTERN print_int
EXTERN get_char
EXTERN exit

{ len 0x0A }
mem: db 10

print_mem:
  RAM_AL len
  RAM_B mem
printloop:
  PUSHA PUSHB
  -- ^ ptr count
  PEEKB rB_AL CALL print_int
  RAM_AL 0x20 CALL put_char
  POPA INCA A_B
  POPA DECA JMPRNZ $printloop
  RAM_AL 0x0A CALL put_char
  RET

-- [ptr, _]
-- exit with 1 and print error if outof bound
boundcheck:
  A_B RAM_A mem SUB JMPRN $error
  INCA RAM_BL len SUB JMPRN $error
  RET

error_string: "ERROR: out of boundaries" 0x0A 0x00
error:
  RAM_A error_string CALL print
  CALL exit
noclose_error_string: "ERROR: no ] found" 0x0A 0x00
noclose_error:
  RAM_A noclose_error_string CALL print
  CALL exit
noopen_error_string: "ERROR: no [ found" 0x0A 0x00
noopen_error:
  RAM_A noopen_error_string CALL print
  CALL exit

_start:
  RAM_A mem PUSHA
  PUSHB
loop:
  -- ^ prg ptr
  PEEKB rB_AL CMPA JMPRZ $loopend

  PEEKB rB_AL A_B

  RAM_AL "+" SUB JMPRNZ $_01
  PEEKAR 0x04 A_B rB_AL INCA AL_rB
  JMPR $next_char
_01:
  RAM_AL "-" SUB JMPRNZ $_02
  PEEKAR 0x04 A_B rB_AL DECA AL_rB
  JMPR $next_char
_02:
  RAM_AL ">" SUB JMPRNZ $_03
  PEEKAR 0x04 INCA PUSHAR 0x04
  CALL boundcheck
  JMPR $next_char
_03:
  RAM_AL "<" SUB JMPRNZ $_04
  PEEKAR 0x04 DECA PUSHAR 0x04
  CALL boundcheck
  JMPR $next_char
_04:
  RAM_AL "[" SUB JMPRNZ $_05
  -- ^ prg ptr
  PEEKAR 0x04 A_B rB_AL CMPA JMPRNZ $next_char

  POPB RAM_AL 0x00 PUSHA PUSHB
search_close_par:
  -- ^ prg parcount ptr
  PEEKB rB_AL A_B
  CMPA JMPRZ $noclose_error
  RAM_AL "[" SUB JMPRNZ $__01
  PEEKAR 0x04 INCA PUSHAR 0x04
  JMPR $__02
__01:
  RAM_AL "]" SUB JMPRNZ $__02
  PEEKAR 0x04 DECA JMPRZ $found_close_par PUSHAR 0x04
__02:
  POPA INCA PUSHA JMPR $search_close_par

found_close_par:
  POPA INCSP PUSHA
  JMPR $next_char

_05:
  RAM_AL "]" SUB JMPRNZ $_06
  PEEKAR 0x04 A_B rB_AL CMPA JMPRZ $next_char

  POPB RAM_AL 0x00 PUSHA PUSHB
search_open_par:
  -- ^ prg parcount ptr
  PEEKB rB_AL A_B
  CMPA JMPRZ $noopen_error
  RAM_AL "]" SUB JMPRNZ $__03
  PEEKAR 0x04 INCA PUSHAR 0x04
  JMPR $__04
__03:
  RAM_AL "[" SUB JMPRNZ $__04
  PEEKAR 0x04 DECA JMPRZ $found_open_par PUSHAR 0x04
__04:
  POPA DECA PUSHA JMPR $search_open_par

found_open_par:
  POPA INCSP PUSHA
  JMPR $next_char

_06:
  RAM_AL "." SUB JMPRNZ $_07
  PEEKAR 0x04 A_B rB_AL CALL put_char
  JMPR $next_char
_07:
  RAM_AL "," SUB JMPRNZ $_08
  CALL get_char PUSHA
  PEEKAR 0x06 A_B POPA AL_rB
  JMPR $next_char
_08:

next_char:
  -- CALLR $print_mem
  POPA INCA PUSHA JMPR $loop

loopend:
  RAM_AL 0x0A CALL put_char
  CALLR $print_mem

  RAM_AL 0x00 CALL exit
