use criterion::{criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use inc_exc::{
    clauses::{map::MapClause, vec::VecClause},
    dnf::{Sign, DNF},
    solve, Merge,
};
use rand::{thread_rng, Rng};

const NUM_VARS: u32 = 100;
const MIN_VARS: u32 = 20;
const MAX_VARS: u32 = 200;
const NUM_CLAUSES: u32 = 20;
const MIN_CLAUSES: u32 = 10;
const MAX_CLAUSES: u32 = 100;

fn gen_random_dnf<T: Merge>(
    num_vars: u32,
    num_clauses: u32,
    vars_per_clause: Option<usize>,
) -> DNF<T> {
    let mut rng = thread_rng();
    let mut output = vec![];
    for _ in 0..num_clauses {
        let mut clause = vec![];
        for _ in 0..(vars_per_clause.unwrap_or(3)) {
            clause.push((
                rng.gen_range(0..num_vars),
                if rng.gen() {
                    Sign::Positive
                } else {
                    Sign::Negative
                },
            ))
        }
        output.push(clause)
    }
    T::from_vec(output)
}

fn single_point(c: &mut Criterion) {
    let mut group = c.benchmark_group("Single Point");
    group.bench_with_input(
        BenchmarkId::new("VecClause", format!("{NUM_VARS}, {NUM_CLAUSES}, 3")),
        &gen_random_dnf::<VecClause>(NUM_VARS, NUM_CLAUSES, Some(3)),
        |b, dnf| {
            b.iter(|| solve(dnf, NUM_VARS, NUM_CLAUSES, 3));
        },
    );
    group.bench_with_input(
        BenchmarkId::new("MapClause", format!("{NUM_VARS}, {NUM_CLAUSES}, 3")),
        &gen_random_dnf::<MapClause>(NUM_VARS, NUM_CLAUSES, Some(3)),
        |b, dnf| {
            b.iter(|| solve(dnf, NUM_VARS, NUM_CLAUSES, 3));
        },
    );
    group.finish();
}

fn vary_var_number(c: &mut Criterion) {
    let mut group = c.benchmark_group("Varying variable number");
    for num_vars in (MIN_VARS..MAX_VARS).step_by(((MAX_VARS - MIN_VARS) / 20) as usize) {
        group.throughput(Throughput::Elements(num_vars as u64));
        group.bench_with_input(
            BenchmarkId::new("VecClause", format!("{num_vars}, {NUM_CLAUSES}, 3")),
            &gen_random_dnf::<VecClause>(num_vars, NUM_CLAUSES, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, num_vars, NUM_CLAUSES, 3));
            },
        );
        group.bench_with_input(
            BenchmarkId::new("MapClause", format!("{num_vars}, {NUM_CLAUSES}, 3")),
            &gen_random_dnf::<MapClause>(num_vars, NUM_CLAUSES, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, num_vars, NUM_CLAUSES, 3));
            },
        );
    }
    group.finish();
}

fn vary_clause_number(c: &mut Criterion) {
    let mut group = c.benchmark_group("Varying clause number");
    for num_clauses in
        (MIN_CLAUSES..MAX_CLAUSES).step_by(((MAX_CLAUSES - MIN_CLAUSES) / 20) as usize)
    {
        group.throughput(Throughput::Elements(num_clauses as u64));
        group.bench_with_input(
            BenchmarkId::new("VecClause", format!("{NUM_VARS}, {num_clauses}, 3")),
            &gen_random_dnf::<VecClause>(NUM_VARS, num_clauses, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, NUM_VARS, num_clauses, 3));
            },
        );
        group.bench_with_input(
            BenchmarkId::new("MapClause", format!("{NUM_VARS}, {num_clauses}, 3")),
            &gen_random_dnf::<MapClause>(NUM_VARS, num_clauses, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, NUM_VARS, num_clauses, 3));
            },
        );
    }
    group.finish();
}

criterion_group!(benches, single_point, vary_var_number, vary_clause_number);
criterion_main!(benches);
