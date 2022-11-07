# Cargo

[Install Cargo](https://doc.rust-lang.org/cargo/getting-started/installation.html")

# Running
To run the main function in [main.rs](./src/main.rs), run `cargo run`. The main function runs the correctness checks under correctness_samples. 

# Benchmarking
To run the benchmarks, run `cargo bench --bench n_taut -- --warm-up-time 1`. This reduces the warm up to 1 second. Opening the `target/criterion/report/index.html` file will show all the results of the benchmark in graphs.


