fn gen_subsets(clause_count: usize, size: usize) -> Vec<Vec<usize>> {
    let mut out: Vec<Vec<usize>> = Vec::new();
    let mut stack: Vec<Vec<usize>> = vec![vec![]];
    while let Some(set) = stack.pop() {
        if set.len() == size {
            out.push(set);
        } else {
            // extend by another index
            // this new index must be greater than the last index in the set
            for new_index in (set.last().map(|&e| e + 1).unwrap_or(0))..(clause_count) {
                let mut new_set = set.clone();
                new_set.push(new_index);
                stack.push(new_set);
            }
        }
    }
    out
}
