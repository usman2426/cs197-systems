#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/****** GLOBAL DATA STRUCTURES ******/

// Any sequence of events you want to cross-check across the basic & optimized
// solvers can be logged by calling xprintf. It will only log if the LOG_XCHECK
// flag is set by main(), i.e., if the user passes an argument.
int LOG_XCHECK = 0;
#define xprintf(...) { if (LOG_XCHECK) { fprintf(stderr, __VA_ARGS__); } }

// The number of variables and clauses. This is specified by the "p cnf ..."
// line of the input.
unsigned N_VARS = 0, N_CLAUSES = 0;

// Each clause is a list of literals, along with a count of the number of
// literals set to zero in the current partial assignment. The clause is
// implied once n_zeros == (n_lits - 1) and falsified if n_zeros == n_lits.
struct clause {
    int *literals, n_literals;
    int n_zeros;
};
struct clause *CLAUSES = NULL;

// The current partial assignment is a simple map from var_id -> assignment.
enum assignment {
    UNASSIGNED  = -1,
    FALSE       = 0,
    TRUE        = 1,
};
enum assignment *ASSIGNMENT = NULL;

// We maintain a decision stack that logs every time we assign a variable &
// why. In the Chaff paper a distinction is made between "decisions" and
// "assignments", the latter being assignments as a result of BCP. For us, the
// only difference will be the decision_type.
enum decision_type {
    IMPLIED         = 0,
    TRIED_ONE_WAY   = 1,
    TRIED_BOTH_WAYS = 2,
};
struct decision {
    unsigned var;
    enum decision_type type;
};
struct decision *DECISION_STACK = NULL;
unsigned N_DECISION_STACK = 0;

// We maintain a list of literal -> clauses having that literal. For the basic
// implementation, these lists don't need to be changed after initialization so
// we'll just use a simple heap array.
struct clause_list {
    struct clause **clauses;
    int n_clauses;
};
struct clause_list *LIT_TO_CLAUSES = NULL;

/****** HELPER METHODS ******/

// LIT_TO_CLAUSES maps literals -> ids. This converts a literal to an index
// into that map. Basically, the ordering goes -1, 1, -2, 2, ...
int literal_to_id(int literal) {
    return (2 * abs(literal)) + (literal > 0);
}

// Uses literal_to_id to index into LIT_TO_CLAUSES
struct clause_list *clauses_touching(int literal) {
    return LIT_TO_CLAUSES + literal_to_id(literal);
}

// Absolute value, used for turning a literal into a variable id.
int abs(int x) { return (x < 0) ? -x : x; }

int is_literal_true(int lit) {
    int var = abs(lit);
    return ASSIGNMENT[var] != UNASSIGNED && ASSIGNMENT[var] == (lit > 0);
}

// Called when a solution is found
int satisfiable() { printf("SAT\n"); }

// Called when it is proved that no solution exists
int unsatisfiable() { printf("UNSAT\n"); }

/****** KEY OPERATIONS ******/

// Attempt to assign the given literal. Then update all the n_zeros counters.
// If this assignment causes a conflict (i.e., for some clause n_zeros ==
// n_literals), this method will return 0. Otherwise it will return 1.
int set_literal(int literal, enum decision_type type) {
    int var = abs(literal);
    assert(ASSIGNMENT[var] == UNASSIGNED);
    // Update the main assignment vector
    ASSIGNMENT[var] = literal > 0;
    // And add a new node on the decision stack.
    DECISION_STACK[N_DECISION_STACK].var = var;
    DECISION_STACK[N_DECISION_STACK++].type = type;

    // Update clause counters, check if any is completely false
    int conflict_found = 0;
    struct clause_list *list = clauses_touching(-literal);
    for (int i = 0; i < list->n_clauses; i++) {
        list->clauses[i]->n_zeros++;
        conflict_found |= (list->clauses[i]->n_zeros == list->clauses[i]->n_literals);
    }
    return !conflict_found; // 1 -> good
}

// Undo the latest assignment on the decision stack. Then update all the
// n_zeros counters. Note that undoing an assignment can never cause a new
// conflict, so we don't need to report anything.
void unset_latest_assignment() {
    // Pop a node off the decision stack
    unsigned var = DECISION_STACK[--N_DECISION_STACK].var;
    int literal = ASSIGNMENT[var] ? var : -var;

    // Update the partial assignment
    ASSIGNMENT[var] = UNASSIGNED;

    // Update n_zeros
    struct clause_list *list = clauses_touching(-literal);
    for (int i = 0; i < list->n_clauses; i++)
        list->clauses[i]->n_zeros--;
}

/****** DP METHODS ******/

