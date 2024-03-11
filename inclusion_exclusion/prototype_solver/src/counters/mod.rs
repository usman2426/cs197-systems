use std::fmt::Display;

pub mod bignum;
pub mod float;
pub mod gmp;

pub trait Counter
where
    Self: Display,
{
    fn new(max_bit_hint: u32) -> Self;
    fn equal(&self, power_of_two: u32) -> bool;
    fn less_than(&self, power_of_two: u32) -> bool;
    fn add(&mut self, power_of_two: u32);
    fn sub(&mut self, power_of_two: u32);
}
