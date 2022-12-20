use std::fs::{self, DirEntry};

use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion};
use inc_exc::{
    clauses::bit::BitClause, counters::bignum::BigCounter, load::parse_dimacs, solve,
};

// fn gen_random_dnf<T: Merge>(
//     num_vars: u16,
//     num_clauses: u32,
//     vars_per_clause: Option<usize>,
// ) -> DNF<T> {
//     let mut rng = thread_rng();
//     let mut output = vec![];
//     for _ in 0..num_clauses {
//         let mut clause = vec![];
//         for _ in 0..(vars_per_clause.unwrap_or(3)) {
//             clause.push((
//                 rng.gen_range(0..num_vars),
//                 if rng.gen() {
//                     Sign::Positive
//                 } else {
//                     Sign::Negative
//                 },
//             ))
//         }
//         output.push(clause)
//     }
//     T::from_vec(output)
// }

// fn single_point(c: &mut Criterion) {
//     let mut group = c.benchmark_group("Single Point");
//     for (num_vars, num_clauses) in [
//         (100, 10),
//         (100, 60),
//         (60, 6),
//         (60, 36),
//         (1000, 100),
//         (1000, 600),
//     ] {
//         group.bench_with_input(
//             BenchmarkId::new("VecClause", format!("{num_vars}, {num_clauses}, 3")),
//             &gen_random_dnf::<VecClause>(num_vars, num_clauses, Some(3)),
//             |b, dnf| {
//                 b.iter(|| solve::<_, BigCounter>(dnf, num_vars as u32, num_clauses, 3));
//             },
//         );
//     }
//     group.finish();
// }

fn general_testing(c: &mut Criterion) {
    let mut group = c.benchmark_group("ShaTar Testing");

    let mut bench = |dir_name: &str, bench_name: &str| {
        let mut files = fs::read_dir("../ShaTar Testing/".to_string() + dir_name)
            .unwrap()
            .map(|entry| entry.unwrap())
            .collect::<Vec<DirEntry>>();
        // deterministic
        files.sort_by(|a, b| b.path().cmp(&a.path()));
        for file in files {
            // blacklist for testing purposes
            if file.file_name().to_str().unwrap().contains("30v") {
                // if vec!["30v"].contains(&file.file_name().to_str().unwrap()) {
                continue;
            }

            group
                .bench_with_input(
                    BenchmarkId::new(bench_name, format!("{}", file.path().to_string_lossy())),
                    &parse_dimacs::<BitClause>(&{
                        let file = &file.path();
                        fs::read_to_string(file)
                            .expect(&format!("Failed to open file {}", file.to_string_lossy()))
                    })
                    .expect("invalid DIMACS in sample input")
                    .1,
                    |b, (dnf, num_vars, num_clauses)| {
                        b.iter(|| solve::<_, BigCounter>(dnf, *num_vars, *num_clauses, 999));
                    },
                )
                .sample_size(10);
        }
    };
    bench("factoring test cases", "Factoring");
    bench("pigeonhole test cases", "Pigeonhole");
    bench("random test cases", "Random");
    bench("toughsat test cases", "Toughsat");

    group.finish();
}

criterion_group!(benches, general_testing);
criterion_main!(benches);
