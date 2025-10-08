GLOBAL alloc
EXTERN shiftr
EXTERN shiftl

  { iup2_ptr 0xF808 }

-- pseudo code:
--
-- fail if 0 < count <= 256
--
-- page_num = 0 // the number of the page that is checking
-- _page_map = pages_map // the page map shifted with on the lsb the page checking
--
-- loop:
--   if _page_map == 0 {
--     // alloc page
--     _page_map = iup2
--     page_num = 0
--     page_mask = 1
--     while _page_map & 1 != 0 {
--       page_num ++
--       if page_num == 16 { fail no more pages }
--       page_mask <<= 1
--       _page_map >>= 1
--     }
--     iup2 += page_mask
--     pages_map += page_mask
--     _page_map = pages_map >> page_num
--   }
--   while _page_map & 1 == 0 {
--     _page_map >>= 1
--     page_num ++
--     if page_num == 16 { fail no more pages }
--   }
--
--   block_map = block_map_list[page_num]
--   blocks_mask = (1 << count) - 1
--   block_num = count - 1 // the num from the last block of the left-most block
--   // block_map:  0010010100010001
--   // block_mask:          11
--   //                      ^
--   // block_num:           \- 5
--
--   while block_map & blocks_mask != 0 {
--     block_mask <<= 1
--     block_num ++
--     if block_num == 16 {
--       jmp loop
--       TODO: is it right?
--     }
--   }
--
--   block_map_list[page_num] = block_map + block_mask
--   return 2048*(31 - page_num) + 128 * (15 - block_num)

pages_map: 0x0000 -- 2^ half of ram
block_map_list: db 32 -- 2 bytes per map * 16 pages (upper half)

-- [u16 count, _] -> [u8* ptr, _]
-- allocate `count` contiguous blocks (128 bytes wide) in a page reserved for it
-- crash [0xDFFA, _] if no more pages
-- ERROR:
-- [0, count] if invalid alloc count (not 0 < count <= 16)
alloc:
  A_B CMPA JMPRZ $invalid_alloc_count
  RAM_AL 0x10 SUB JMPRNN $invalid_alloc_count -- if count - 16 > 0
  PUSHB
  -- ^ count

  RAM_AL 0x00 PUSHA
  RAM_A pages_map PUSHA
loop:
  -- ^ _pages_map page_num count
  PEEKA CMPA JMPRZ $has_pages
  -- ALLOC PAGES
  INCSP INCSP
  RAM_AL 0x00 PUSHA -- page_num = 0
  RAM_B iup2_ptr rB_A PUSHA -- _pages_map = iup2
  RAM_AL 0x01 PUSHA -- page_mask = 1
search_free_iup:
  -- ^ page_mask _page_map page_num count
  PEEKAR 0x04 SHR JMPRNC $found_free_iup PUSHAR 0x04
  POPA SHL PUSHA
  PEEKAR 0x06 INCA PUSHAR 0x06
  A_B RAM_AL 0x10 SUB JMPRZ $no_more_pages
  JMPR $search_free_iup

found_free_iup:
  -- ^ page_mask _page_map page_num count
  RAM_B iup2_ptr rB_A PEEKB SUM RAM_B iup2_ptr A_rB
  RAM_B pages_map rB_A POPB SUM RAM_B pages_map A_rB
  -- ^ _page_map page_num count [pages_map, _]
  INCSP PEEKB CALL shiftr PUSHA -- _page_map = pages_map >> page_num
has_pages:
  -- ^ _page_map page_num count [_page_map, _]
  SHR JMPRC $found_page
  INCSP PUSHA -- _page_map = _page_map >> 1
  PEEKAR 0x04 INCA PUSHAR 0x04
  A_B RAM_AL 0x10 SUB JMPRZ $no_more_pages
  JMPR $has_pages

found_page:
  -- ^ _page_map page_num count
  PEEKAR 0x06 DECA PUSHA
  -- ^ block_num _page_map page_num count
  PEEKAR 0x06 SHL RAM_B block_map_list SUM A_B rB_A PUSHA
  -- ^ block_map block_num _page_map page_num count
  PEEKAR 0x08 A_B RAM_AL 0x01 CALL shiftl DECA PUSHA
  -- ^ blocks_mask block_map block_num _page_map page_num count
search_block:
  -- ^ blocks_mask block_map block_num _page_map page_num count
  PEEKAR 0x04 PEEKB AND JMPRZ $found_block
  -- TODO:
found_block:
  -- ^ blocks_mask block_map block_num _page_map page_num count
  POPA POPB SUM PUSHA
  -- ^ new_block_map block_num _page_map page_num count
  PEEKAR 0x08 SHL RAM_B block_map_list SUM A_B POPA A_rB
  -- ^ block_num _page_map page_num count
  POPB RAM_BL 0x0F SUB SHL SHL SHL SHL SHL SHL SHL PUSHA
  -- ^ _ _page_map page_num count
  PEEKAR 0x06 RAM_BL 0x1F SUB SHL SHL SHL SHL SHL SHL SHL SHL SHL SHL SHL
  POPB SUM
  -- ^ _page_map page_num count
  INCSP INCSP INCSP RET

invalid_alloc_count:
  -- [_, count]
  RAM_AL 0x00
  RET

no_more_pages:
  RAM_A 0xDFFA HLT
