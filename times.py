# registers with reset: https://www.mouser.it/c/semiconductors/logic-ics/flip-flops/?number%20of%20circuits=8%20Circuit
# register has clock, reset so to input needs flag and-ed with the clock and to output needs a buffer

# counter? https://www.ti.com/lit/ds/symlink/sn74ls191.pdf?ts=1774373216393

reg_clock_to_out = 90
buffer = 7
trans = 8.4
_and = 4.8
_or = 30
_not = 6
sram_access = 45
sram_addr_available = 45
adder = 14 # TODO: maybe the sum to carry out of the first has to prop on the other one and so on
eq_zero = 0

counter_in = 50
counter_out = buffer
counter_up = 24
counter_down = counter_up
counter_reset = 0

reg_in = _and + reg_clock_to_out
reg_out = buffer

ram_in = _and + _or + _not + sram_access
ram_out = ram_in

alu_out = eq_zero + reg_in

HLT    = 0
IPi    = counter_in
IPo    = counter_out
IPp    = counter_up
Ai     = reg_in
AHi    = reg_in
Ao     = reg_out
MARi   = reg_in + sram_addr_available
Bi     = reg_in
Bo     = reg_out
RAMi   = trans + ram_in
RAMo   = trans + ram_out
RAM16i = trans + ram_in
RAM16o = trans + ram_out
Xi     = reg_in
Yi     = reg_in
Add    = alu_out + buffer + adder
Sub    = alu_out + buffer + adder
Shr    = alu_out + buffer
And    = alu_out + buffer + _and
Ci     = 0
SPi    = counter_in
SPo    = counter_out
SPp    = counter_up
SPm    = counter_down
IRi    = reg_in
SCr    = counter_reset
SECi   = reg_in
SECo   = reg_out
NDXi   = reg_in
NDXo   = reg_out
MEMi   = 0
MEMo   = 0
KEYo   = 0
GPUAi  = 0
GPUi   = 0

uinst = [(eval(i),i) for i in ["HLT", "IPi", "IPo", "IPp", "Ai", "AHi", "Ao", "MARi", "Bi", "Bo", "RAMi", "RAMo", "RAM16i", "RAM16o", "Xi", "Yi", "Add", "Sub", "Shr", "And", "Ci", "SPi", "SPo", "SPp", "SPm", "IRi", "SCr", "SECi", "SECo", "NDXi", "NDXo", "MEMi", "MEMo", "KEYo", "GPUAi", "GPUi"]]
uinst.sort(reverse=True)
print(uinst)

fetch = (IPo + MARi) + max(RAMo + IRi, IPp)
jmpr = fetch + (IPo + max(MARi, Yi)) + (RAM16o + Xi) + (Add + IPi) + SCr

