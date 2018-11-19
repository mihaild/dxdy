import tqdm
import sys
import sympy.ntheory

PRIMES = list(sympy.ntheory.primerange(3, 1000000))


def check(n):
    for p in PRIMES:
        if (pow(n, n + 1, p) + n + 1) % p == 0:
            return False
    return sympy.ntheory.isprime(n**(n + 1) + n + 1)


def main():
    n = int(sys.argv[1])
    p = tqdm.trange(1, n + 1)
    for i in p:
        if check(i):
            p.write(str(i) + '\n')


if __name__ == "__main__":
    main()
