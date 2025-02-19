bss section

maybe check that reloc where is an even number

test flag in assembler linker etc to test encoding and others

put the exe and exe_state things in link
and obj things in assemble

merge some asm files together

# solve_path

- absolute path

# sh

- print exitcode

# Assembler

- warn macro unused
- warn label unused
- warn if extern not used

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
