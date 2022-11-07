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

    fn equal(&self, power_of_two: u32) -> bool {
        if self.sign == Sign::Negative {
            return false;
        }
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);
        // everything must be 0, except for that bit
        // so we check if there are any nonzero values not matching block_index and inner_index
        !self.buffer.iter().enumerate().any(|(index, block)| {
            // nonzero block
            *block != 0 &&
            // that does not have matching index or does not match inner index
            !index == block_index
                || !(block ^ (1 << inner_index) == 0)
        })
    }

    fn less_than(&self, power_of_two: u32) -> bool {
        if self.sign == Sign::Negative {
            return true;
        }
        let (block_index, inner_index) = Self::get_block_and_inner(power_of_two as usize);
        // for the counter to be less, every bit in a position of more or equal significance compared to this bit must be 0
        // return false if any block is invalid (has higher bits not equal to 0)
        !self
            .buffer
            // only look at more significant blocks
            .split_at(block_index)
            .1
            .iter()
            .enumerate()
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

    fn subtract(&mut self, power_of_two: u32) {
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
        // if the storage is too small, right pad with 0's till we reach desired length and then set the necessary bit
        if block_index >= self.buffer.len() {
            // length will be block_index + 1 because 0-indexing
            self.buffer.resize(block_index + 1, 0);
            self.buffer[block_index] ^= 1 << inner_index;
        } else {
            // otherwise, do a wrapping add on the block, and if overflow, cascade upwards
            let mut index = block_index;
            let mut carry;
            (self.buffer[index], carry) = self.buffer[index].overflowing_add(1 << inner_index);
            while carry {
                index += 1;
                // reached end of number with a carry,
                if index == self.buffer.len() {
                    // add a single 0000...0001 to the number in the most significant position
                    self.buffer.push(1);
                    break;
                }
                // not at the end, directly add to the number and check for overflow
                (self.buffer[index], carry) = self.buffer[index].overflowing_add(1);
            }
        }
    }

    fn cascade_down(&mut self, block_index: usize, inner_index: usize) {
        // subtract from the starting block, and if overflow, subtract 1 from the next block
        // keep going until reach last block. if overflow, then flip all bits in all the blocks from 0..=last_block, add one, and flip sign
        let mut index = block_index;
        let mut carry;
        (self.buffer[index], carry) = self.buffer[index].overflowing_sub(1 << inner_index);
        while carry {
            index += 1;
            // reached highest possible block and had overflow
            if index == self.buffer.len() && carry {
                // flip every bit in the blocks from 0..=last_block
                self.buffer
                    .iter_mut()
                    .for_each(|block| *block ^= 0xFFFF_FFFF_FFFF_FFFF);
                // add one (two's complement)
                self.cascade_up(0, 0);
                // negate sign
                self.sign = !self.sign;
                break;
            }
            // subtract one from the next block
            (self.buffer[index], carry) = self.buffer[index].overflowing_sub(1);
        }
    }
    fn get_block_and_inner(power_of_two: usize) -> (usize, usize) {
        (
            power_of_two as usize / (size_of::<u64>() * 8),
            power_of_two as usize % (size_of::<u64>() * 8),
        )
    }
}
