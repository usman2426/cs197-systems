use std::collections::VecDeque;

pub mod clauses;
pub mod load;

use load::{Sign, DNF};

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum SolutionResult {
    Inconclusive,
    Satisfiable,
    Unsatisfiable,
}

pub trait Merge
where
    Self: Sized + Clone + Default,
{
    fn merge(a: Self, b: &Self) -> MergeResult<Self>;
    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self>;
    fn len(&self) -> usize;
}

pub enum MergeResult<T: Merge> {
    Set(T),
    Incompatible,
}

// max_size is the maximum allowed size for subset generation
pub fn solve<Clause: Merge>(
    dnf: &DNF<Clause>,
    num_vars: u32,
    num_clauses: u32,
    max_size: usize,
) -> (SolutionResult, usize, f64) {
    // 0th generation. no combination of indices yet
    let mut queue: VecDeque<(Vec<usize>, Clause)> =
        VecDeque::from(vec![(vec![], Clause::default())]);
    // critical variable for testing for completion.
    let mut sum: f64 = 0.0;
    // total number of variables in all the clauses. (alternatively, parse from the dimacs input)
    let total_vars = num_vars as i32;
    // the queue can contain future and current generations (generations by size of subset)
    // therefore, we use max_seen_size to determine when we have finished one generation
    // and will start processing the next generation
    let mut max_seen_size = 0;
    while let Some((set, merge_result)) = queue.pop_front() {
        if set.len() != max_size {
            // extend by another index
            // this new index must be greater than the last index in the set
            // because our set is an ordered list of increasing indices, this guarantees
            // every new set we create will be unique
            for new_index in (set.last().map(|&e| e + 1).unwrap_or(0))..(num_clauses as usize) {
                // merge the cached set with the clause at the new_index
                match Clause::merge(merge_result.clone(), &dnf[new_index]) {
                    // if the merge succeeds,
                    MergeResult::Set(clause) => {
                        let mut new_set = set.clone();
                        new_set.push(new_index);
                        // add to sum the size of the solution space for this merged set
                        let current_term = f64::powi(2.0, total_vars - clause.len() as i32);
                        if new_set.len() % 2 == 0 {
                            sum -= current_term;
                        } else {
                            sum += current_term
                        };
                        // add this set of clauses and the merge result in the queue to be explored further
                        queue.push_back((new_set, clause));
                    }
                    // if incompatible, we do not wish to further explore this and thus do not insert into queue
                    // HOWEVER, it may be wise to create a cache structure to allow storing INVALID results
                    // for example, we may explore 1,3 before 1,2,3. if 1,3 is invalid, we may store that it is invalid and look up later
                    // 1,3 is not an obvious direct ancestor of 1,2,3. thus, this search would necessitate linear probing of
                    // every possible indirect ancestor in the previous generation: 2,3; 1,3; 1,2
                    // thus, we do not implement it here
                    MergeResult::Incompatible => {}
                }
            }
        } else {
            // reached maximum size
            return (SolutionResult::Inconclusive, max_seen_size, sum);
        }
        // if this new length is greater than all previous seen sizes, we must have
        // reached a new level of lengths (advanced to next generation)
        // by nature of the queue logic, the sizes must be always nondecreasing
        // therefore, once we "level up", we may analyze the sum, as it now accounts for the entire previous level of sizes
        if set.len() > max_seen_size {
            max_seen_size = set.len();
            // check the sum and see if it is (a lower bound and above 2^N) or (an upper bound and below 2^n)
            if max_seen_size % 2 == 0 && sum == f64::powi(2.0, total_vars) {
                return (SolutionResult::Unsatisfiable, max_seen_size, sum);
            } else if max_seen_size % 2 == 1 && sum < f64::powi(2.0, total_vars) {
                return (SolutionResult::Satisfiable, max_seen_size, sum);
            }
        }
    }
    // the last "level up" is not caught inside the loop
    // therefore, we add this final check after we've exhausted all search paths in the queue and exited the loop
    if sum >= f64::powi(2.0, total_vars) {
        (SolutionResult::Unsatisfiable, max_seen_size, sum)
    } else {
        (SolutionResult::Satisfiable, max_seen_size, sum)
    }
}
