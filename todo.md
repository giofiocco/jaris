bss section

!! BOOTLOADER and EXECUTE new EXE format

maybe check that reloc where is an even number

exe symbols are one for the extern and one for the globals -> merge them

remove all symbols in the final exe without debug_info

test flag in assembler linker etc to test encoding and others

put the exe and exe_state things in link
and obj things in assemble

merge some asm files together

# execute

- absolute path

# sh

- print exitcode

# Linker

- option to select the stdlib location

# Sim

- step mode add commands
  - `read 0010` to read from ram
  - `next` to run till the next `HLT`

# encodemem

- pass bootloader stdlib os as args

# bootloader

- add file checking etc

# Inspect

- disassemble using debug info
- read from the stdin

# Assemble

- files obj functions in assemble.c
- bytecode BLABEL
- error when trying:

```
argv: arg 0x0000
arg: "asdf" 0x00
```
