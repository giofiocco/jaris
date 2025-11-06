1. globals etc as dynamic arrays

asm formatter

bss section?

maybe check that when copying a label ecc it will always be null terminated

allow label alone or hex2 as rel with @allow thing
labels with dot as sublabels?
conditional call?

remove some micro flags

test flag in assembler linker etc to test encoding and others

put the exe and exe_state things in link
and obj things in assemble

max index in directory sector?

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

# execute

- errors instead of crashes (but how do you diff between exitcode or error from execute?)
