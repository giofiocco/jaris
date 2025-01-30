GLOBAL exit

  { process_table_start 0xF820 }
  { process_map_ptr 0xF804 }
  { current_process_ptr 0xF802 }
  { SP_offset 0x04 }

-- [int exit_code, _] -> [_, _]
exit:
  PUSHA
  -- index = (current_process - process_table_start) / 16
  RAM_B current_process_ptr rB_A
  A_B RAM_A process_table_start SUB
  SHR SHR SHR SHR
  RAM_B 0x8000 PUSHB
compute_mask:
  -- ^ mask exit_code [index, _]
  A_B POPA SHR PUSHA B_A DECA JMPRNZ $compute_mask
  -- mask = 0x8000 << index

  -- ^ process_mask exit_code
  RAM_B process_map_ptr rB_A A_B POPA SUB
  RAM_B process_map_ptr A_rB -- *process_map_ptr -= process_mask

  RAM_B current_process_ptr rB_A A_B rB_A
  RAM_B current_process_ptr A_rB -- *current_process_ptr = current_process_ptr->parent_process

  RAM_BL SP_offset SUM A_B rB_A POPB A_SP -- SP = current_process_ptr->SP
  -- [_, exit_code]

  B_A RET
