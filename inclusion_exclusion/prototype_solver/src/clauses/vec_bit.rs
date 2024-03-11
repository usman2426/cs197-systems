//! adapts from list to bitmap when sufficient density is achieved
//! memory usage of a VecClause is n*(32+8) where n is the number of literals in a clause
//! memory usage of a plain BitSet is 2*(N/32) for the two bitsets. one for the sign and one for the literals where N is the total num of literals
//! merging VecClause takes O(n+m)
//! merging a bit based clause takes O(n)
//! makes sense memory-wise to switch when n*(32+8) > 2*(N/32) or equivalently n > N / 16 / 40 or n > N / 640

use crate::{
    dnf::{Sign, DNF},
    Merge, MergeResult,
};

use super::{bit::BitClause, vec::VecClause};

#[derive(Clone, Debug)]
/// As in adaptive clause
pub enum AdaClause {
    Vec(VecClause),
    Bit(BitClause),
}

impl Default for AdaClause {
    fn default() -> Self {
        AdaClause::Vec(VecClause::default())
    }
}

impl Merge for AdaClause {
    fn merge(a: Self, b: &Self, total_size_hint: u32) -> MergeResult<Self> {
        macro_rules! result {
            ( $x:expr ) => {{
                match $x {
                    MergeResult::Set(set) => set,
                    MergeResult::Incompatible => return MergeResult::Incompatible,
                }
            }};
        }

        use AdaClause::*;
        MergeResult::Set(match (a, b) {
            (Vec(a), Vec(b)) => {
                let vec_clause = result!(VecClause::merge(a, b, total_size_hint));
                // see comments at top of file
                if vec_clause.len() > (total_size_hint as usize) / 640 {
                    AdaClause::Bit(BitClause::from(vec_clause.into_vec()))
                } else {
                    AdaClause::Vec(vec_clause)
                }
            }
            (Bit(a), Vec(b)) => {
                // convert b to bits
                AdaClause::Bit(result!(BitClause::merge(
                    a,
                    &BitClause::from(b.clone().into_vec()),
                    total_size_hint
                )))
            }
            (Vec(a), Bit(b)) => AdaClause::Bit(result!(BitClause::merge(
                BitClause::from(a.clone().into_vec()),
                b,
                total_size_hint
            ))),
            (Bit(a), Bit(b)) => AdaClause::Bit(result!(BitClause::merge(
                a, 
                b, 
                total_size_hint
            ))),
        })
    }

    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self> {
        DNF::from(
            // construction code copied from vec implementation
            vec.into_iter()
                .map(|mut list| {
                    list.sort_unstable_by_key(|e| e.0);
                    list.dedup();
                    AdaClause::Vec(VecClause::from(list))
                })
                .collect::<Vec<AdaClause>>(),
        )
    }

    fn len(&self) -> usize {
        match self {
            AdaClause::Vec(vec) => vec.len(),
            AdaClause::Bit(bits) => bits.len(),
        }
    }
}
