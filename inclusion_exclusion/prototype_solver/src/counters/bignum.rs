use std::{fmt::Display, mem::size_of, ops::Not};

use super::Counter;

#[derive(Clone, Debug)]
/// Essentially a little endian number.
/// Stores least significant byte at smallest memory address
pub struct BigCounter {
    buffer: Vec<u64>,
    sign: Sign,
}

impl PartialEq for BigCounter {
    fn eq(&self, other: &Self) -> bool {
        if self.sign != other.sign {
            return false;
        }
        // allow for different expressions of the same number to be equal
        let mut a = self.buffer.iter();
        let mut b = self.buffer.iter();
        loop {
            match (a.next(), b.next()) {
                (None, None) => break true,
                (None, Some(r)) | (Some(r), None) => {
                    if *r != 0 {
                        break false;
                    }
                }
                (Some(a_r), Some(b_r)) => {
                    if a_r != b_r {
                        break false;
                    }
                }
            }
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
enum Sign {
    Positive,
    Negative,
}

impl Not for Sign {
    type Output = Self;

    fn not(self) -> Self::Output {
        match self {
            Self::Positive => Self::Negative,
            Self::Negative => Self::Positive,
        }
    }
}

impl Display for BigCounter {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_fmt(format_args!(
            "{}[{}]",
            if self.sign == Sign::Positive { "" } else { "-" },
            self.buffer
                .iter()
                .map(|e| e.to_string())
                .collect::<Vec<String>>()
                .join(", ")
        ))
    }
}

impl Counter for BigCounter {
    fn new(max_bit_hint: u32) -> Self {
        let mut out = vec![0];
        out.reserve(max_bit_hint as usize / size_of::<u64>() / 8);
        BigCounter {
            buffer: out,
            sign: Sign::Positive,
        }
    }

    /// Tests for equality of a Counter and a power of two
    ///
    /// # Examples
    ///
    /// ```
    /// # use inc_exc::counters::{bignum::BigCounter, Counter};
    /// let mut counter = BigCounter::new(0);
    /// // simply adding 2^1 should result in 2^1
    /// counter.add(1);
    /// assert!(counter.equal(1));
    /// // combining 2^1+2^1 should give 2^2. Adding 2^2 should give 2^3.
    /// counter = BigCounter::new(0);
    /// counter.add(1);
    /// counter.add(1);
    /// println!("{counter}");
    /// assert!(counter.equal(2));
    /// counter.add(2);
    /// assert!(counter.equal(3));
    /// // counter = 0 should not equal 2^0 and counter = 2 should not equal 2^0
    /// counter = BigCounter::new(0);
    /// assert!(!counter.equal(0));
    /// counter.add(0);
    /// assert!(counter.equal(0));
    /// ```
    fn equal(&self, power_of_two: u32) -> bool {
        if self.sign == Sign::Negative {
            return false;
        }
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);
        // if the bit at that index is not set, return false
        if let Some(integer) = self.buffer.get(block_index) {
            // is unset
            if integer & (1 << inner_index) == 0 {
                return false;
            }
        }
        // everything else must be 0
        // so we check if there are any nonzero values not matching block_index and inner_index
        !self.buffer.iter().enumerate().any(|(index, block)| {
            // nonzero block
            *block != 0 &&
            // that does not have both a matching index and a matching inner index
            !(index == block_index
                && (block ^ (1 << inner_index) == 0))
        })
    }

    /// Checks if the BigNum is less than a power of two
    ///
    /// # Examples
    ///
    /// ```
    /// # use inc_exc::counters::{bignum::BigCounter, Counter};
    /// let mut counter = BigCounter::new(0);
    /// // 0 < 2^0
    /// assert!(counter.less_than(0));
    /// // 1 == 2^0, so counter should not be less
    /// counter.add(0);
    /// assert!(!counter.less_than(0));
    /// // 2 > 2^0, so counter should not be less
    /// counter.add(0);
    /// assert!(!counter.less_than(0));
    /// let mut counter = BigCounter::new(0);
    /// // negative numbers should be less than 1
    /// counter.sub(1);
    /// assert!(counter.less_than(0));
    /// // large negative number should be also be less than powers of two
    /// counter.sub(99);
    /// assert!(counter.less_than(4));
    /// ```
    fn less_than(&self, power_of_two: u32) -> bool {
        if self.sign == Sign::Negative {
            return true;
        }
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);
        if block_index >= self.buffer.len() {
            return true;
        }
        // for the counter to be less, every bit in a position of more or equal significance compared to this bit must be 0
        // return false if any block is invalid (has higher bits not equal to 0)
        !self
            .buffer
            .iter()
            .enumerate()
            // only look at more significant blocks
            .skip(block_index)
            .any(|(index, block)| {
                // if block is 0, then this block is correct
                // we short circuit with true when the block is incorrect
                (if index == block_index {
                    // if we are at the block containing the bit, then take the block right shifted by inner_index
                    // if the inner_index is 1 (corresponding to first power of 2 = 0b10),
                    // then the block is invalid if anything past the first bit is set (0b10, 0b11, 0b100, 0b110, etc.)
                    block >> (inner_index)
                } else {
                    *block
                }) != 0
            })
    }

