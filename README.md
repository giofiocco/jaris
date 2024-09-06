# Jaris

Jaris is a 16bit computer

## Table of contents

1. [Assembler](#assembler)
2. [File Specifications](#file-specifications)
   - [EXE](#exe-file)
   - [OBJ](#obj-file)
   - [BIN](#bin-file)
   - [SO](#so-file)

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

## File Specifications

### EXE file

| size [B] | description                   |
| :------- | :---------------------------- |
| 3        | 'EXE'                         |
| 2        | code size                     |
|          | code                          |
| 2        | number of reloc table entries |
| 4\*#     | reloc table                   |

### OBJ file

| size [B] | description                   |
| :------- | :---------------------------- |
| 3        | 'OBJ'                         |
| 1        | globals table size            |
|          | globals table                 |
| 1        | externs table size            |
|          | externs table                 |
| 2        | number of reloc table entries |
| 4\*#     | reloc table                   |
| 2        | code size                     |
|          | code                          |

### Bin file

| size [B] | description |
| :------- | :---------- |
|          | code        |

### SO file

| size [B] | description                   |
| :------- | :---------------------------- |
| 2        | 'SO'                          |
| 1        | globals table size            |
|          | globals table                 |
| 2        | number of reloc table entries |
| 4\*#     | reloc table                   |
| 2        | code size                     |
|          | code                          |

#### Globals Table Entry

| size [B] | description |
| :------- | :---------- |
| 1        | symbol size |
|          | symbol      |
| 2        | address     |

#### Externs Table Entry

| size [B] | description              |
| :------- | :----------------------- |
| 1        | symbol size              |
|          | symbol                   |
| 1        | number of where to subst |
| 2\*#     | where to substs          |

#### Reloc Table Entry

| size [B] | description    |
| :------- | :------------- |
| 2        | where to subst |
| 2        | what to subst  |
