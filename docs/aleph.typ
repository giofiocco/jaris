#import "@preview/cetz:0.3.4" as cetz: canvas, draw
#import "@preview/circuiteria:0.2.0" as circuiteria: circuit
#import "@preview/lovelace:0.3.0": pseudocode-list

#set par(justify: true)
#set text(10pt)
#set page(numbering: "1", columns:2, margin:1cm)
#show heading: smallcaps
#let alephzero = smallcaps([Aleph Zero])
#show link: underline

#place(
  top + center,
  scope: "parent",
  float: true,
  clearance: 4em,
  text(35pt)[*$aleph_0$ #alephzero*]
)
#outline()

= Introduction
This is the design for a 16bit TTL computer (#alephzero) designed to be simple hardware-wise but yet complex enought to be used to explore and implement many features of modern computers.

/ The meaning of the name: $aleph_0$ in math is the cardinality of the set of natural numbers, the smallest infinity, similarly this project aims to be the smallest computer capable of infinite computations, or at least a good compromise of power and simplicity.

The core design is heavly inspired by #link("https://eater.net/8bit")[Ben Eater's 8bit computer], and the successive redesign by #link("https://www.youtube.com/@weirdboyjim")[James Sharman] and #link("https://www.youtube.com/@slu467")[Slu4] (especially for the graphics).

= Modules
#figure(image("aleph_images/modules_diagram.png", height:20%), caption:[Modules diagram])

== RAM Module
#figure(image("aleph_images/ram_module_diagram.png", height:20%), caption:[RAM module diagram])

The MAR (memory adress register) is a 16 bits register which holds the adress the RAM points,
its output is always enabled and connected (bits 0 - 14 from MSb) to RAMs adresses.

The RAM can be used reading/writing 8 bits/16 bits, when reading 16 bits both the SRAM are active, the switch is set so that RAM 0 output to the low bus and RAM 1 the high bus, because of that the 15th address' bit (LSb) is ignored and the address must be 2 bytes alligned (word alligned), the same appends for writing 16 bits.
When reading 8 bits the SRAM enabled depends on the address' LSb and the RAM 1 switch is set so that the output is connected at the low bus.

== Booting Circuit
#figure(image("aleph_images/booting_circuit_diagram.png", height:20%), caption:[Booting circuit diagram])

It loads the first sector (first 256 bytes) from the Non-volatile memory (the bootloader code) in the RAM (from `0xFF00`) and runs it.
It executes a serie of instructions:
#pseudocode-list(hooks:.5em)[
+ `0xFF00` out | `IPi`
+ zero out | `SECi`
+ loop 256 times:
  + `IPo` | `MARi` | `NDXi` (only the low byte will go in `NDX`)
  + `MEMo` | `Ai`
  + `Ao` | `RAM` | `IPp`
]

== Keyboard
#link("https://wiki.osdev.org/PS/2_Keyboard")[SP/2 Keyboard], command `0xF0` with sub command `1` to set the scan code set 1,
if response is `0xFE` means resend and `0xFA` means ACK (possibily limited to 3 retries before assume the command is not supported).

== Control Flags

== File System
The non-volatile memory is subdivided in 256 bytes wide sectors, the first one is reserved for bootloader, the second one is for the root directory.
The memory is accessible through a sector register and an index register.
Sectors can be eighter a directory one or a file one.

/ Directory sector:
#figure(table(
  columns:3,
  stroke:.5pt,
  [*offset [`B`]*], [*size [`B`]*], [*description*],
  [`00`], [1], [`"D"`],
  [`01`], [2], [next dir sector],
  [`03`], [3], [`"..\0"`],
  [`06`], [2], [parent dir sector],
  [`08`], [2], [`".\0"`],
  [`0A`], [2], [head dir sector],
  [`0C`], [], [other entries],
))

Entries is composed by the entry name (null terminated) followed by 2 bytes of the entry sector.
The last entry is just a byte `0`, an empty name.
If entries exceed the sector size 

/ File sector:
#figure(table(
  columns:3,
  stroke:.5pt,
  [*offset [`B`]*], [*size [`B`]*], [*description*],
  [`00`], [1], [`"F"`],
  [`01`], [2], [next file sector],
  [`03`], [1], [max index],
  [`04`], [], [data],
))

== Standard Library

= Components

#figure(table(
  columns:2,
  stroke:.5pt,
  [*Component*], [*DigiKey Part Number*],
  [SRAM], [IS61C256AL-12JLI-TR],
  [Switch 4:2], [NLV14551BDR2G],
))

#figure(table(
  columns:3,
  stroke:.5pt,
  [*From*], [*To*], [*Time* [ns]],
  [RAM adress set], [data valid], [12],
))
