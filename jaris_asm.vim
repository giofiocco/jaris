" put this file in $VIMRUNTIME/syntax/

if exists("b:current_syntax")
  finish
endif

set iskeyword=a-z,A-Z,_
syntax keyword asmKeyword ALIGN EXTERN GLOBAL IMPORT db

syntax region asmCommentLine start="--" end="$"

syntax region asmString start=/\v"/ skip=/\v\\./ end=/\v"/

syntax keyword asmLabel NOP INCA DECA INCB RAM_AL RAM_BL RAM_A RAM_B INCSP DECSP PUSHA POPA PEEKA PEEKAR PUSHAR PUSHB POPB PEEKB SUM SUB SHR SHL CMPA CMPB JMP JMPR JMPRZ JMPRN JMPRC JMPRNZ JMPRNN JMPRNC JMPA JMPAR A_B B_A B_AH AL_rB A_rB rB_AL rB_A A_SP SP_A A_SEC SEC_A RAM_NDX INCNDX NDX_A A_NDX MEM_A MEM_AH CALL CALLR RET _KEY_A DRW RAM_DRW HLT

" syntax match asmNumber "\v(0x[\dA-F]+|\d+)"

highlight link asmKeyword Keyword
highlight link asmCommentLine Comment
highlight link asmString String
highlight link asmLabel Label
" highlight link asmNumber Number

let b:current_syntax = "jaris_asm"
