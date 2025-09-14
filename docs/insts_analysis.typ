#set page(width:auto)

#let mis = (:)
#let braces = 1
#for (i,line) in read("../src/runtime.c").split("void set_control_rom() {").at(1).split("\n").enumerate() {
  braces += line.matches("{").len()
  braces -= line.matches("}").len()
  if braces == 0 {
    break
  }
  for reg in (regex("set_instruction_allflag\(([A-Z]+), .+, (.+)\)"), regex("set_instruction_flags\(([A-Z]+), .+, .+, .+, (.+)\)")) {
    let m = line.match(reg)
    if m != none {
      let (inst, minst) = m.captures
      minst = minst.matches(regex("micro\((.+?)\)")).map(x => x.captures.at(0))
      for mi in minst {
        for mj in minst {
          if mi != mj and not mj in mis.at(mi,default:()) {
            if mi in mis.keys() {
              mis.at(mi).push(mj)
            } else {
              mis.insert(mi, ())
            }
          }
        }
      }
    }
  }
}

#figure(table(
  stroke:.5pt,
  columns: (4em,) * (mis.keys().len() + 1),
  [], ..mis.keys(),
  ..mis.pairs().map(x => {
    let (k,v) = x
    (k, ..mis.keys().map(y => if y in v or y == k { table.cell(fill:black,[]) } else { [] }))
  }).join()
))