    /// Add a power of two to the bignum.
    ///
    /// # Examples
    ///
    /// ```
    /// # use inc_exc::counters::{bignum::BigCounter, Counter};
    /// let mut counter = BigCounter::new(0);
    /// // adding a large power of two
    /// counter.add(128);
    /// assert!(counter.equal(128));
    /// ```
    fn add(&mut self, power_of_two: u32) {
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);

        match self.sign {
            Sign::Positive => {
                self.cascade_up(block_index, inner_index);
            }
            Sign::Negative => {
                self.cascade_down(block_index, inner_index);
            }
        }
    }

    /// Subtracts a power of two from the bignum.
    ///
    /// # Examples
    ///
    /// ```
    /// # use inc_exc::counters::{bignum::BigCounter, Counter};
    /// let mut counter = BigCounter::new(0);
    /// // subtracting a large power of two
    /// counter.sub(128);
    /// counter.add(127);
    /// counter.add(126);
    /// counter.add(126);
    /// counter.add(1);
    /// assert!(counter.equal(1));
    /// ```
    fn sub(&mut self, power_of_two: u32) {
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);
        match self.sign {
            Sign::Positive => {
                self.cascade_down(block_index, inner_index);
            }
            Sign::Negative => {
                self.cascade_up(block_index, inner_index);
            }
        }
    }
}

impl BigCounter {
    fn cascade_up(&mut self, block_index: usize, inner_index: usize) {
        // if the storage is too small, right pad the buffer with 0's till we reach desired length and then set the necessary bit
        if block_index >= self.buffer.len() {
            // length will be block_index + 1 because 0-indexing
            self.buffer.resize(block_index + 1, 0);
            self.buffer[block_index] ^= 1 << inner_index;
        } else {
            // if the last element is not 0
            if let Some(true) = self.buffer.last().map(|e| *e != 0) {
                // add a 0
                // so we can avoid bounds checking later
                self.buffer.push(0);
            }
            // do a wrapping add on the block, and if overflow, cascade upwards
            let mut index = block_index;
            let mut carry;
            (self.buffer[index], carry) = self.buffer[index].overflowing_add(1 << inner_index);
            // carry is guaranteed to be false before index overflows because
            // we added the extra 0 element if the last was not already 0
            while carry {
                index += 1;
                // not at the end, directly add to the number and check for overflow
                (self.buffer[index], carry) = self.buffer[index].overflowing_add(1);
            }
        }
    }

    fn cascade_down(&mut self, block_index: usize, inner_index: usize) {
        // if the storage is too small, right pad the buffer with 0's till we reach desired length, then flip all previous bits and add one
        if block_index >= self.buffer.len() {
            // length will be block_index + 1 because 0-indexing
            self.buffer.resize(block_index + 1, 0);
            self.buffer[block_index] = (1_u64 << inner_index) - 1;
            // dont modify the last block
            for index in 0..block_index {
                self.buffer[index] ^= 0xFFFF_FFFF_FFFF_FFFF;
            }
            // add one
            self.cascade_up(0, 0);
            // negate sign
            self.sign = !self.sign;
        } else {
            // subtract from the starting block, and if overflow, subtract 1 from the next most significant block
            // keep going until reach last block. if overflow, then flip all bits in all the blocks from 0..=last_block, add one, and flip sign
            let mut index = block_index;
            let mut carry;
            (self.buffer[index], carry) = self.buffer[index].overflowing_sub(1 << inner_index);
            while carry {
                index += 1;
                // reached highest possible block and had overflow
                if index == self.buffer.len() {
                    // flip every bit in the blocks from 0..=last_block
                    self.buffer
                        .iter_mut()
                        .for_each(|block| *block ^= 0xFFFF_FFFF_FFFF_FFFF);
                    // add one
                    self.cascade_up(0, 0);
                    // negate sign
                    self.sign = !self.sign;
                    break;
                }
                // subtract one from the next block
                (self.buffer[index], carry) = self.buffer[index].overflowing_sub(1);
            }
        }
    }
    fn get_block_and_inner(power_of_two: usize) -> (usize, usize) {
        (
            power_of_two as usize / (size_of::<u64>() * 8),
            power_of_two as usize % (size_of::<u64>() * 8),
        )
    }
}
