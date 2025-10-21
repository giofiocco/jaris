#import "@preview/cetz:0.3.4" as cetz: canvas, draw
#import "@preview/circuiteria:0.2.0" as circuiteria: circuit
#import "@preview/lovelace:0.3.0": pseudocode-list
#import "@preview/oxifmt:1.0.0": strfmt
#import "@preview/rivet:0.3.0": schema, config

#set par(justify: true)
#set text(10pt)
#set page(numbering: "1", columns: 2, margin: 1cm)
#show heading: smallcaps
#let alephzero = smallcaps([Aleph Zero])
#show link: underline

#let sizes(..body, caption: []) = {
  let acc = 0
  figure(table(
    columns: 3,
    stroke: .5pt,
    align: left,
    [*offset*], [*size [`B`]*], [*description*],
    ..for row in body.pos().chunks(2) {
      (raw(strfmt("0x{:02X}", acc)), if row.at(0) == 0 { [] } else { [#row.at(0)] }, row.at(1))
      acc += row.at(0)
    }
  ), caption: caption)
}

#let register(s, cap) = figure(schema.render(schema.load(s), config:config.config(default-font-family:"Nimbus Sans")), caption:cap)

#place(top + center, scope: "parent", float: true, clearance: 4em, text(35pt)[*$aleph_0$ #alephzero*])
#outline()

#text(red)[
  - *TODO*: if a program executed uses more than one page how would the exit know how to free all the pages?
  - *TODO*: exit maybe uses SP to compute the page mask? does not work for multiple pages
]

= Introduction
This is the design for a 16bit TTL computer (#alephzero) designed to be simple hardware-wise but yet complex enought to be used to explore and implement many features of modern computers.

/ Reason for the name: $aleph_0$ in math is the cardinality of the set of natural numbers, the smallest infinity, similarly this project aims to be the smallest computer capable of infinite computations, or at least a good compromise of power and simplicity.

The core design is heavly inspired by #link("https://eater.net/8bit")[Ben Eater's 8bit computer], and the successive redesign by #link("https://www.youtube.com/@weirdboyjim")[James Sharman] and #link("https://www.youtube.com/@slu467")[Slu4] (especially for the graphics).

Each module is controlled by some _microcode-flags_ (1bit pins) that #text(red)[TODO]
The computer has a single 16bit data/address bus, a #text(red)[[TODO: Hz]] clock that 

= Modules
#figure(image("aleph_images/modules_diagram.png", height: 20%), caption: [Modules diagram])


== RAM Module
#figure(image("aleph_images/ram_module_diagram.png", height: 20%), caption: [RAM module diagram])

The MAR (memory adress register) is a 16 bits register which holds the adress the RAM points,
its output is always enabled and connected (bits 0 - 14 from MSb) to RAMs adresses.

The RAM can be used reading/writing 8 bits/16 bits, when reading 16 bits both the SRAM are active, the switch is set so that RAM 0 output to the low bus and RAM 1 the high bus, because of that the 15th address' bit (LSb) is ignored and the address must be 2 bytes alligned (word alligned), the same appends for writing 16 bits.
When reading 8 bits the SRAM enabled depends on the address' LSb and the RAM 1 switch is set so that the output is connected at the low bus.

== ALU Module
#figure(image("aleph_images/alu_diagram.png", height:20%), caption:[ALU module diagram])

== Booting Circuit
#figure(image("aleph_images/booting_circuit_diagram.png", height: 20%), caption: [Booting circuit diagram])

It loads the first sector (first 256 bytes) from the Non-volatile memory (the bootloader code) into the RAM (from `0xFF00`) and runs it.
It is connected directly to individual microcode-flags.
It executes a serie of instructions:

#pseudocode-list(hooks: .5em)[
  + reset the `IP`
  + `IPo` | `SECi`
  + loop 256 times:
    + `IPm`
    + `IPo` | `MARi` | `NDXi` #h(1fr) _% only the low byte will go in `NDX`_
    + `MEMo` | `RAM`
]

== Keyboard
#link("https://wiki.osdev.org/PS/2_Keyboard")[SP/2 Keyboard], command `0xF0` with sub command `1` to set the scan code set 1,
if response is `0xFE` means resend and `0xFA` means ACK (possibily limited to 3 retries before assume the command is not supported).

== GPU
#figure(image("aleph_images/gpu.png"), caption: [GPU diagram])
The GPU produce a VGA bicolor signal at #text(red)[resolution], with a separate #text(red)[TODO] clock (#link("http://www.tinyvga.com/vga-timing")[VGA timing]).
The screen is divided into 8x8 pixel square, each one filled with a so called pattern stored in the PATTERN RAM, while the ATTRIBUTE RAM stores which pattern to draw in a specific square.
Each pattern is stored in 8 bytes (in rows) it is loaded on a shift-register and bit by bit converted to VGA signal.

The fastest clock for a reasonable resolution is 35.5 MHz so the reading circuit will need to be faster than 28 ns \* 8 = 224 ns because the ram is read every 8 clock cycle.



The coordinates of the pixel to draw will be refered as x and y (xl the least significant 3 bit, xh the rest of the bits, and the same for yh, yl):

#register(```yaml
structures:
  main:
    bits: 15
    ranges:
      0-4:
        name: XH
      5-9:
        name: YH
```, [ATTRIBUTE RAM address])

#register(```yaml
structures:
  main:
    bits: 15
    ranges:
      0-2:
        name: YL
      3-14:
        name: ATTRIBUTE RAM OUT
```, [PATTERN RAM address])

== Control Flags

= File System
The non-volatile memory is subdivided in 256 bytes wide sectors, the first one is reserved for bootloader, the second one is for the root directory.
The memory is accessible through a sector register and an index register.
Sectors can be eighter a directory one or a file one.

/ Directory sector:
#sizes(
  1, [`"D"`],
  2, [next dir sector],
  3, [`"..\0"`],
  2, [parent dir sector],
  2, [`".\0"`],
  2, [head dir sector],
  0, [other entries],
  caption: [Directory sector]
)

Entries is composed by the entry name (null terminated) followed by 2 bytes of the entry sector.
The last entry is just a byte `0`, an empty name.
If entries exceed the sector size

/ File sector:
#sizes(
  1, [`"F"`],
  2, [next file sector],
  1, [max index],
  0, [data],
  caption: [File sector]
)

= RAM Layout
The RAM is divided in 32 pages 2048B wide.
The first pages are for the os and the standard library, and the last one is reserved for all the system structs and the stack of the os.

#text(red)[
  The `execute` function will set the last word (2 bytes) of the page as a bit map of used blocks,
  subdividing the page in 16 128B-wide blocks,
  this information will be used by `alloc` and `free` functions to dynamically allocating memory.
  The os and stdlib pages will not have that, and will not be able to use `alloc` not `free`.
]

#figure(table(
  columns: 3,
  stroke: .5pt,
  align: left,
  [*range*],      [*description*], [*size*],
  [],             [pages],         [],
  [`F800..F820`], [os struct],     [32 `B`],
  [`F820..F920`], [process table], [16$times$16 `B`],
), caption: [RAM Layout])

The first process struct is the os process.

#sizes(
  2, [ptr to stdlib],
  2, [ptr to current process struct],
  2, [used process map],
  4, [used page map],
  caption: [OS struct]
)

#sizes(
  2, [ptr to parent process],
  2, [cwd sec],
  2, [SP],
  2, [stdout redirect],
  caption: [Process struct]
)

If `stdout redirect` is `0xFFFF` the `put_char` will print to the screen otherwise it will be redirected to the stdout struct pointed by `stdout redirect`.

`SP` is `0x0000` as set only when the process execute another process and used in the `exit` function to reset.

== Other structs

/ File struct:
#sizes(
  2, [sec of the file],
  1, [ndx to read or write to],
  1, [undef (used to keep alignment)],
  caption: [File struct]
)

/ Stdout struct:
#sizes(
  2, [count of remaining chars],
  2, [ptr to next char],
  0, [chars],
  caption: [Stdout struct]
)

= Standard Library

= Components

#figure(table(
  columns: 2,
  stroke: .5pt,
  [*Component*], [*DigiKey Part Number*],
  [SRAM],        [IS61C256AL-12JLI-TR],
  [Switch 4:2],  [NLV14551BDR2G],
))

#figure(table(
  columns: 3,
  stroke: .5pt,
  [*From*],         [*To*],       [*Time* [ns]],
  [RAM adress set], [data valid], [12],
))
