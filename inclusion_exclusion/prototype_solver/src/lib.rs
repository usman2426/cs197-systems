pub mod clauses;
pub mod counters;
pub mod dnf;
pub mod load;

use clauses::MergeResult;
pub use clauses::{Merge, SolutionResult};
use counters::Counter;
use dnf::DNF;

// max_size is the maximum allowed size for subset generation
pub fn solve<Clause: Merge, Count: Counter>(
    dnf: &DNF<Clause>,
    num_vars: u32,
    num_clauses: u32,
    max_size: usize,
) -> (SolutionResult, usize, Count) {
    // critical variable for testing for completion.
    // generic over multiple implementation for counting (should be zero cost: monomorphization)
    let mut sum: Count = Count::new(num_vars);

    let mut current_generation: Vec<(usize, Clause)> = vec![(0, Clause::new_empty())];
    let mut next_generation: Vec<(usize, Clause)>;

    for combo_size in 0..max_size {
        if current_generation.len() == 0 {
            break;
        }
        next_generation = vec![];
        for (last_index, merge_result) in current_generation.iter() {
            for new_index in *last_index..(num_clauses as usize) {
                if let MergeResult::Set(new_merge_result) =
                    Clause::merge(merge_result.clone(), &dnf[new_index])
                {
                    // new_index + 1 is the starting point for the combos now
                    next_generation.push((new_index + 1, new_merge_result));
                }
            }
        }

        // calculating sum
        for (_, clause) in next_generation.iter() {
            if combo_size % 2 == 1 {
                sum.sub(num_vars - clause.len() as u32);
            } else {
                sum.add(num_vars - clause.len() as u32);
            };
        }

        current_generation = next_generation;

        if combo_size % 2 == 1 && sum.equal(num_vars) {
            return (SolutionResult::Unsatisfiable, combo_size, sum);
        } else if combo_size % 2 == 0 && sum.less_than(num_vars) {
            return (SolutionResult::Satisfiable, combo_size, sum);
        }
    }

    (SolutionResult::Inconclusive, max_size - 1, sum)
}
