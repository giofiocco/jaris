GLOBAL _start
EXTERN put_char
EXTERN get_char
EXTERN stream_init
EXTERN str_find_char
EXTERN solve_path
EXTERN print
EXTERN execute
EXTERN print_int
EXTERN exit

  { current_process 0xF802 }
  { cwd_offset 0x02 }
  { stdout_offset 0x06 }
  { stdin_offset 0x08 }
  { stdout_to_stdin_offset 0x02 }

old_stdout: 0x0000
old_stdin:  0x0000
not_found_string: ": command not found" 0x0A 0x00
input: db 128

_start:
  SP_A A_B RAM_AL 0x30 SUB A_B RAM_A stdout SUB A_B -- init stdout stream with size sp - stdout - 0x30
  RAM_A stdout CALL stream_init

  RAM_B current_process rB_A RAM_BL stdout_offset SUM PUSHA -- &current_process->stdout
  A_B rB_A RAM_B old_stdout A_rB
  POPB RAM_AL stdout_to_stdin_offset SUM A_B rB_A RAM_B old_stdin A_rB
main:
  RAM_AL "$" CALL put_char
  RAM_AL " " CALL put_char

  
  CALLR $echo RAM_BL "|" SUB JMPRNZ $not_pipe

  CALLR $pipe
  JMPR $main
not_pipe:
  CALLR $exe_cmd
  JMPR $main

-- [_,_] -> [char,_]
echo:
  RAM_A input PUSHA
echo_loop:
  -- ^ inputi
  CALL get_char
  CMPA JMPRZ $quit
  PUSHA CALL put_char
  POPB RAM_AL 0x0A SUB JMPRZ $echo_end
  RAM_AL "|" SUB JMPRZ $echo_end
  B_A POPB AL_rB INCB PUSHB
  JMPR $echo_loop
echo_end:
  PUSHB
  -- ^ last_char inputi
  PEEKAR 0x04 A_B RAM_AL 0x00 AL_rB -- *inputi = 0
  POPA INCSP RET

quit:
  -- ^ input
  RAM_AL 0x00 CALL exit

-- [_,_] -> [_,_]
pipe:
  RAM_B current_process rB_A RAM_BL stdout_offset SUM PUSHA A_B RAM_A stdout A_rB -- current_process->stdout = stdout
 -- ^ &current_process->stdout
  CALL exe_cmd
  RAM_B old_stdout rB_A POPB A_rB -- current_process->stdout = old_stdout
  CALLR $echo RAM_AL "|" SUB JMPRNZ $pipe_end
  CALLR $pipe
pipe_end:
  RAM_B current_process rB_A RAM_BL stdin_offset SUM PUSHA A_B RAM_A stdout A_rB -- current_process->stdin = stdout
 -- ^ &current_process->stdin
  CALL exe_cmd
  RAM_B old_stdin rB_A POPB A_rB -- current_process->stdin = old_stdin
  RET

-- [_,_] -> [_,_]
exe_cmd:
  RAM_A input PUSHA
skip_spaces:
  -- ^ inputi
  PEEKB rB_AL RAM_BL " " SUB JMPRNZ $find_args
  POPA INCA PUSHA JMPR $skip_spaces
find_args:
  -- ^ inputi
  PEEKA RAM_BL " " CALL str_find_char
  CMPA JMPRZ $found_args
  A_B RAM_AL 0x00 AL_rB B_A -- separate command name and args
  INCA
found_args:
  PUSHA
  -- ^ args inputi

  PEEKAR 0x04 CALL solve_path
  CMPB JMPRNN $cmd_found
  -- ^ args inputi
  INCSP POPA CALL print
  RAM_A not_found_string CALL print
  RET
cmd_found:
  -- ^ args inputi
  POPB POPA CALL execute

  CMPA JMPRZ $end_exe_cmd
  CALL print_int
  RAM_AL " " CALL put_char
end_exe_cmd:
  RET


  ALIGN
stdout:
  NOP -- TODO: this is because otherwise code_analyze complains
