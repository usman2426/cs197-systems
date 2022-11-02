#![allow(unused)]
use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use inc_exc::{
    clauses::{vec::VecClause, map::MapClause},
    load::{Sign, DNF},
    solve, Merge,
};
use rand::{thread_rng, Rng};

use std::iter;

fn gen_random_dnf<T: Merge>(
    num_vars: u32,
    num_clauses: usize,
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
        BenchmarkId::new("VecClause", "10, 10, 3"),
        &gen_random_dnf::<VecClause>(10, 10, Some(3)),
        |b, dnf| {
            b.iter(|| solve(dnf, 10, 10, 3));
        },
    );
    group.bench_with_input(
        BenchmarkId::new("MapClause", "10, 10, 3"),
        &gen_random_dnf::<MapClause>(10, 10, Some(3)),
        |b, dnf| {
            b.iter(|| solve(dnf, 10, 10, 3));
        },
    );
    group.finish();
}

fn vary_var_number(c: &mut Criterion) {
    let mut group = c.benchmark_group("Varying variable number");
    for num_vars in (4..60).step_by(2) {
        group.throughput(Throughput::Elements(num_vars));
        group.bench_with_input(
            BenchmarkId::new("VecClause", num_vars),
            &gen_random_dnf::<VecClause>(num_vars as u32, 10, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, num_vars as u32, 10, 3));
            },
        );
        group.bench_with_input(
            BenchmarkId::new("MapClause", num_vars),
            &gen_random_dnf::<MapClause>(num_vars as u32, 10, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, num_vars as u32, 10, 3));
            },
        );
    }
    group.finish();
}

fn vary_clause_number(c: &mut Criterion) {
    let mut group = c.benchmark_group("Varying clause number");
    for num_clauses in (4..60).step_by(2) {
        group.throughput(Throughput::Elements(num_clauses));
        group.bench_with_input(
            BenchmarkId::new("VecClause", num_clauses),
            &gen_random_dnf::<VecClause>(10, num_clauses as usize, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, 10, num_clauses as u32, 3));
            },
        );
        group.bench_with_input(
            BenchmarkId::new("MapClause", num_clauses),
            &gen_random_dnf::<MapClause>(10, num_clauses as usize, Some(3)),
            |b, dnf| {
                b.iter(|| solve(dnf, 10, num_clauses as u32, 3));
            },
        );
    }
    group.finish();
}

criterion_group!(benches, single_point, vary_var_number, vary_clause_number);
criterion_main!(benches);
