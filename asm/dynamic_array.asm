GLOBAL dynamic_array_init
GLOBAL dynamic_array_push16
GLOBAL dynamic_array_push8
EXTERN alloc

-- dynamic_array_t (size 6 bytes)
-- int data[]; // ptr to data
-- int size;
-- int capacity;

-- [dynamic_array_t *list, _] -> [dynamic_array_t *list, _]
-- inits the dynamic_array struct
dynamic_array_init:
  PUSHA
  RAM_AL 0x01 CALL alloc
  PEEKB A_rB
  RAM_AL 0x02 SUM A_B RAM_AL 0x00 A_rB
  RAM_AL 0x02 SUM A_B RAM_AL 0x80 A_rB
  POPA RET

-- [dynamic_array_t* list, u16 e] -> [dynamic_array_t* list, _]
dynamic_array_push16:
  PUSHB PUSHA
  -- ^ &list e
  -- if size + 2 >= capacity { reloc } -> size - capacity + 2 >= 0
  RAM_BL 0x02 SUM A_B rB_A PUSHA PUSHA
  RAM_AL 0x02 SUM A_B rB_A
  POPB SUB INCA INCA JMPRNN $reloc
  -- ^ size &list e
  PEEKAR 0x04 A_B rB_A A_B POPA SUM A_B
  -- ^ &list e
  PEEKAR 0x04 A_rB

  PEEKB RAM_AL 0x02 SUM A_B rB_A INCA INCA A_rB
  POPA INCSP RET

-- [dynamic_array_t* list, u8 e] -> [dynamic_array_t* list, _]
dynamic_array_push8:
  PUSHB PUSHA
  -- ^ &list e
  -- if size + 1 >= capacity { reloc } -> size - capacity + 1 >= 0
  RAM_BL 0x02 SUM A_B rB_A PUSHA PUSHA
  RAM_AL 0x02 SUM A_B rB_A
  POPB SUB INCA JMPRNN $reloc
  -- ^ size &list e
  PEEKAR 0x04 A_B rB_A A_B POPA SUM A_B
  -- ^ &list e
  PEEKAR 0x04 AL_rB

  PEEKB RAM_AL 0x02 SUM A_B rB_A INCA A_rB
  POPA INCSP RET

reloc:
  RAM_A 0xAB03 HLT
