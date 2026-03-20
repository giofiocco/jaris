counter_load_to_output = 220
buffer_enable_output = 20
counter_clock_to_up = 30
reg_in_to_out = 10
buffer_enable = 10
ram_in = 20
ram_out = 20
mux_enable = 10

HLT = 0
IPi = counter_load_to_output
IPo = buffer_enable_output
IPp = counter_clock_to_up
Ai = buffer_enable
AHi = buffer_enable
Ao = buffer_enable
MARi = reg_in_to_out
Bi = buffer_enable
Bo = buffer_enable
RAMi = max(ram_in, mux_enable + ram_in)
RAMo = max(ram_out, mux_enable + ram_out)
RAM16i = ram_in
RAM16o = ram_out
Xi = reg_in_to_out
Yi = reg_in_to_out
Add = 0
Sub = 0
Shr = 0
And = 0
Ci = 0
SPi = 0
SPo = 0
SPp = 0
SPm = 0
IRi = 0
SCr = 0
SECi = 0
SECo = 0
NDXi = 0
NDXo = 0
MEMi = 0
MEMo = 0
KEYo = 0
GPUAi = 0
GPUi = 0

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
    print(inst, time)

max_time = sorted([time for inst,time in instructions.items()])[-1]

print([inst for inst,time in instructions.items() if time == max_time])
print("max_time: {} ns -> {:.3f} MHz".format(max_time, 1e-6*1e9/max_time/2))
