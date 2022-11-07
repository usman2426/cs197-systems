use std::fmt::Display;

use rug::Integer;

use super::Counter;

#[derive(Clone)]
pub struct GmpCounter(Integer);

impl Display for GmpCounter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("{}", self.0))
    }
}

impl Default for GmpCounter {
    fn default() -> Self {
        Self(Default::default())
    }
}

impl Counter for GmpCounter {
    /// We may not know an exact bound on the maximum number for the counter
    /// but the counter will likely reach the number of variables in the dnf/cnf
    /// therefore, we can preallocate up to there
    fn new(max_bit_hint: u32) -> Self {
        let mut out = Integer::new();
        out.reserve(max_bit_hint as usize);
        GmpCounter(out)
    }

    fn equal(&self, power_of_two: u32) -> bool {
        self.0 == Integer::from(Integer::i_pow_u(2, power_of_two))
    }

    fn less_than(&self, power_of_two: u32) -> bool {
        self.0 < Integer::from(Integer::i_pow_u(2, power_of_two))
    }

    fn add(&mut self, power_of_two: u32) {
        self.0 += Integer::from(Integer::i_pow_u(2, power_of_two))
    }

    fn subtract(&mut self, power_of_two: u32) {
        self.0 -= Integer::from(Integer::i_pow_u(2, power_of_two))
    }
}
