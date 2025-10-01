import gmpy2
import time
import sys

k = 2
was = {}
t = time.time()
total_primes = 0
step = 0
as_prime_pos = {}

add = int(sys.argv[1])

while True:
    step += 1
    if step % 100 == 0:
        print(step, len(str(k)), len(was), total_primes, time.time() - t)
    if gmpy2.is_prime(k):
        if k in was:
            break
        was[k] = step
        as_prime_pos[k] = len(as_prime_pos)
        k = k**2 + add
        total_primes += 1
    else:
        k = k // 2

print("cycle starts with %d" % (k, ))
print("len %d, has %d primes, starts at pos %d" % (step - was[k], len(as_prime_pos) - as_prime_pos[k], was[k], ))

f = open('cycles', 'a')
f.write('%3d %5d %6d %12d %5d\n' % (add, k, step - was[k], len(as_prime_pos) - as_prime_pos[k], was[k], ))
