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

// Each clause is a list of literals, along with two of them that are currently
// being watched. Notice there is no counter here!
struct clause {
    int *literals, n_literals;
    int watching[2];
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

// In this implementation instead of tracking literal -> clauses containing
// that literal, we only track literal -> clauses *watching* that literal. We
// need to modify this during the solve, so we'll use a linked list instead of
// an array.
struct clause_list {
    struct clause *clause;
    struct clause_list *next;
};
struct clause_list **LIT_TO_CLAUSES = NULL;

// Here we track the list of literal implications discovered when visiting
// clauses. We represent this in two ways, a direct list (BCP_LIST, N_BCP_LIST)
// of the clauses and a bool vector IS_BCP_LISTED to determine if a given
// literal is already in the list.
int *BCP_LIST = NULL;
unsigned N_BCP_LIST = 0;
char *IS_BCP_LISTED = NULL;

/****** HELPER METHODS ******/

// LIT_TO_CLAUSES maps literals -> ids. This converts a literal to an index
// into that map. Basically, the ordering goes -1, 1, -2, 2, ...
int literal_to_id(int literal) {
    return (2 * abs(literal)) + (literal > 0);
}

// Uses literal_to_id to index into LIT_TO_CLAUSES
struct clause_list **clauses_watching(int literal) {
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

// Specify the @which'th watched literal for clause CLAUSES[clause_i]. Called
// by main().
void init_watcher(int clause_i, int which, int literal) {
    CLAUSES[clause_i].watching[which] = literal;
    struct clause_list *watch = malloc(sizeof(*watch));
    struct clause_list **curr_head = clauses_watching(literal);
    watch->clause = CLAUSES + clause_i;
    watch->next = *curr_head;
    *curr_head = watch;
}

// Add a literal to the BCP list.
void queue_bcp(int literal) {
    if (IS_BCP_LISTED[literal_to_id(literal)]) return;
    IS_BCP_LISTED[literal_to_id(literal)] = 1;
    BCP_LIST[N_BCP_LIST++] = literal;
}

// Pop a literal from the BCP list.
int dequeue_bcp() {
    if (!N_BCP_LIST) return 0;
    int lit = BCP_LIST[--N_BCP_LIST];
    IS_BCP_LISTED[literal_to_id(lit)] = 0;
    return lit;
}

/****** KEY OPERATIONS ******/

// Attempt to assign the given literal. Any clauses that are watching -lit get
// visited and processed.
int set_literal(int literal, enum decision_type type) {
    int var = abs(literal);
    // Update the main assignment vector
    ASSIGNMENT[var] = literal > 0;
    // And add a new node on the decision stack.
    DECISION_STACK[N_DECISION_STACK].var = var;
    DECISION_STACK[N_DECISION_STACK++].type = type;

    // Visit any clauses that contain a literal now made false.
    for (struct clause_list **w = clauses_watching(-literal); *w;) {
        struct clause *clause = (*w)->clause;
        int watch_id = (clause->watching[0] == -literal) ? 0 : 1;
        int other_watch = clause->watching[!watch_id];
        int new_watch_lit = 0;
        // Look for an unwatched literal not set to zero in this clause.
        for (int i = 0; i < clause->n_literals; i++) {
            if (clause->literals[i] == other_watch) continue;
            if (is_literal_true(-clause->literals[i])) continue;
            // Found one!
            new_watch_lit = clause->literals[i];
            break;
        }
        if (new_watch_lit) {
            // Switch the watcher to be watching this new literal instead.
            // Note: we need to change both the clause struct itself as well as
            // move its watcher element onto the correct linked list in
            // LIT_TO_CLAUSES.
            clause->watching[watch_id] = new_watch_lit;
            struct clause_list *watcher = *w;
            *w = watcher->next; // remove it from the old list
            struct clause_list **new_head = clauses_watching(clause->watching[watch_id]);
            watcher->next = *new_head;
            *new_head = watcher;
        } else {
            // No other non-zero literal was found in the clause. This is a
            // conflict if the other watched literal is zero, else it is an
            // implication we need to BCP.
            if (is_literal_true(-other_watch)) {
                // CONFLICT
                return 0; // trigger a resolution.
            } else if (is_literal_true(other_watch)) {
                // This clause is already sat, don't have to do anything.
            } else {
                // BCP IMPLICATION, tell bcp() to set other_watch to true.
                queue_bcp(other_watch);
            }
            w = &((*w)->next);
        }
    }
    return 1;
}

// Undo the latest assignment on the decision stack. Then update all the
// n_zeros counters. Note that undoing an assignment can never cause a new
// conflict, so we don't need to report anything.
void unset_latest_assignment() {
    // Pop a node off the decision stack
    unsigned var = DECISION_STACK[--N_DECISION_STACK].var;
    // Update the partial assignment
    ASSIGNMENT[var] = UNASSIGNED;
    // No longer need to update any counters or watched literals!
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
    while (N_BCP_LIST) {
        int lit = dequeue_bcp();
        if (!set_literal(lit, IMPLIED))
            return 0;
    }
    return 1;
}

// From the paper: "... to deal with a conflict, we can just undo all those
// implications, flip the value of the decision assignment, and allow BCP to
// then proceed as normal. If both values have already been tried for this
// decision, then we backtrack through the decision stack until we encounter a
// decision that has not been tried both ways, and proceed from there in the
// manner described above. ... If no decision can be found which has not been
// tried both ways, that indicates that f is not satisfiable."
int resolveConflict() {
    // Clear the pending BCP list
    while (N_BCP_LIST)
        dequeue_bcp();

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

    BCP_LIST = calloc(2 * N_VARS, sizeof(BCP_LIST[0]));
    IS_BCP_LISTED = calloc(2 * N_VARS, sizeof(IS_BCP_LISTED[0]));

    for (size_t i = 0; i < N_CLAUSES; i++) {
        int literal = 0;
        for (assert(scanf("%d ", &literal)); literal; assert(scanf("%d ", &literal))) {
            // Append to the clause's literal list.
#define append(obj, field) \
            (obj).n_##field++; \
            (obj).field = realloc((obj).field, (obj).n_##field * sizeof((obj).field[0])); \
            (obj).field[(obj).n_##field - 1]
            append(CLAUSES[i], literals) = literal;

            // TODO: We really need to dedup the literal list. TODO: evaluate
            // how bad an n^2 thing would hurt us.
        }
    }

    for (size_t i = 0; i < N_CLAUSES; i++) {
        if (!CLAUSES[i].n_literals) return unsatisfiable();
        if (CLAUSES[i].n_literals == 1) {
            // If a clause has only one literal, it must be set.
            queue_bcp(CLAUSES[i].literals[0]);
        } else {
            // Watch the first 2 literals.
            init_watcher(i, 0, CLAUSES[i].literals[0]);
            init_watcher(i, 1, CLAUSES[i].literals[1]);
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
