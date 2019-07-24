from ortools.linear_solver import pywraplp
import sys


def main():
    N = int(sys.argv[1])

    primes = []
    for i in range(3, N**2 + 10, 2):
        if all(i % p for p in primes):
            primes.append(i)
        if len(primes) == N:
            break

    primes_set = set(primes)
    sequences = []
    coverage = {p: [] for p in primes}
    print("Total primes: %d, max prime %d" % (len(primes), primes[-1], ))

    for i, p in enumerate(primes):
        for j in range(i + 1, len(primes)):
            p2 = primes[j]
            d = p2 - p
            if p2 + d in primes_set and p - d not in primes_set:
                seq = [p, p2]
                while seq[-1] + d in primes_set:
                    seq.append(seq[-1] + d)
                for x in seq:
                    coverage[x].append(len(sequences))
                sequences.append(seq)

    solver = pywraplp.Solver('solver', pywraplp.Solver.CBC_MIXED_INTEGER_PROGRAMMING)
    var_s = [solver.IntVar(0, 1, 's' + str(i)) for i in range(len(sequences))]
    var_p = []

    for p in primes:
        v = solver.IntVar(0, 1, 'p' + str(p))
        var_p.append(v)
        solver.Add(v <= sum(var_s[x] for x in coverage[p]))

    obj = solver.Objective()
    for v in var_s:
        obj.SetCoefficient(v, 2)
    for v in var_p:
        obj.SetCoefficient(v, -1)
    obj.SetMinimization()

    status = solver.Solve()
    assert status == solver.OPTIMAL

    print("seqs:")
    use_seqs = 0
    for i, v in enumerate(var_s):
        if v.solution_value() > 0:
            print(sequences[i])
            use_seqs += 1
    print("rest:", [primes[i] for i in range(len(primes)) if var_p[i].solution_value() == 0])
    use_primes = len(primes) - sum(v.solution_value() for v in var_p)

    print("total: %d" % (use_seqs + (use_primes + 1) // 2, ))


if __name__ == "__main__":
    main()
