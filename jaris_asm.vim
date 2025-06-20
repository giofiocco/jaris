" put this file in $VIMRUNTIME/syntax/myasm.vim
" and add in init.vim
" autocmd BufRead, BufNewFile *.myasm set filetype=myasm

if exists("b:current_syntax")
  finish
endif

set iskeyword=a-z,A-Z,_
syntax keyword myasmKeyword ALIGN EXTERN GLOBAL IMPORT db

syntax region myasmCommentLine start="--" end="$"

syntax region myasmString start=/\v"/ skip=/\v\\./ end=/\v"/

syntax keyword myasmLabel NOP INCA DECA RAM_AL RAM_BL RAM_A RAM_B INCSP DECSP PUSHA POPA PEEKA PEEKAR PUSHAR PUSHB POPB PEEKB SUM SUB SHR SHL CMPA CMPB JMP JMPR JMPRZ JMPRN JMPRC JMPRNZ JMPRNN JMPRNC JMPA A_B B_A B_AH AL_rB A_rB rB_AL rB_A A_SP SP_A A_SEC SEC_A RAM_NDX INCNDX NDX_A A_NDX MEM_A MEM_AH CALL CALLR CALLrRAM KEY RET HLT

"syntax match myasmNumber "\v(0x[\dA-F]+|\d+)"

highlight link myasmKeyword Keyword
highlight link myasmCommentLine Comment
highlight link myasmString String
highlight link myasmLabel Label
"highlight link myasmNumber Number

let b:current_syntax = "myasm"
