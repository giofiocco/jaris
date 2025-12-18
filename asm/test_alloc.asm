GLOBAL _start

EXTERN exit
EXTERN alloc
EXTERN free
EXTERN print_int
EXTERN put_char

_start:
       RAM_AL 0x03 CALL alloc PUSHA CALL print_int RAM_AL 0x0A CALL put_char
       RAM_AL 0x02 CALL alloc PUSHA CALL print_int RAM_AL 0x0A CALL put_char
  POPA RAM_BL 0x02 CALL free
       RAM_AL 0x04 CALL alloc       CALL print_int RAM_AL 0x0A CALL put_char
  POPA RAM_BL 0x03 CALL free
       RAM_AL 0x04 CALL alloc       CALL print_int RAM_AL 0x0A CALL put_char
       RAM_AL 0x02 CALL alloc       CALL print_int RAM_AL 0x0A CALL put_char
       RAM_AL 0x01 CALL alloc       CALL print_int RAM_AL 0x0A CALL put_char

  RAM_AL 0x00 CALL exit
