import matplotlib.pyplot as plot

def u16(n):
    return n & 0xFFFF

def compute(a,c,m,x,N):
    Xs = []
    freq = {}
    i = 0
    while i < N:
        x = u16(u16(u16(a*x) + c) % m)
        Xs.append(x % 1000)
        if x in freq.keys():
            freq[x] += 1/N
        else:
            freq[x] = 1/N
        i += 1

    plot.scatter(Xs[:-1], Xs[1:])


#compute(109, 853, 2**16, 0, 5000)
#compute(25173, 13849, 2**16, 0, 5000)
#compute(75, 74, 2**16, 0, 5000)
compute(36969, 18000, 2**16, 0, 5000)
plot.show()
