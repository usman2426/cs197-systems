use bit_set::BitSet;

use crate::{
    dnf::{Sign, DNF},
    Merge, MergeResult,
};

use super::vec::VecClause;

/// must always be sorted
#[derive(Clone, Debug, Default)]
pub struct BitClause {
    literals: BitSet,
    signs: BitSet,
}

impl Merge for BitClause {
    fn merge(a: Self, b: &Self) -> MergeResult<Self> {
        let mut conflicts = a.signs.clone();
        conflicts.symmetric_difference_with(&b.signs);
        conflicts.intersect_with(&a.literals);
        conflicts.intersect_with(&b.literals);
        // same logic as in compressed bitset
        if !conflicts.is_empty() {
            MergeResult::Incompatible
        } else {
            // returns L1 | L2, S1 | S2
            MergeResult::Set(BitClause {
                literals: a.literals.union(&b.literals).collect(),
                signs: a.signs.union(&b.signs).collect(),
            })
        }
    }

    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self> {
        DNF::from(
            vec.iter()
                .map(|e| {
                    let (mut literals, mut signs) = (BitSet::new(), BitSet::new());
                    for (l, s) in e {
                        // bitset only takes usizes
                        literals.insert(*l as usize);
                        if *s == Sign::Positive {
                            signs.insert(*l as usize);
                        }
                    }
                    BitClause { literals, signs }
                })
                .collect::<Vec<BitClause>>(),
        )
    }

    fn len(&self) -> usize {
        self.literals.len()
    }
}

impl From<Vec<(u32, Sign)>> for BitClause {
    fn from(value: Vec<(u32, Sign)>) -> Self {
        let mut out = BitClause::default();
        for (l, s) in value {
            out.literals.insert(l as usize);
            if s == Sign::Positive {
                out.signs.insert(l as usize);
            }
        }
        out
    }
}
