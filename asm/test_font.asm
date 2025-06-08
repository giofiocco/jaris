GLOBAL _start

EXTERN print
EXTERN exit

_start:
  RAM_A string CALL print
  RAM_AL 0x00 CALL exit


string: "ABCDEFGHIJKLMNOPQRSTUVWXYZ" 0x0A
        "abcdefghijklmnopqrstuvwxyz" 0x0A
        "0123456789" 0x0A
        "!" 0x22 "#$%&'()*+,-./:;<=>?@[\]^_`{|}"
        0x0A 0x00
