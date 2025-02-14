bss section

remove all symbols in the final exe without debug_info

test flag in assembler linker etc to test encoding and others

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
