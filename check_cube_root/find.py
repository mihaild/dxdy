from functools import reduce
from sympy.ntheory import factorint
import tqdm
from math import gcd
from functools import lru_cache


def prod(a):
    return reduce(int.__mul__, a, 1)


@lru_cache(None)
def get_bruteforce(n):
    return len(set(i**3 % n for i in range(n)))


def get(n):
    return prod(get_bruteforce(k**v) for k, v in factorint(n).items())


best = 1.0
for i in tqdm.trange(2, 10**6):
    if gcd(i, 5005) == 1:
        r = get(i) / i
        if r < best:
            best = r
            print(r, i)
