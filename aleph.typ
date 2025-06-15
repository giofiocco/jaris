#import "@preview/cetz:0.3.4" as cetz: canvas, draw
#import "@preview/circuiteria:0.2.0" as circuiteria: circuit
#import "pictypst/PIC.typ": pic

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
#alephzero is the project for a 16bit TTL computer designed to be simple hardware-wise but yet complex enought to be used to explore and implement many features of modern computers.

/ Meaning of the name: $aleph_0$ in math is the cardinality of the set of natural numbers, the smallest infinity, similarly this project aims to be the smallest computer capable of infinite computations, or at least a good compromise of power and simplicity.

The core design is heavly inspired by #link("https://eater.net/8bit")[Ben Eater's 8bit computer], and the successive redesign by #link("https://www.youtube.com/@weirdboyjim")[James Sharman] and #link("https://www.youtube.com/@slu467")[Slu4] (especially for the graphics).

#text(blue)[
It consist of one 16bit bus, 2 general purpose registers, a non volatile memory, keyboard input, VGA graphics, #text(red)[(SPI, parallel thing?)].

TODO: control, flags, alu
]

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
#pic("

")

