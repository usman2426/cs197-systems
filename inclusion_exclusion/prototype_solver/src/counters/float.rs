use std::fmt::Display;

use super::Counter;

#[derive(Clone)]
pub struct FloatCounter(f64);

impl Display for FloatCounter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!("{}", self.0))
    }
}

impl Counter for FloatCounter {
    fn new(_max_bit_hint: u32) -> Self {
        FloatCounter(0.0)
    }

    fn equal(&self, power_of_two: u32) -> bool {
        self.0 == f64::powf(2.0, power_of_two as f64)
    }

    fn less_than(&self, power_of_two: u32) -> bool {
        self.0 < f64::powf(2.0, power_of_two as f64)
    }

    fn add(&mut self, power_of_two: u32) {
        self.0 += f64::powf(2.0, power_of_two as f64)
    }

    fn sub(&mut self, power_of_two: u32) {
        self.0 -= f64::powf(2.0, power_of_two as f64)
    }
}
