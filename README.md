# Jaris 

Jaris is a 16bit computer

## Assembler

### Tokens
- SYM: `[a-zA-Z_][a-zA-Z_0-9]*` 
- INST: as SYM
- HEX: `0[xX][0-9a-fA-F]{2}`
- HEX2: `0[xX][0-9a-fA-F]{4}`
- MACROO: `{`
- MACROC: `}`
- COLON: `:`
- REL: `$`
- GLOBAL: `GLOBAL`
- EXTERN: `EXTERN`
- STRING: `"[^"]*"`
- ALIGN: `ALIGN`
- DB: `db`
- INT: `[1-9][0-9]*`

### Comments
- Single line comments: `--.*\n`
