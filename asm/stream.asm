GLOBAL stream_init
GLOBAL stream_write
GLOBAL stream_read

-- [STREAM *stream, u16 size] -> [_, _]
-- size should be the size of the struct
stream_init:
  PUSHA
  SUM PUSHA -- stream + size
  -- ^ ending stream
  PEEKAR 0x04 RAM_BL 0x04 SUM A_B POPA A_rB -- set ending
  -- ^ stream [_, &ending]
  PEEKA RAM_BL 0x06 SUM -- data
  POPB PUSHA
  -- ^ data [data, &next_r]
  A_rB -- set next_r
  RAM_AL 0x02 SUM A_B POPA A_rB -- set next_w
  RET

-- [STREAM *stream, u8 char] -> [_, _]
-- CRASH [0xEFFA, _] if stdout full
stream_write:
  PUSHB
  INCA INCA PUSHA A_B rB_A PUSHA
  -- ^ next_w &next_w char
  RAM_AL 0x02 SUM A_B rB_A -- ending
  POPB SUB DECA JMPRN $not_full -- if (next_w <= ending) not_full => next_w - ending - 1 < 0
  RAM_A 0xEFFA HLT
not_full:
  -- ^ &next_w char [_, next_w]
  PEEKAR 0x04 AL_rB
  B_A INCA POPB A_rB
  -- ^ char
  INCSP RET

-- [STREAM *stream, _] -> [u8 char, _]
-- returns [0xFFFF, _] if empty
stream_read:
  PUSHA
  A_B rB_A PUSHA -- next_r
  RAM_AL 0x02 SUM A_B rB_A -- next_w
  PEEKB SUB JMPRN $not_empty -- if (next_r < next_w) not_empty
  -- ^ next_r stream
  INCSP INCSP RAM_A 0xFFFF RET
not_empty:
  -- ^ next_r stream
  POPA INCA POPB A_rB
  DECA A_B rB_AL RET
