GLOBAL mul

-- [a, b] -> [a*b, _]
mul:
  PUSHB PUSHA RAM_BL 0x00 PUSHB
loop:
  -- ^ c a b [a, _]
  SHR JMPRNC $end_if -- if (a & 1) goto end_if
  PEEKAR 0x06 POPB SUM PUSHA -- c += b
end_if:
  -- ^ c a b 
  PEEKAR 0x06 SHL PUSHAR 0x06 -- b <<= 1
  PEEKAR 0x04 SHR PUSHAR 0x04 -- a >>= 1
  CMPA JMPRNZ $loop
  POPA INCSP INCSP
  RET

