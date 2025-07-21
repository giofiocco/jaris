GLOBAL _start
EXTERN exit
EXTERN print
EXTERN put_char
EXTERN print_int
EXTERN get_char


ALIGN
mem: db 16
mem_end:

ALIGN
code: db 512
code_end:
ALIGN code_ptr: 0x0000

ALIGN
jmp_stack: db 32
ALIGN jmp_head_ptr: 0x0000

{ OP_NONE  0x00 } -- always 0x00
{ OP_PLUS  "+" }
{ OP_MINUS "-" }
{ OP_LEFT  "<" }
{ OP_RIGHT ">" }
{ OP_OPEN  "[" }
{ OP_CLOSE "]" }
{ OP_COMMA "," }
{ OP_POINT "." }

check_and_push:
  -- ^ ptr times lastop [op, opposite_op]
  PUSHA
  -- ^ op ptr times lastop [op, opposite_op]
  PEEKAR 0x08 SUB JMPRNZ $not_opposite_op
  -- TODO: doesnt arrive here
  INCSP
  -- ^ ptr times lastop
  PEEKAR 0x06 DECA PUSHAR 0x06
  CMPA JMPRNZ $found_op
  PUSHAR 0x06 -- [0x00, _] lastop = OP_NONE
  JMPR $found_op
not_opposite_op:
  -- ^ op ptr times lastop
  PEEKAR 0x08 A_B PEEKA SUB JMPRZ $equal_op
  PEEKAR 0x06 CALLR $push_op
  POPA PUSHAR 0x08
  RAM_AL 0x00 PUSHAR 0x06
equal_op:
  -- ^ ptr times lastop
  PEEKAR 0x04 INCA PUSHAR 0x04
  JMPR $found_op

-- Usage: bfjit program
_start:
  RAM_AL 0x00 PUSHA PUSHA
  PUSHB
  -- ^ prg times lastop

  RAM_B code_ptr RAM_A code A_rB
  RAM_B jmp_head_ptr RAM_A jmp_stack A_rB

  RAM_AL NOP CALLR $push_inst
  RAM_AL RAM_A CALLR $push_inst
  RAM_B code_ptr rB_A A_B RAM_A mem A_rB
  RAM_B code_ptr rB_A INCA INCA A_rB
  RAM_AL PUSHA CALLR $push_inst
  RAM_AL NOP CALLR $push_inst

loop:
  -- ^ prg times lastop
  PEEKB rB_AL CMPA JMPRZ $end_loop A_B

  -- ^ prg times lastop [_, char]
  RAM_AL "+" SUB JMPRNZ $not_plus
  RAM_AL OP_PLUS RAM_BL OP_MINUS JMPR $check_and_push
not_plus:
  RAM_AL "-" SUB JMPRNZ $not_minus
  RAM_AL OP_MINUS RAM_BL OP_PLUS JMPR $check_and_push
not_minus:
  RAM_AL "<" SUB JMPRNZ $not_left
  RAM_AL OP_LEFT RAM_BL OP_RIGHT JMPR $check_and_push
not_left:
  RAM_AL ">" SUB JMPRNZ $not_right
  RAM_AL OP_RIGHT RAM_BL OP_LEFT JMPR $check_and_push
not_right:
  RAM_AL "[" SUB JMPRNZ $not_open
  PEEKAR 0x06 A_B PEEKAR 0x04 CALLR $push_op
  RAM_AL 0x01 RAM_BL OP_OPEN CALLR $push_op
  RAM_AL 0x00 PUSHAR 0x06 PUSHAR 0x04
  JMPR $found_op
not_open:
  RAM_AL "]" SUB JMPRNZ $not_close
  PEEKAR 0x06 A_B PEEKAR 0x04 CALLR $push_op
  RAM_AL 0x01 RAM_BL OP_CLOSE CALLR $push_op
  RAM_AL 0x00 PUSHAR 0x06 PUSHAR 0x04
  JMPR $found_op
not_close:
  RAM_AL "," SUB JMPRNZ $not_comma
  PEEKAR 0x06 A_B PEEKAR 0x04 CALLR $push_op
  RAM_AL 0x01 RAM_BL OP_COMMA CALLR $push_op
  RAM_AL 0x00 PUSHAR 0x06 PUSHAR 0x04
  JMPR $found_op
not_comma:
  RAM_AL "." SUB JMPRNZ $not_point
  PEEKAR 0x06 A_B PEEKAR 0x04 CALLR $push_op
  RAM_AL 0x01 RAM_BL OP_POINT CALLR $push_op
  RAM_AL 0x00 PUSHAR 0x06 PUSHAR 0x04
  JMPR $found_op
not_point:
found_op:
  POPA INCA PUSHA
  JMPR $loop
end_loop:
  PEEKAR 0x06 A_B PEEKAR 0x04 CALLR $push_op
  RAM_AL INCSP CALLR $push_inst
  RAM_AL RET CALLR $push_inst

  CALL code

  -- print_mem
  RAM_B mem_end RAM_A mem SUB
  RAM_B mem
print_mem_loop:
  PUSHA PUSHB
  -- ^ ptr count
  PEEKB rB_AL CALL print_int
  RAM_AL " " CALL put_char
  POPA INCA A_B
  POPA DECA JMPRNZ $print_mem_loop

  RAM_AL 0x0A CALL put_char
  RAM_A code_len_string CALL print
  RAM_B code_ptr rB_A A_B RAM_A code SUB CALL print_int
  RAM_AL 0x0A CALL put_char
  RAM_AL 0x00 CALL exit

code_len_string: "code len: " 0x00

