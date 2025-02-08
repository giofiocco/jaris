bss section

in obj and so gloabals and externs table count and not size

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
