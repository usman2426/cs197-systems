//! Vector implementation of clause storage and merging
//! Requires all clauses to be initially sorted and therefore has linear time for merging

use crate::{
    dnf::{Sign, DNF},
    Merge, MergeResult,
};

/// must always be sorted
#[derive(Clone, Debug)]
pub struct VecClause(Vec<(u32, Sign)>);

impl Default for VecClause {
    fn default() -> Self {
        Self(Default::default())
    }
}

impl Merge for VecClause {
    fn merge(a: VecClause, b: &VecClause, _total_size_hint: u32) -> MergeResult<VecClause> {
        let mut a = a.0.iter();
        let mut b = b.0.iter();

        let mut index_a = a.next();
        let mut index_b = b.next();

        let mut output = Vec::with_capacity(a.len() + b.len());

        loop {
            match (index_a, index_b) {
                (Some(&a_r), Some(&b_r)) => {
                    // same literal
                    if a_r.0 == b_r.0 {
                        // differing signs
                        if a_r.1 != b_r.1 {
                            return MergeResult::Incompatible;
                        } else {
                            // same sign, advance both and put one into output
                            output.push(a_r);
                            index_a = a.next();
                            index_b = b.next();
                            continue;
                        }
                    }
                    if a_r.0 < b_r.0 {
                        output.push(a_r);
                        index_a = a.next();
                    } else {
                        output.push(b_r);
                        index_b = b.next();
                    }
                }
                (Some(&a_r), None) => {
                    output.push(a_r);
                    index_a = a.next();
                }
                (None, Some(&b_r)) => {
                    output.push(b_r);
                    index_b = b.next();
                }
                _ => return MergeResult::Set(output.into()),
            }
        }
    }

    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self> {
        // sort and deduplicate the clauses
        DNF::from(
            vec.into_iter()
                .map(|mut list| {
                    list.sort_unstable_by_key(|e| e.0);
                    list.dedup();
                    VecClause::from(list)
                })
                .collect::<Vec<VecClause>>(),
        )
    }

    fn len(&self) -> usize {
        self.0.len()
    }
}

impl From<Vec<(u32, Sign)>> for VecClause {
    fn from(value: Vec<(u32, Sign)>) -> Self {
        VecClause(value)
    }
}

impl VecClause {
    // to convert into bit.rs
    pub(crate) fn into_vec(self) -> Vec<(u32, Sign)> {
        self.0
    }
}
