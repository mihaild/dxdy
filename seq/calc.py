import gmpy2
import sys
from collections import namedtuple
import tqdm


TMember = namedtuple("TMember", "index is_prime")


def main():
    add = int(sys.argv[1])
    cur = 2
    sequence = {}
    p = tqdm.tqdm(total=None)
    max_len = 0
    primes = 0
    while cur not in sequence:
        is_prime = gmpy2.is_prime(cur)
        l = len(str(cur))
        max_len = max(max_len, l)
        p.set_description(f"len {l}, max_len {max_len}, primes {primes}")
        p.update(1)
        if is_prime:
            sequence[cur] = TMember(len(sequence), True)
            cur = cur**2 + add
            primes += 1
        else:
            sequence[cur] = TMember(len(sequence), False)
            cur //= 2
    p.close()
    f = open(f'logs/{add:d}', 'w')
    sorted_seq = sorted(sequence.items(), key=lambda k: k[1].index)
    for k, (i, p) in sorted_seq:
        if p:
            f.write(f'{k} {i}\n')
    f.write('{cur}\n')

    stats = open('stats.tex', 'a')
    stats.write(f'    {add} & {cur} & {sequence[cur].index} & {len(sequence) - sequence[cur].index} & {max_len} \\\\\n')
    stats.write('    \\hline\n')
    stats.close()

    readme = open('README.md', 'a')
    readme.write(f'{add} | {cur} | {sequence[cur].index} | {len(sequence) - sequence[cur].index} | {max_len}\n')
    readme.close()


if __name__ == "__main__":
    main()
