from ortools.linear_solver import pywraplp


def main():
    for N in range(1, 100):
        solver = pywraplp.Solver('solver', pywraplp.Solver.CBC_MIXED_INTEGER_PROGRAMMING)

        vc = [[solver.IntVar(0, 1, '%s%d' % (color, i + 1)) for color in 'rbg'] for i in range(N)]

        squares = set(i**2 for i in range(1, N))

        for v in vc:
            solver.Add(v[0] + v[1] + v[2] == 1)

        for i in range(N):
            for j in range(i + 1, N):
                if j - i in squares:
                    for k in range(3):
                        solver.Add(vc[i][k] + vc[j][k] <= 1)

        obj = solver.Objective()
        for v in vc:
            for x in v:
                obj.SetCoefficient(x, 1)
        obj.SetMaximization()

        status = solver.Solve()
        if status == solver.INFEASIBLE:
            print("impossible for %d" % (N, ))
            return
        assert status == solver.OPTIMAL

        assert solver.Objective().Value() == N
        colors = {c: [] for c in 'rgb'}
        for v in vc:
            for x in v:
                if x.solution_value() > 0:
                    colors[x.name()[0]].append(int(x.name()[1:]))
        print("colors for %d: %s " % (N, colors, ))


if __name__ == "__main__":
    main()