instructions = {
    'NOP': fetch + SCr,
    'INCA':  fetch + (Ao + Yi) + Xi + (Ci + Add + Ai) + SCr, # TODO: maybe a register with 1
    'DECA':  fetch + (Ao + Yi) + Xi + (Ci + Sub + Ai) + SCr, # TODO: maybe a register with 1
    'INCB':  fetch + (Bo + Yi) + Xi + (Ci + Add + Ai) + SCr, # TODO: maybe a register with 1
    'RAM_AL': fetch + (IPo + MARi) + max(RAMo + Ai, IPp) + SCr,
    'RAM_BL': fetch + (IPo + MARi) + max(RAMo + Bi, IPp) + SCr,
    'RAM_A': fetch + (IPo + MARi) + max(RAM16o + Ai, IPp) + IPp + SCr,
    'RAM_B': fetch + (IPo + MARi) + max(RAM16o + Bi, IPp) + IPp + SCr,
    'INCSP': fetch + SPp + SCr,
    'DECSP': fetch + SPm + SCr,
    'PUSHA': fetch + (SPo + MARi) + (Ao + RAM16i + SPm) + SCr,
    'POPA': fetch + SPp + (SPo + MARi) + (RAM16o + Ai) + SCr,
    'PEEKA': fetch + SPp + (SPo + MARi) + max(RAM16o + Ai, SPm) + SCr,
    'PEEKAR': fetch + (IPo + MARi) + max(RAMo + Yi, IPp) + (SPo + Xi) + (Add + MARi) + (RAM16o + Ai) + SCr,
    'PUSHAR': fetch + (IPo + MARi) + max(RAMo + Yi, IPp) + (SPo + Xi) + (Add + MARi) + (Ao + RAM16i) + SCr,
    'PUSHB': fetch + (SPo + MARi) + (Bo + RAM16i + SPm) + SCr,
    'POPB': fetch + SPp + (SPo + MARi) + (RAM16o + Bi) + SCr,
    'PEEKB': fetch + SPp + (SPo + MARi) + max(RAM16o + Bi, SPm) + SCr,
    'SUM': fetch + (Ao + Xi) + (Bo + Yi) + (Add + Ai) + SCr,
    'SUB': fetch + (Ao + Xi) + (Bo + Yi) + (Ci + Sub + Ai) + SCr, # TODO: shure of the Ci?
    'SHR': fetch + (Ao + Xi) + (Ao + Yi) + (Add + Ai) + SCr,
    'SHL': fetch + (Ao + Xi) + (Shr + Ai) + SCr,
    'AND': fetch + (Ao + Xi) + (Bo + Yi) + (And + Ai) + SCr,
    'CMPA': fetch + (Ao + Xi) + Yi + Add + SCr, # TODO: maybe zero register
    'CMPB': fetch + (Bo + Xi) + Yi + Add + SCr, # TODO: maybe zero register
    'JMP': fetch + (IPo + MARi) + (RAM16o + IPi) + SCr,
    'JMPR': jmpr,
    'JMPRZ': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPRN': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPRC': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPRNZ': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPRNN': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPRNC': max(jmpr, fetch + IPp + IPp + SCr),
    'JMPA': fetch + (Ao + IPi) + SCr,
    'JMPAR': fetch + (IPo + max(MARi, Yi)) + (Ao + Xi) + (Add + IPi) + SCr,
    'A_B': fetch + (Ao + Bi) + SCr,
    'B_A': fetch + (Bo + Ai) + SCr,
    'B_AH': fetch + (Bo + AHi) + SCr,
    'AL_rB': fetch + (Bo + MARi) + (Ao + RAMi) + SCr,
    'A_rB': fetch + (Bo + MARi) + (Ao + RAM16i) + SCr,
    'rB_AL': fetch + (Bo + MARi) + (RAMo + Ai) + SCr,
    'rB_A': fetch + (Bo + MARi) + (RAM16o + Ai) + SCr,
    'A_SP': fetch + (Ao + SPi) + SCr,
    'SP_A': fetch + (SPo + Ai) + SCr,
    'A_SEC': fetch + (Ao + SECi) + SCr,
    'SEC_A': fetch + (SECo + Ai) + SCr,
    'RAM_NDX': fetch + (IPo + MARi) + max(RAMo + NDXi, IPp) + SCr,
    'INCNDX': fetch + (NDXo + Yi) + Xi + (Ci + Add + NDXi) + SCr,
    'NDX_A': fetch + (NDXo + Ai) + SCr,
    'A_NDX': fetch + (Ao + NDXi) + SCr,
    'MEM_A': fetch + (MEMo + Ai) + SCr,
    'MEM_AH': fetch + (MEMo + AHi) + SCr,
    'A_MEM': fetch + (Ao + MEMi) + SCr,
    'CALL': fetch + (SPo + MARi) + max(IPo + RAM16i, SPm) + (IPo + MARi) + (RAM16o + IPi) + SCr,
    'CALLR': fetch + (SPo + MARi) + max(IPo + RAM16i, SPm) + (IPo + max(MARi,Yi)) + (RAM16o + Xi) + (Add + IPi) + SCr,
    'RET': fetch + SPp + (SPo + MARi) + (RAM16o + IPi) + IPp + IPp + SCr, # TODO: maybe when calling save the IP+2 and there remove IPp IPp
    '_KEY_A': fetch + (KEYo + Ai) + SCr,
    'DRW': fetch + (Bo + GPUAi) + (Ao + GPUi) + SCr,
    'RAM_DRW': fetch + (Bo + GPUAi) + (IPo + MARi) + max(RAMo + GPUi, IPp) + SCr,
    'HLT': fetch + HLT,
}

for inst,time in instructions.items():
    print('{} {:.3f}'.format(inst, time))

max_time = sorted([time for inst,time in instructions.items()])[-1]

print([inst for inst,time in instructions.items() if time == max_time])
print("max_time: {} ns -> {:.3f} MHz".format(max_time, 1e-6*1e9/max_time/2))
