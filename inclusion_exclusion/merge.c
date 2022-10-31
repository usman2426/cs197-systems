#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

typedef struct Literal {
    unsigned long value;
    bool sign;
} Literal;

typedef struct Clause {
    Literal *literals;
    unsigned long numLiterals;    
} Clause;

Clause createClause(unsigned long *vals,  bool *signs, unsigned long numLits) {
    Clause clause = {NULL, numLits};
    clause.literals = malloc(sizeof(Literal)*numLits);
    assert(clause.literals);
    for (int i = 0; i < numLits; i++) {
        Literal newLit = {vals[i], signs[i]};
	clause.literals[i] = newLit;
    } 
    return clause;
}

void printClause(Clause clause) {
	printf("Number of Literals: %lu \n", clause.numLiterals);
	for (unsigned long i = 0; i < clause.numLiterals; i++) {
	    printf("%lu ", clause.literals[i].value);
	}
	printf("\n");
}

Clause merge(Clause clause1, Clause clause2) {
	Clause emptySet = {NULL, 0}; 
	unsigned long maxSize = clause1.numLiterals + clause2.numLiterals;
	Literal* newLiterals = malloc(sizeof(Literal) * maxSize);
	assert(newLiterals);
	unsigned long clause1idx = 0;
	unsigned long clause2idx = 0;
	unsigned long newClauseSize = 0;
	Literal* lastAdded = NULL;
	while(clause1idx < clause1.numLiterals || clause2idx < clause2.numLiterals) {
		Literal* next;
		if (clause1idx < clause1.numLiterals && clause2idx < clause2.numLiterals) {
			next = clause1.literals[clause1idx].value < clause2.literals[clause2idx].value ? clause1.literals + clause1idx : clause2.literals + clause2idx;
		} 
		else if (clause1idx == clause1.numLiterals) next = clause2.literals + clause2idx;
	        else next = clause1.literals + clause1idx;	
		//printf("Next literal to look at: ");
		//printf("%lu\n", next->value);
		if (lastAdded && (next->value == lastAdded->value)) { // If the values are the same we either don't add or end because of conflict.
			if (next->sign != lastAdded->sign) {
				free(newLiterals); // since this pointer will not be accessible in the return result
				return emptySet; 
			}
			if (next == clause1.literals + clause1idx) clause1idx++;
			else clause2idx++;
		} else {
		 //printf("Adding literal to merged clause. \n");
		 newLiterals[newClauseSize] = *next;
		 newClauseSize++;
		 //printf("Num lits: %lu \n", newClauseSize);
		 lastAdded = next;
		}
		if (lastAdded == clause1.literals + clause1idx) clause1idx = clause1idx < clause1.numLiterals ? clause1idx + 1 : clause1idx;
		if (lastAdded == clause2.literals + clause2idx) clause2idx = clause2idx < clause2.numLiterals ? clause2idx + 1 : clause2idx;
	}
	Clause result; //= {newLiterals, newClauseSize};
	//printf("Merged clause before realloc: \n");
	//printClause(result);
	result.literals = realloc(newLiterals, sizeof(Literal) * newClauseSize);
	if (newClauseSize) assert(result.literals);
	result.numLiterals = newClauseSize;
	//printf("Clause after realloc finished (within merge call):\n");
	//printClause(result);
	return result;
}

int main(void) {
   unsigned long clauseVals1[] = {1, 2, 3, 4};
   bool clauseSigns1[] = {true, true, true, true};
   unsigned long clauseVals2[] = {1, 2, 3, 4, 5};
   bool clauseSigns2[] = {true, true, true, true, false};
   Clause clause1 = createClause(clauseVals1, clauseSigns1, 4);
   printClause(clause1);
   Clause clause2 = createClause(clauseVals2, clauseSigns2, 5);
   printClause(clause2);
   Clause merged = merge(clause1, clause2);
   printClause(merged);
   free(merged.literals);
   free(clause1.literals);
   free(clause2.literals);
   return 0;
}
