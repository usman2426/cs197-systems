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
            clauseLiterals = map(negateLiteral, line.strip().split(" "))
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
    return 2^power

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

def main():
	dimacs = sys.argv[1] #takes DIMACS input as command line input
	numLiterals, numClauses, clauses = parseDIMACS(dimacs)
	currentTotal = 0
	### instantiate data structures
	### I wrapper logic
		### currentTotal +/- calculateSummation(previousGeneration, lookupMerges, numLiterals)
	### check if conditions met for SAT/UNSAT
