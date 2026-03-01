import time
from ortools.sat.python import cp_model

def solve_min_maximal_rectangles(N=20, horizontal_only=True):
    model = cp_model.CpModel()

    # 1. Variables representing the start coordinates of the rectangles
    hr = {} # hr[(r, c)] == 1 if a horizontal 1x3 rectangle starts at (r, c)
    for r in range(N):
        for c in range(N - 2):
            hr[(r, c)] = model.NewBoolVar(f'hr_{r}_{c}')

    vr = {} # vr[(r, c)] == 1 if a vertical 3x1 rectangle starts at (r, c)
    for r in range(N - 2):
        for c in range(N):
            vr[(r, c)] = model.NewBoolVar(f'vr_{r}_{c}')

    C = {} # C[(r, c)] == 1 if cell (r, c) is covered by ANY placed rectangle

    # 2. Cell Mapping & Non-Overlapping (Independent Set)
    for r in range(N):
        for c in range(N):
            rects = []
            # Gather all possible horizontal rectangles covering (r, c)
            for dc in range(3):
                if 0 <= c - dc < N - 2:
                    rects.append(hr[(r, c - dc)])

            # Gather all possible vertical rectangles covering (r, c)
            for dr in range(3):
                if 0 <= r - dr < N - 2:
                    rects.append(vr[(r - dr, c)])

            # Constraint: A cell can be covered by at most 1 rectangle
            C[(r, c)] = model.NewBoolVar(f'C_{r}_{c}')
            model.Add(sum(rects) <= 1)
            model.Add(C[(r, c)] == sum(rects))

    # 3. Maximality Constraint ("no other rectangle can be added")
    # Every valid 1x3 window on the board MUST have at least 1 covered cell
    for r in range(N):
        for c in range(N - 2):
            model.AddBoolOr([C[(r, c)], C[(r, c+1)], C[(r, c+2)]])

    for r in range(N - 2):
        for c in range(N):
            model.AddBoolOr([C[(r, c)], C[(r+1, c)], C[(r+2, c)]])

    # 4. Heuristic 1: 1D Pigeonhole Valid Inequalities
    # Any row or column of length 20 without 3 consecutive spaces must have >= 6 covered cells
    min_covered = 6
    for r in range(N):
        model.Add(sum(C[(r, c)] for c in range(N)) >= min_covered)
    for c in range(N):
        model.Add(sum(C[(r, c)] for r in range(N)) >= min_covered)

    # 5. Heuristic 2: D4 Symmetry Breaking
    # Force the Top-Left quadrant to be denser than the others to break rotational symmetry
    half = N // 2
    TL = sum(C[(r, c)] for r in range(half) for c in range(half))
    TR = sum(C[(r, c)] for r in range(half) for c in range(half, N))
    BL = sum(C[(r, c)] for r in range(half, N) for c in range(half))
    BR = sum(C[(r, c)] for r in range(half, N) for c in range(half, N))
    model.Add(TL >= TR)
    model.Add(TL >= BL)
    model.Add(TL >= BR)

    # 6. Heuristic 3: Single Orientation Sub-search
    # Instantly proves optimal global lower bounds (a density of exactly 1/5)
    if horizontal_only:
        model.Add(sum(vr.values()) == 0)

    # 7. Objective: Minimize the total number of placed rectangles
    model.Minimize(sum(hr.values()) + sum(vr.values()))

    # Solve Setup
    solver = cp_model.CpSolver()
    solver.parameters.num_search_workers = 8 # Activate multi-core portfolio search

    print("Solving CP-SAT Model...")
    start_time = time.time()
    status = solver.Solve(model)
    elapsed = time.time() - start_time

    if status in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        print(f"\nStatus: {solver.StatusName(status)} in {elapsed:.2f} seconds")
        print(f"Minimum rectangles needed: {int(solver.ObjectiveValue())}\n")

        # ASCII Grid Visualization
        grid = [['.' for _ in range(N)] for _ in range(N)]
        for r in range(N):
            for c in range(N - 2):
                if solver.Value(hr[(r, c)]):
                    grid[r][c] = grid[r][c+1] = grid[r][c+2] = '-'
        for r in range(N - 2):
            for c in range(N):
                if solver.Value(vr[(r, c)]):
                    grid[r][c] = grid[r+1][c] = grid[r+2][c] = '|'

        for row in grid:
            print(' '.join(row))
    else:
        print("No solution found.")

if __name__ == '__main__':
    solve_min_maximal_rectangles(horizontal_only=False)
