# Jaris

Jaris is a 16bit computer

## Table of contents

1. [Architecture](#architecture)
2. [Assembler](#assembler)
3. [File Specifications](#file-specifications)
   - [EXE](#exe-file)
   - [OBJ](#obj-file)
   - [BIN](#bin-file)

- [SO](#so-file)

4. [File System](#file-system)
   - [Dir Sector](#directory-sector)
   - [File Sector](#file-sector)
5. [RAM Layout](#ram-layout)
   - [OS struct](#os-struct)
   - [Process struct](#process-struct)
   - [FILE struct](#file-struct)

## Architecture

IMAGE

- A: 16 bit register which can be load 16 bit number or the low 8 bit of the bus in its high 8 bit.
- B: 16 bit general purpose register.
- IP: instruction pointer.
- X and Y: write only registers, used by the ALU.
- ALU: arithmetic & logic unit that can perform addition, subtraction and shift right operations.
- FR: flag register
- SC: step counter
- IR: instruction register
- SP: stack pointer
- MAR: memory address register

### Micro Instruction

- IPi: IP input
- IPo: IP output
- IPp: IP plus, increment the IP
- Ai: A input, 16 bit input
- AHi: A high input, 8 bit input on the high half
- Ao
- MARi
- Bi
- Bo
- RAM: enables the RAM (example: `RAM | RAMo` to output 8bit, `RAM | RAM16` to input 16bit)
- RAMo: sets the RAM to output, otherwise is on input
- RAM16: sets the RAM to 16bit, otherwise is on 8bit
- Xi
- Yi
- ALUo: ALU output (example: `ALUo` to sum, `ALU | _SUB` to sub, `ALU | _SHR` to `>>`)
- \_SUB: perform subtraction, otherwise addition
- \_SHR: perform right shift
- Ci: carry input to 1
- SPi
- SPo
- SPu: SP update, increase or decrease the SP (based on SPm)
- SPm: SP minus, if set decreases SP on SPu, otherwise increases
- IRi
- SCr: SC reset
- SECi
- SECo
- NDXi
- NDXo
- MEMi
- MEMo

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
|          | dynamic linking table         |

The code size is always an even number
The ending `Dynamic Linking Table Entry` is 0x00.

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
| 2        | code size                     |
|          | code                          |
| 2        | number of reloc table entries |
| 4\*#     | reloc table                   |
| 1        | globals table size            |
|          | globals table                 |

### Debug info

| size [B] | description |
| :------- | :---------- |

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

#### Dynamic Linking Table Entry

If the file is the std-lib the file name is 0x01 0x00.

| size [B] | description                 |
| :------- | :-------------------------- |
|          | file name (null terminated) |
| 2        | number of reloc enries      |
|          | reloc entries               |

The reloc `where` is relative to the `exe` code the reloc `what` is relative to the `so` code.

## File System

The NV memory is divided in sectors of size 256 bytes.
The sector 0 is reserved for the bootloader.
The other sectors can be a directory or file
The sector 1 is the root directory.

### Directory sector

The `next dir sector` is 0xFFFF if the dir fit in the sector.
The `parent dir sector` is 0xFFFF if the dir is the root.
The `head dir sector` is 0xFFFF if the dir is not the `next dir sector` of anyother sector.

| size [B] | description       |
| :------- | :---------------- |
| 1        | 'D'               |
| 2        | next dir sector   |
| 3        | '..\0'            |
| 2        | parent dir sector |
| 2        | '.\0'             |
| 2        | head dir sector   |
|          | other dir entries |

The last entry must have empty name.

#### Dir Entry

| size [B] | description                  |
| :------- | :--------------------------- |
|          | entry name (null terminated) |
| 2        | sector of the entry          |

### File Sector

The `next file sector` is 0xFFFF if the file fit in the sector.
The `max index` is 0xFF if the file doesn't fit in the sector (or if it size is 256 - 4).

| size [B] | description      |
| :------- | :--------------- |
| 1        | 'F'              |
| 2        | next file sector |
| 1        | max index        |
|          | data             |

## RAM Layout

The RAM is divided in 32 pages of size 2048

| range      | description   | size     |
| :--------- | :------------ | :------- |
|            | pages         |          |
| F800..F820 | os struct     | 32 B     |
| F820..F920 | process table | 16\*16 B |

The first process struct is the os-process

### OS Struct

| size [B] | description                   |
| :------- | :---------------------------- |
| 2        | ptr to stdlib                 |
| 2        | ptr to current process struct |
| 2        | used process map              |
| 4        | used page map                 |
| 2        | stdout struct ptr             |

### Process Struct

| size [B] | description           |
| :------- | :-------------------- |
| 2        | ptr to parent process |
| 2        | cwd sec               |
| 2        | SP                    |

### FILE struct

| size [B] | description |
| :------- | :---------- |
| 2        | sec of file |
| 1        | ndx         |
| 1        | max ndx     |

### Stdout struct

| size [B] | description   |
| :------- | :------------ |
| 2        | next char ptr |
| 2        | end ptr       |
|          | chars         |
