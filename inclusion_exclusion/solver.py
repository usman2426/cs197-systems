import sys

"""
This is the python implementation of the sat solver, please
note that this is for reference pruposes only. For example,
testing any algorithmic optimizations
"""



###############################################
# Please note that this is not tested yet so
# there might be small bugs here and there - will
# be addressing them soon !!
#################################################

def negateLiteral(literalStr):
	# takes in a string literal, converts to an int, and negates it
	return -1 * int(literalStr)

def parseDIMACS(dimacs):
    numLiterals = 0
    numClauses = 0
    clauses = []
    dimacsFile = open(dimacs)
    for line in dimacsFile:
        if line[0] == 'p':
            data = line.strip().split(" ")
            numLiterals = int(data[2])
            numClauses = int(data[3])
        elif line[0] != 'c':
            clauseLiterals = list(map(negateLiteral, line.strip().split(" ")))
            clauseLiterals.pop() # removes 0 at end of line
            if len(clauseLiterals) > 0:
                clause = set(clauseLiterals)
                clauses.append(clause)
    dimacsFile.close()
    # the returned clause is in DNF form
    return numLiterals, numClauses, clauses

def merge(clause1, clause2):
	# removes all literals in common
    possibleConflicts = clause1.symmetric_difference(clause2)
    seen = set() # stores all the seen literals
    conflicts = set()
    for literal in possibleConflicts:
        if (abs(literal)) in unique:
            conflicts.add(literal)
        else:
            unique.add(abs(literal))
    if len(conflicts) == 0:
            return clause1.union(clause2)  #sucessfully merged set
    else:
            return set() # empty set, because no solutions for this merge

def countSolns(clause, numLiterals):
    power = numLiterals - len(clause)
    return 2**power

def calculateSummation(previousGeneration, lookupMerges, numLiterals):
	summation = 0
	#TODO: 
	#determine some data structure to store and access previous and new merges
	# for gen 1, we just skip merge and only calcuate solutions
	#for each previousMerge in previousGeneration(this is likely a set or list of set/list of clauses)
		# determine new clauses to merge with it
		# determine a newClause to merge with it
		# previousMergeResult = lookupMerges(previousMerge)
		# if len(previousMergeResult) != 0:
		# 	newMerge = merge(previousMergeResult, newClause)
		#   store previousMergeResult in lookup table @ previousMerge with newClause added
		# 	summation += countSolns(newMergeResult, numLiterals)
	return summation


def calc_sol_for_kpair(interim_list, generations, numSolutions, numLiterals, clauses):
        prev_generation_list = interim_list[0 : len(interim_list) - 1]
        prev_generation_string = "".join(prev_generation)
        prev_generation_clause = generations[prev_generation_string]
        # add logic for optimization for prev generation sol are zero
        
        cur_clause_number = interim_list[ len(interim_list) - 1 ]
        merged_clause = merge(prev_generation_clause, clauses[cur_clause_number])
        num_solns = countSolns(merged_clause, numLiterals)

        cur_generation_string = "".join(interim_list)

        # set the current generation - both the clause and the number of solutions
        generations[cur_generation_string] = merged_clause
        numSolutions[cur_generation_string] = num_solns

        return num_solns
                               

"""
Parameters information:
generations: this is a dict that will have strings as values and sets as keys
             for example the merge of clause 1 and 2 is represented with string "12" 
             and key is the union of the clauses
 
numSolutions: this is also a dict with strings as values and integers as keys
              for example if the merge of clause 1 and 2 has 2 solutions then we have "12" : 2
"""

def gen_num_sol(generations, numSolutions, numClauses, numLiterals, k, clauses):
        # this is a list of of the k-pairs, where each element is a list
        out = []
        my_stack = [[]] #this is a list of lists where each elem is a list of the interim k-pair

        num_sol = 0
        while(my_stack):
                interim_list = my_stack.pop(0)
                if len(interim_list) == k:
                        print("found a", k, "-pair")
                        out.append(interim_list)
                        num_sol += calc_sol_for_kpair(interim_list, generations, numSolutions, numLiterals, clauses)
                        
                else:
                        # we still need to keep building this list we found
                        index = 0
                        # set i to the correct starting position for our for loop
                        if not interim_list: # when list empty we set to 1
                                index = 1
                        else: # we set to the value just above the value of the last elem
                                index = interim_list[len(interim_list) - 1] + 1
                        for i in range(index, (numClauses + 1)):
                                new_list = interim_list.copy()
                                new_list.append(i)
                                my_stack.append(new_list)
                        print("stack state: ", my_stack)
                

        for i in range (len(out)):
                print(i, "th", k ,"-pair, printing:")
                print(out[i])
        return num_sol
        


def populate_first_gen(generations, numSolutions, numLiterals, clauses):
        first_gen_sol = 0
        for i in range(len(clauses)):
                num_soln = countSolns(clauses[i], numLiterals)
                first_gen_sol += num_soln
                numSolutions[str(i+1)] = num_soln
                generations[str(i+1)] = clauses[i]
        return first_gen_sol
                
        

def sat_solver(numLiterals, numClauses, clauses):
        # this is a dict that will have strings as values and sets as keys
        # for example the merge of clause 1 and 2 is represented with string "12" and key is the union
        # of the clauses
        generations = {}
        # this is also a dict with strings as values and integers as keys
        # for example if the merge of clause 1 and 2 has 2 solutions then we have "12" : 2
        numSolutions = {}
        # implement the I1 and I2 logic here, which will use the gen_num_sol function to
        # see how many solutions to the merge of k clauses where k is the size
        
        # gen_num_sol(generations, numSolutions, numClauses, 3)

        total_possible_sol = 2 ** numClauses
        num_sol_fgen = populate_first_gen(generations, numSolutions, numLiterals, clauses)
        print("first gen num Sol ", numSolutions)
        print("first gen generation ", generations)
        # if solutions for first gen < total_possible, it is "sat"
        if (num_sol_fgen < total_possible_sol):
                return "sat"

        
        num_cur_sol = 0
        print("total possible sol: ", total_possible_sol)
        for i in range(2, (numClauses + 1)):
                # when at odd step
                if (i % 2) == 1:
                        num_sol_k_pairs = gen_num_sol(generations, numSolutions, numClauses, numLiterals, i, clauses)
                        num_cur_sol += num_sol_k_pairs
                        # here we over estimated so if it is less than the CNF is SAT
                        if (num_cur_sol < total_possible_sol):
                                return "sat"
                else:
                        # we are at the even step
                        num_sol_k_pairs = gen_num_sol(generations, numSolutions, numClauses, numLiterals, i, clauses)
                        num_cur_sol -= num_sol_k_pairs
                        # here we underestimated so if it is equal to total_possible, then it is unsat
                        if (num_cur_sol == total_possible_sol):
                                return "unsat"
                

def main():
        print("ahahah")
        dimacs = sys.argv[1] #takes DIMACS input as command line input
        print(dimacs)
        numLiterals, numClauses, clauses = parseDIMACS(dimacs)
        print(numClauses)
        print (type(clauses))
        output = sat_solver(numLiterals, numClauses, clauses)
        print(output)
        #print(clauses)
	
	### instantiate data structures
	### I wrapper logic
		### currentTotal +/- calculateSummation(previousGeneration, lookupMerges, numLiterals)
	### check if conditions met for SAT/UNSAT

if __name__ == "__main__":
        main()
