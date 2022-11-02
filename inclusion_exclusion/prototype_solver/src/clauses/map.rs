//! Vector implementation of clause storage and merging

use std::collections::HashMap;

use crate::{
    load::{Sign, DNF},
    Merge, MergeResult,
};

#[derive(Clone)]
pub struct MapClause(HashMap<u32, Sign>);

impl Default for MapClause {
    fn default() -> Self {
        Self(Default::default())
    }
}

impl Merge for MapClause {
    fn merge(mut a: MapClause, b: &MapClause) -> MergeResult<MapClause> {
        for (literal, sign) in &b.0 {
            if a.0.insert(*literal, *sign) == Some(!*sign) {
                return MergeResult::Incompatible;
            }
        }
        MergeResult::Set(a)
    }

    fn from_vec(vec: Vec<Vec<(u32, Sign)>>) -> DNF<Self> {
        DNF::from(
            vec.iter()
                .map(|e| MapClause(e.iter().cloned().collect::<HashMap<u32, Sign>>()))
                .collect::<Vec<MapClause>>(),
        )
    }

    fn len(&self) -> usize {
        self.0.len()
    }
}
