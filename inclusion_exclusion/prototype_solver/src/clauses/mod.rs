use std::fmt::Debug;

use crate::dnf::{Sign, DNF};

pub mod bit;
pub mod map;
pub mod vec;
pub mod vec_bit;

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum SolutionResult {
    Inconclusive,
    Satisfiable,
    Unsatisfiable,
}

pub trait Merge
where
    Self: Sized + Clone + Default + Debug,
{
    fn merge(a: Self, b: &Self) -> MergeResult<Self>;
    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self>;
    fn len(&self) -> usize;
}

pub enum MergeResult<T: Merge> {
    Set(T),
    Incompatible,
}