-- [count, op] -> [_, _]
push_op:
  PUSHA
  -- ^ count
  CMPA JMPRZ $end_push_op
  CMPB JMPRZ $end_push_op

  -- PUSHB B_A CALL put_char PEEKAR 0x04 CALL print_int RAM_AL 0x0A CALL put_char POPB

  RAM_AL OP_PLUS SUB JMPRNZ $push_not_plus
  PEEKA CMPA JMPRZ $end_push_op
  RAM_BL 0x01 SUB JMPRZ $push_inc
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL RAM_BL CALLR $push_inst
  PEEKA CALLR $push_inst
  RAM_AL SUM CALLR $push_inst
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL AL_rB CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  JMPR $end_push_op
push_inc:
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL INCA CALLR $push_inst
  RAM_AL AL_rB CALLR $push_inst
  JMPR $end_push_op

push_not_plus:
  RAM_AL OP_MINUS SUB JMPRNZ $push_not_minus
  PEEKA CMPA JMPRZ $end_push_op
  RAM_BL 0x01 SUB JMPRZ $push_dec
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL A_B CALLR $push_inst
  RAM_AL RAM_AL CALLR $push_inst
  PEEKA CALLR $push_inst
  RAM_AL SUB CALLR $push_inst
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL AL_rB CALLR $push_inst
  JMPR $end_push_op
push_dec:
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL DECA CALLR $push_inst
  RAM_AL AL_rB CALLR $push_inst
  JMPR $end_push_op

push_not_minus:
  RAM_AL OP_LEFT SUB JMPRNZ $push_not_left
  PEEKA CMPA JMPRZ $end_push_op
  RAM_BL 0x01 SUB JMPRZ $push_left_one
  RAM_AL POPB CALLR $push_inst
  RAM_AL RAM_AL CALLR $push_inst
  PEEKA CALLR $push_inst
  RAM_AL SUB CALLR $push_inst
  RAM_AL PUSHA CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  JMPR $end_push_op
push_left_one:
  RAM_AL POPA CALLR $push_inst
  RAM_AL DECA CALLR $push_inst
  RAM_AL PUSHA CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  JMPR $end_push_op

push_not_left:
  RAM_AL OP_RIGHT SUB JMPRNZ $push_not_right
  PEEKA CMPA JMPRZ $end_push_op
  RAM_BL 0x01 SUB JMPRZ $push_right_one
  RAM_AL POPB CALLR $push_inst
  RAM_AL RAM_AL CALLR $push_inst
  PEEKA CALLR $push_inst
  RAM_AL SUM CALLR $push_inst
  RAM_AL PUSHA CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  JMPR $end_push_op
push_right_one:
  RAM_AL POPA CALLR $push_inst
  RAM_AL INCA CALLR $push_inst
  RAM_AL PUSHA CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  JMPR $end_push_op

push_not_right:
  RAM_AL OP_OPEN SUB JMPRNZ $push_not_open

  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL CMPA CALLR $push_inst
  RAM_AL JMPRZ CALLR $push_inst
  RAM_B code_ptr rB_A INCA INCA A_rB

  -- TODO: check overflow
  RAM_B jmp_head_ptr rB_A PUSHA
  RAM_B code_ptr rB_A POPB A_rB
  B_A INCA INCA RAM_B jmp_head_ptr A_rB

  JMPR $end_push_op

push_not_open:
  RAM_AL OP_CLOSE SUB JMPRNZ $push_not_close

  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL CMPA CALLR $push_inst
  RAM_AL JMPRNZ CALLR $push_inst

  -- TODO: check underflow
  RAM_B jmp_head_ptr rB_A DECA DECA A_rB
  A_B rB_A PUSHA
  RAM_B code_ptr rB_A
  A_B PEEKA SUB PUSHA
  -- ^ diff_code open_code_pos count

  RAM_BL 0x00 SUB PUSHA RAM_B code_ptr rB_A A_B POPA A_rB -- *code_ptr = -diff_code
  RAM_B code_ptr rB_A INCA INCA A_rB
  PEEKAR 0x04 DECA DECA A_B A_B POPA A_rB -- *(open_code_pos - 2) = diff_code + 2
  INCSP
  -- ^ count
  JMPR $end_push_op

push_not_close:
  RAM_AL OP_COMMA SUB JMPRNZ $push_not_comma
  RAM_AL NOP CALLR $push_inst
  RAM_AL CALL CALLR $push_inst
  RAM_B code_ptr rB_A A_B RAM_A get_char A_rB
  RAM_B code_ptr rB_A INCA INCA A_rB
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL AL_rB CALLR $push_inst
  JMPR $end_push_op

push_not_comma:
  RAM_AL OP_POINT SUB JMPRNZ $push_not_point
  RAM_AL PEEKB CALLR $push_inst
  RAM_AL rB_AL CALLR $push_inst
  RAM_AL NOP CALLR $push_inst
  RAM_AL CALL CALLR $push_inst
  RAM_B code_ptr rB_A A_B RAM_A put_char A_rB
  RAM_B code_ptr rB_A INCA INCA A_rB
  JMPR $end_push_op

push_not_point:
end_push_op:
  -- ^ count
  INCSP RET

-- [inst, _] -> [_, _]
push_inst:
  PUSHA
  RAM_B code_ptr rB_A A_B POPA AL_rB
  RAM_B code_ptr rB_A INCA A_rB
  RAM_B code_end SUB JMPRZ $code_exceeding
  RET

code_exceeding_string: "ERROR: code exceeding" 0x0A 0x00
code_exceeding:
  RAM_A code_exceeding_string CALL print
  RAM_AL 0x01 CALL exit
