use nom::{
    branch::alt,
    bytes::complete::{tag, take_till},
    character::{
        complete::{digit1, newline, space1, line_ending},
        is_newline,
    },
    combinator::{map, opt, verify},
    multi::{many0, many1, separated_list1},
    sequence::{pair, terminated},
    IResult,
};

use crate::{
    dnf::{Sign, DNF},
    Merge,
};

/// 0 or more comment lines
fn front_comments(input: &str) -> IResult<&str, Vec<(&str, &str)>> {
    many0(pair(
        tag("c"),
        terminated(take_till(|c| is_newline(c as u8)), newline),
    ))(input)
}

// swallows any comments in the middle of the block of variables defining the clause
fn clause(input: &str) -> IResult<&str, Vec<(u32, Sign)>> {
    let (input, digits) = separated_list1(
        // separator is either space or a newline with possible comments
        alt((
            map(space1, |_| ()),
            map(pair(line_ending, front_comments), |_| ()),
        )),
        // nonzero numbers that may start with a "-"
        verify(pair(opt(tag("-")), digit1), |(_, d)| d != &"0"),
    )(input)?;
    // swallow the final " *0\n?" if it exists and the newline if it exists
    let (input, _) = opt(terminated(space1, pair(tag("0"), opt(line_ending))))(input)?;
    Ok((
        input,
        digits
            .iter()
            .map(|s: &(Option<&str>, &str)| {
                if let Some(_) = s.0 {
                    // negation. those with hyphens become positive.
                    // input is in CNF, so negating will form the DNF
                    (s.1.parse::<u32>().unwrap(), Sign::Positive)
                } else {
                    (s.1.parse::<u32>().unwrap(), Sign::Negative)
                }
            })
            .collect(),
    ))
}

// as specified at https://people.sc.fsu.edu/~jburkardt/data/cnf/cnf.html
// 102002525 lines loaded in 465.558930916s
pub fn parse_dimacs<T: Merge>(file_contents: &str) -> IResult<&str, (DNF<T>, u32, u32)> {
    let input = file_contents;
    let (input, _) = front_comments(input)?;
    let (input, _) = terminated(tag("p"), space1)(input)?;
    let (input, _) = terminated(tag("cnf"), space1)(input)?;
    let (input, num_vars) = terminated(digit1, space1)(input)?;
    let (input, num_clauses) = terminated(digit1, line_ending)(input)?;
    let (input, _) = front_comments(input)?;
    // the clauses in the input are in cnf form. therefore, we negate every value while parsing to obtain the dnf
    many1(terminated(clause, opt(front_comments)))(input).map(|out| {
        (
            out.0,
            (
                T::from_vec(out.1),
                num_vars.parse().expect("Failed to parse num vars"),
                num_clauses.parse().expect("Failed to parse num clauses"),
            ),
        )
    })
}
