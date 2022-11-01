// Forget about #1 for now and focus on #2. How can we estimate this? Well,
// pick a k and then pick k clauses at random and see if their
// merge/intersection is empty. Repeat this, say, 100 times for each k from
// 1 to 100. That should give us an estimate, for each k, of how many
// "merged clauses" we'll have to deal with at each depth. Fewer is better.

// Run this program on all of SAT-COMP. Look for instances that have few
// merges/intersections, and also that existing SAT solvers do poorly on
// (e.g., run kissat with a short timeout for each). That should give you a
// good idea of which problem instances might be most worthwhile to target.
// The goal would be to find at least one problem in SAT-COMP that
// inclusion/exclusion should be expected to do much better on.

// I expect this to be perhaps a bit easier than actually implementing the
// solver? Again, all you need here is a way to read the DIMACS input
// files, choose k of them at random (in Python, check out the
// random.choices method), and then check if any of them contain
// conflicting literals. Then run that against the SAT-COMP benchmark
// suite.

use std::{
    fs::{self, DirEntry},
    time::Instant,
};

use inc_exc::{load::parse_dimacs, solve, SolutionResult};

fn main() {
    let check_file = |file: DirEntry, expected_result: SolutionResult| {
        // filter
        // if !file.file_name().to_string_lossy().contains("minimal.cnf") {
        //     return;
        // }
        println!("===== {} =====", file.file_name().to_string_lossy());
        let dnf = parse_dimacs(&{
            let file = &file.path();
            fs::read_to_string(file)
                .expect(&format!("Failed to open file {}", file.to_string_lossy()))
        })
        .expect("invalid DIMACS in sample input")
        .1;

        let start = Instant::now();
        let (result, _, _) = solve(&dnf, 4);
        let duration = start.elapsed();
        if result != expected_result {
            eprintln!(
                "Expected {:?} for {}, but got {:?}",
                expected_result,
                file.file_name().to_string_lossy(),
                result
            );
        }

        println!("{:?} for {} clauses", duration, dnf.len());
    };
    let check_dir = |dir: &str, expected_result: SolutionResult| {
        for file in fs::read_dir(dir).unwrap().map(|entry| entry.unwrap()) {
            check_file(file, expected_result)
        }
    };
    check_dir("samples/sat", SolutionResult::Satisfiable);
    check_dir("samples/unsat", SolutionResult::Unsatisfiable);
}
