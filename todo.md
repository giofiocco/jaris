bss section

remove some micro flags

emit graph

maybe check that reloc where is an even number

test flag in assembler linker etc to test encoding and others

put the exe and exe_state things in link
and obj things in assemble

# Assembler

- warn macro unused
- warn label unused
- warn if extern not used

# Sim

- test with a fixed mem.bin

- step mode add commands
  - `read 0010` to read from ram
  - `next` to run till the next `HLT`

# encodemem

- pass bootloader stdlib os as args

# bootloader

- add file checking etc

# Inspect

- font file
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

# execute

- errors instead of crashes (but how do you diff between exitcode or error from execute?)
- allocate_page not as a function
