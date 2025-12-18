1. globals etc as dynamic arrays
1. maybe not fclose stdin in exe_decode etc
1. remove disassemble and maybe use the code analyzer one

asm formatter

bss section?
data section?

maybe check that when copying a label ecc it will always be null terminated

allow label alone or hex2 as rel with @allow thing
labels with dot as sublabels?

remove some micro flags

# Assembler

- warn macro unused
- warn label unused
- warn if extern not used

# bootloader

- add file checking etc

# Inspect

- os disassemble does not work

# Assemble

- bytecode BLABEL
- error when trying:

```
argv: arg 0x0000
arg: "asdf" 0x00
```

# Code Analyzer

- disassemble correctly data or code

# execute

- errors instead of crashes (but how do you diff between exitcode or error from execute?)

# alloc

- allocate the blocks starting from the start of the page so a realloc wouldn't need to move always
