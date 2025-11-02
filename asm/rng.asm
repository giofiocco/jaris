GLOBAL rng_next

-- [RNG &rng, _] -> [u16 n, _]
-- returns a random number modifying the RNG
-- RNG is a u16 initialized with the wanted seed
-- and then used only for the rng_next func
-- example:
-- | rng: db 2
-- | ...
-- | RAM_B rng RAM_A 0x4123 A_rB -- init with seed
-- | ...
-- | RAM_A rng CALL rng_next -- return a number and modify the rng
-- with parameters a: 36969, c: 18000, m: 2**16
rng_next:
  PUSHA A_B rB_A
  -- ^ &rng [rng, _]
  -- keep in A the shifted number and push it when sum, on B the partial result
  A_B
  SHL SHL SHL PUSHA SUM A_B POPA
  SHL SHL PUSHA SUM A_B POPA
  SHL PUSHA SUM A_B POPA
  SHL SHL SHL SHL SHL SHL PUSHA SUM A_B POPA
  SHL SHL SHL PUSHA SUM A_B POPA
  RAM_A 0x4650 SUM
  POPB A_rB
  RET
