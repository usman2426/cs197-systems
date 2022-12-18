// looks like its much slower than vec but is also much less memory (half at least?)
// probably because using compressed bitset (roaring)

use std::ops::{BitAnd, BitOr, BitXor};

use roaring::RoaringBitmap;

use crate::{
    dnf::{Sign, DNF},
    Merge, MergeResult,
};

#[derive(Clone, Debug, Default)]
pub struct BitClause {
    literals: RoaringBitmap,
    signs: RoaringBitmap,
}

impl Merge for BitClause {
    fn merge(a: Self, b: &Self) -> MergeResult<Self> {
        // the signs for the intersection of literals must agree
        // in other words, (S1 ^ S2) & L1 & L2 must be empty
        // because S1 ^ S2 will only have 1's where the "sign" differed
        // so either there was a negative (0) and a positive (1)
        // or there was an undefined (also 0) and a positive
        // if there was undefined, we can ignore the xor false positive
        // by taking the AND with L1 and L2
        if !a
            .signs
            .clone()
            .bitxor(&b.signs)
            .bitand(&a.literals)
            .bitand(&b.literals)
            .is_empty()
        {
            MergeResult::Incompatible
        } else {
            // returns L1 | L2, S1 | S2
            MergeResult::Set(BitClause {
                literals: a.literals.bitor(&b.literals),
                signs: a.signs.bitor(&b.signs),
            })
        }
    }

    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self> {
        DNF::from(
            vec.iter()
                .map(|e| BitClause {
                    literals: e.iter().map(|(l, _)| l).collect::<RoaringBitmap>(),
                    signs: e
                        .iter()
                        .filter(|(_, s)| *s == Sign::Positive)
                        .map(|(l, _)| l)
                        .collect::<RoaringBitmap>(),
                })
                .collect::<Vec<BitClause>>(),
        )
    }

    fn len(&self) -> usize {
        self.literals.len() as usize
    }
}
