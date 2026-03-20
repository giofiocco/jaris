1. test pipe
1. globals etc as dynamic arrays
1. remove disassemble and maybe use the code_analyzer one
1. maybe remove obj_dump from assemble so if one wants assemble.a donesnt need to compiel with files
   removing the -d obj flag ?

bss section?
data section?

maybe check that when copying a label ecc it will always be null terminated
fix all the count + 1 < MAX ... etc

allow label alone or hex2 as rel with @allow thing
labels with dot as sublabels?

remove some micro flags for hardware

# Assembler

- warn macro unused
- warn label unused
- warn if extern not used

# bootloader

- add file checking etc

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

- realloc

# pipe

- what if first program fails
