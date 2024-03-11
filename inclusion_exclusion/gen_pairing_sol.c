typedef struct Literal {
    unsigned long value;
    bool sign;
} Literal;

typedef struct Clause {
    Literal *literals;
    unsigned long numLiterals;    
} Clause;


typedef struct GenChild {
    unsigned long last_clause_num;
    unsigned long num_sol;
    Clause *merged_clause;
} GenChild;

typedef struct 

unsigned long gen_size(unsigned long k, unsigned long total_clauses){
    // basically we need to return total_clauses choose k
    return 6;
}

/*
* parameters:
*  prev_generation: an array of GenChilds that stores the solutions to
                    the last merge (so if we are calling with k = 3 then
                    it stores the sols to k = 2) - technically the memory address
                    of the variable that stores the array is what prev_generation is
*/
int gen_num_sol(GenChild **prev_generation, unsigned long *array_size, unsigned long total_clauses, 
                unsigned long k, Clause* clauses)
{
    // so we know that when k is 2, we have the results of the 1st generation ready and we
    // want to calculate the results of the next generation
    unsigned long prev_gen_size = *array_size;
    GenChild *prev_gen = *prev_generation;
    GenChild *new_gen = malloc(gen_size(k, total_clauses) * sizeof(GenChild))
    unsigned long cur_index = 0;
    unsigned long total_solution = 0;
    for(unsigned long i = 0; i < prev_gen_size; ++i){
        // iterate over each entry to see the new generation k-pairs that can be made
        GenChild cur_child = prev_gen[i];
        unsigned long last_clause = cur_child.last_clause_num;
        // loop over all claues we can merge this one with
        for (unsigned long j = last_clause + 1; j <= total_clauses; ++j){
            unsigned long cur_clause_num = j;
            // we need to merge cur_child_clause with this clause
            // clause array should be such that clause 1 is at index 1
            Clause *result = merge(clauses[cur_clause_num], cur_child.merged_clause);
            unsigned long num_sol = calc_num_sol(result);
            if ((num_sol) == 0) {continue;}
            // add the merged clause to the new generation
            new_gen[cur_index] = {cur_clause, num_sol, result};
            cur_index++;
            total_solution += num_sol;
        }
    }
    // we need to free the previous generation, obviously also the merged clauses 
    // within it
    // and then this new_gen becomes the prev_gen
    *prev_generation = new_gen;
}