// From the paper: "The operation of decide() is to select a variable that is
// not currently assigned, and give it a value. This variable assignment is
// referred to as a decision. As each new decision is made, a record of that
// decision is pushed onto the decision stack.  This function will return false
// if no unassigned variables remain and true otherwise."
int decide() {
    // Look for a variable that has not yet been assigned
    int v = 1; // "var 0 doesn't really exist"
    for (; v < N_VARS && ASSIGNMENT[v] != UNASSIGNED; v++);

    // If none is found, return false
    if (v == N_VARS)
        return 0;

    // Otherwise, try setting it false. Note this should never cause a
    // conflict, otherwise it should have been BCP'd.
    assert(set_literal(-v, TRIED_ONE_WAY));

    // Log this decision for xcheck.
    xprintf("Decide: %d\n", -v);
    return 1;
}

// From the paper: "The operation of bcp() ... is to identify any variable
// assignments required by the current variable state to satisfy f. ... if a
// clause consists of only literals with value 0 and one unassigned literal,
// then that unassigned literal must take on a value of 1 to make f sat.  ...
// In the pseudo-code from above, bcp() carries out BCP transitively until
// either there are no more implications (in which case it returns true) or a
// conflict is produced (in which case it returns false)."
int bcp() {
    int any_change = 0;
    for (size_t i = 0; i < N_CLAUSES; i++) {
        struct clause *clause = CLAUSES + i;
        if ((clause->n_zeros + 1) != clause->n_literals) continue;

        // Look for an unassigned literal in the clause.
        for (size_t l = 0; l < clause->n_literals; l++) {
            if (is_literal_true(clause->literals[l])) break;
            if (is_literal_true(-clause->literals[l])) continue;

            if (!set_literal(clause->literals[l], IMPLIED))
                return 0; // conflict!

            any_change = 1;
            break;
        }
    }

    return any_change ? bcp() : 1;
}

// From the paper: "... to deal with a conflict, we can just undo all those
// implications, flip the value of the decision assignment, and allow BCP to
// then proceed as normal. If both values have already been tried for this
// decision, then we backtrack through the decision stack until we encounter a
// decision that has not been tried both ways, and proceed from there in the
// manner described above. ... If no decision can be found which has not been
// tried both ways, that indicates that f is not satisfiable."
int resolveConflict() {
    // Unwind the decision stack until we find a decision that is only
    // TRIED_ONE_WAY.
    while (N_DECISION_STACK
            && DECISION_STACK[N_DECISION_STACK - 1].type != TRIED_ONE_WAY)
        unset_latest_assignment();

    if (!N_DECISION_STACK) // no such decision found
        return 0;

    unsigned var = DECISION_STACK[N_DECISION_STACK - 1].var;

    int new_value = !ASSIGNMENT[var];
    unset_latest_assignment();
    set_literal(new_value ? var : -var, TRIED_BOTH_WAYS);

    return 1;
}

int main(int argc, char **argv) {
    LOG_XCHECK = argc > 1;
    // Read comment lines at the start.
    for (char c; (c = getc(stdin)) == 'c';)
        while (getc(stdin) != '\n');

    assert(scanf(" cnf %u %u\n", &N_VARS, &N_CLAUSES) == 2);
    N_VARS++;

    ASSIGNMENT = malloc(N_VARS * sizeof(ASSIGNMENT[0]));
    memset(ASSIGNMENT, -1, N_VARS * sizeof(ASSIGNMENT[0]));

    DECISION_STACK = calloc(N_VARS, sizeof(DECISION_STACK[0]));

    CLAUSES = calloc(N_CLAUSES, sizeof(struct clause));

    LIT_TO_CLAUSES = calloc(N_VARS * 2, sizeof(LIT_TO_CLAUSES[0]));

    for (size_t i = 0; i < N_CLAUSES; i++) {
        int literal = 0;
        for (assert(scanf("%d ", &literal)); literal; assert(scanf("%d ", &literal))) {
            int repeat = 0;
            for (size_t j = 0; j < CLAUSES[i].n_literals && !repeat; j++) {
                repeat = (CLAUSES[i].literals[j] == literal);
            }
            if (repeat) continue;
            // Append to the clause's literal list.
#define append(obj, field) \
            (obj).n_##field++; \
            (obj).field = realloc((obj).field, (obj).n_##field * sizeof((obj).field[0])); \
            (obj).field[(obj).n_##field - 1]
            append(CLAUSES[i], literals) = literal;

            // Append to the list of clauses touching this literal.
            struct clause_list *list = clauses_touching(literal);
            append(*list, clauses) = CLAUSES + i;
        }
    }

    // Basic DP from the Chaff paper:
    if (!bcp()) return unsatisfiable(); // needed to handle unit clauses
    while (1) {
        if (!decide())
            return satisfiable();

        while (!bcp())
            if (!resolveConflict())
                return unsatisfiable();
    }
}
