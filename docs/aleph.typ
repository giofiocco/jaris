#import "@preview/cetz:0.3.4" as cetz: canvas, draw
#import "@preview/circuiteria:0.2.0" as circuiteria: circuit

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


/*
#place(bottom+center, scope:"parent", float:true, figure(pic("
IP: box \"IP\"; move
A: box \"A\"; move
B: box \"B\"; move
SP: box \"SP\"; move
X: box \"X\"; move;
ALU: box \"+/-/>>\"; move
Y: box \"Y\"; move
IR: box \"IR\"; move

move to ALU; move up; FR: box \"FR\"
move to Y; move up; SC: box \"SC\"
move; CROM: box \"CROM\"
move to IP; move up; move; CLK: box \"CLK\"
move to A; move up; move; box dashed \"BOOT\"
"), caption:[Modules' diagram]))
*/

/*
#place(bottom+center, scope:"parent", float:true, figure(canvas({
  import draw: *
  stroke(.5pt)

  let w = 2
  let h = 1
  let mark = (end:">", fill:black, scale:.75)

  let box(x,y,t) = {
    rect((x,y), (rel:(w,h)))
    content((x + w/2,y + h/2), t)
  }

  box(0,0,"IP"); line((2.5,0.5),(rel:(-0.5,0)), mark:mark)
  box(0,-1.5,"RAM"); line((2.5,0.5),(rel:(0.5,0)), mark:mark)

  box(3,0,"A")
  box(3,-1.5,"B")

  line((2.5,1), (rel:(0,-5)))

}), caption: [Modules diagram]))
*/

= Modules

== RAM Module

== Keyboard
#link("https://wiki.osdev.org/PS/2_Keyboard")[SP/2 Keyboard], command `0xF0` with sub command `1` to set the scan code set 1,
if response is `0xFE` means resend and `0xFA` means ACK (possibily limited to 3 retries before assume the command is not supported).

== Control Flags

== File System
The non-volatile memory is subdivided in 256 bytes wide sectors, the first one is reserved for bootloader, the second one is for the root directory.
The memory is accessible through a sector register and an index register.
Sectors can be eighter a directory one or a file one.

/ Directory sector:
#table(
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
)

Entries is composed by the entry name (null terminated) followed by 2 bytes of the entry sector.
The last entry is just a byte `0`, an empty name.
If entries exceed the sector size 

/ File sector:
#table(
  columns:3,
  stroke:.5pt,
  [*offset [`B`]*], [*size [`B`]*], [*description*],
  [`00`], [1], [`"F"`],
  [`01`], [2], [next file sector],
  [`03`], [1], [max index],
  [`04`], [], [data],
)
