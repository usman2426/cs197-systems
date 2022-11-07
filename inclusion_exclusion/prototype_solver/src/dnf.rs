use std::ops::{Index, Not};

use crate::Merge;

pub struct DNF<T: Merge>(Vec<T>);

// indexing into a dnf
impl<Clause: Merge> Index<usize> for DNF<Clause> {
    type Output = Clause;

    fn index(&self, index: usize) -> &Self::Output {
        &self.0[index]
    }
}

// implement Into for a vector of clauses to a DNF of clauses
impl<Clause: Merge> From<Vec<Clause>> for DNF<Clause> {
    fn from(value: Vec<Clause>) -> Self {
        DNF(value)
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum Sign {
    Positive,
    Negative,
}

impl Not for Sign {
    type Output = Sign;
    fn not(self) -> Self::Output {
        match self {
            Self::Positive => Self::Negative,
            Self::Negative => Self::Positive,
        }
    }
}
