use bit_set::BitSet;

use crate::{
    dnf::{Sign, DNF},
    Merge, MergeResult,
};

/// must always be sorted
#[derive(Clone, Debug, Default)]
pub struct BitClause {
    literals: BitSet,
    signs: BitSet,
}

impl Merge for BitClause {
    fn merge(mut a: Self, b: &Self, _total_size_hint: u32) -> MergeResult<Self> {
        for l in b.literals.iter() {
            if !a.literals.insert(l) {
                if a.signs.contains(l) != b.signs.contains(l) {
                    return MergeResult::Incompatible;
                }
            } else {
                // only for positive
                if b.signs.contains(l) {
                    a.signs.insert(l);
                }
            }
        }
        MergeResult::Set(a)
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
