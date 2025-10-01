GLOBAL div
GLOBAL mul
GLOBAL shiftl
GLOBAL shiftr

-- [a, b] -> [b div a, b mod a]
div:
  -- 0x11 = 17
  PUSHA PUSHB RAM_AL 0x11 PUSHA RAM_AL 0x00 PUSHA PUSHA
  -- ^ q r i b a [_, b]
  CMPB JMPRZ $div_end -- if (b == 0) goto div_end
div_loop:
  -- ^ q r i b a
  PEEKAR 0x06 DECA -- i - 1 
  JMPRZ $div_end -- if (i == 0) goto div_end
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
  PEEKAR 0x04 A_B PEEKAR 0x0A SUB JMPRN $div_loop -- if (r - a < 0) goto div_loop
  PUSHAR 0x04 -- r = r - a
  POPA INCA PUSHA -- q++
  JMPR $div_loop
div_end:
  -- ^ q r i b a
  POPA POPB INCSP INCSP INCSP
  RET

-- [a, b] -> [a*b, _]
mul:
  PUSHB PUSHA RAM_BL 0x00 PUSHB
mul_loop:
  -- ^ c a b [a, _]
  SHR JMPRNC $end_if -- if (a & 1) goto end_if
  PEEKAR 0x06 POPB SUM PUSHA -- c += b
end_if:
  -- ^ c a b 
  PEEKAR 0x06 SHL PUSHAR 0x06 -- b <<= 1
  PEEKAR 0x04 SHR PUSHAR 0x04 -- a >>= 1
  CMPA JMPRNZ $mul_loop
  POPA INCSP INCSP
  RET

-- [a, b] -> [a << b, _]
shiftl:
  CMPB JMPRZ $shiftl_ret
  SHL PUSHA
  B_A DECA A_B POPA
  JMPR $shiftl
shiftl_ret:
  RET

-- [a, b] -> [a >> b, _]
shiftr:
  CMPB JMPRZ $shiftr_ret
  SHR PUSHA
  B_A DECA A_B POPA
  JMPR $shiftr
shiftr_ret:
  RET

