GLOBAL div

-- [a, b] -> [b div a, b mod a]
div:
  -- 0x11 = 17
  PUSHA PUSHB RAM_AL 0x11 PUSHA RAM_AL 0x00 PUSHA PUSHA
  -- ^ q r i b a [_, b]
  CMPB JMPRZ $end -- if (b == 0) goto end
loop:
  -- ^ q r i b a
  PEEKAR 0x06 DECA -- i - 1 
  JMPRZ $end -- if (i == 0) goto end
  PUSHAR 0x06 -- i = i - 1
  PEEKAR 0x04 SHL PUSHAR 0x04 -- r = r << 1
  POPA SHL PUSHA -- q = q << 1
  PEEKAR 0x08 SHL JMPRNC $noneg -- b << 1
  A_B
  PEEKAR 0x04 INCA PUSHAR 0x04 -- r = r + 1
  B_A
noneg:
  -- ^ q r i b a [b << 1, _]
  PUSHAR 0x08 -- b = b << 1
  PEEKAR 0x04 A_B PEEKAR 0x0A SUB JMPRN $loop -- if (r - a < 0) goto loop
  PUSHAR 0x04 -- r = r - a
  POPA INCA PUSHA -- q++
  JMPR $loop
end:
  -- ^ q r i b a
  POPA POPB INCSP INCSP INCSP
  RET
  

