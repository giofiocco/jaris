#let file_list = read("Makefile").match(regex("STDLIB_FILES\s*=\s*([a-zA-Z0-9_ ]+)\s*\n")).captures.at(0).split(" ").map(x => "asm/" + x + ".asm")

#place(
  top+center,
  float:true,
  scope:"parent",
  text(30pt)[*Standard Library*]
)

#for file in file_list {
  let symbols = (:)
  for s in read(file).matches(regex("GLOBAL\s+([a-zA-Z0-9_]+)")).map(x => x.captures).flatten() {
    symbols.insert(s, none)
  }

  let comment = ""
  let is_collecting_comment = 0
  for line in read(file).split("\n") {
    let m = line.match(regex("^\s*--(.+)$"))
    if m != none {
      if is_collecting_comment == 0 {
        comment = ""
        is_collecting_comment = 1
      }
      comment += m.captures.at(0).trim() + "\n"
    } else {
      is_collecting_comment = 0
    }
    m = line.match(regex("\s*([a-zA-Z0-9_]+):\s*$"))
    if m != none {
      if (m.captures.at(0) in symbols) {
        symbols.at(m.captures.at(0)) = comment
        is_collecting_comment = 0
      }
    }

  }

  align(center)[*#file*]
  for (k,v) in symbols.pairs() {
    [/ #raw(k):] + raw(v)
  }
}
