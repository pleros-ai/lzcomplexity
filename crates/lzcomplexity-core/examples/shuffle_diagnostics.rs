//! Inspect the existing shuffle measure without changing its implementation.
//!
//! Build: cargo build --release -p lzcomplexity-core --example shuffle_diagnostics
//! Usage: shuffle_diagnostics INPUT [CAP=-1] [JOBS=1] [LOG_BASE=2] [--verify]
//! Input is raw bytes; output is one JSON object. `--verify` also checks that
//! the complete public pipeline agrees with reuse of the original factor count.

use std::error::Error;
use std::io;

use lzcomplexity_core::lz76::lz76_factorization;
use lzcomplexity_core::shuffle::{
    lz76_random_shuffle_complexity, shuffle_entropy_calculation, shuffle_factorization,
};
use lzcomplexity_core::{LzArgs, Sequence};

fn main() -> Result<(), Box<dyn Error>> {
    let mut argv: Vec<String> = std::env::args().skip(1).collect();
    let verify = argv.last().is_some_and(|arg| arg == "--verify");
    if verify {
        argv.pop();
    }
    if argv.is_empty() || argv.len() > 4 {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "usage: shuffle_diagnostics INPUT [CAP=-1] [JOBS=1] [LOG_BASE=2] [--verify]",
        )
        .into());
    }
    let data = std::fs::read(&argv[0])?;
    let cap: i32 = argv.get(1).map_or(Ok(-1), |arg| arg.parse())?;
    let jobs: usize = argv.get(2).map_or(Ok(1), |arg| arg.parse())?;
    let log_base: u32 = argv.get(3).map_or(Ok(2), |arg| arg.parse())?;
    if cap > 128 || jobs == 0 || log_base < 2 {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "diagnostic limits: cap <=128, jobs >=1, log_base >=2",
        )
        .into());
    }
    let pool = rayon::ThreadPoolBuilder::new().num_threads(jobs).build()?;
    pool.install(|| {
        let seq = Sequence::from_bytes(data);
        let mut args = LzArgs::new();
        args.log_base = log_base;
        args.block_size = cap;
        args.get_shuffle_terms = true;
        let complexity = lz76_factorization(&seq, &args);
        let (counts, maximum) = shuffle_factorization(&seq, &args);
        let result = shuffle_entropy_calculation(&seq, &args, complexity as i32, &counts, maximum);
        let g = if seq.len() > 1 {
            (seq.len() as f64).ln() / (log_base as f64).ln() / seq.len() as f64
        } else {
            0.0
        };
        let raw: Vec<f64> = (1..=maximum)
            .map(|size| size as f64 * (counts[size] as f64 * g - complexity as f64 * g))
            .collect();
        let direct_equals_reused = if verify {
            let direct = lz76_random_shuffle_complexity(&seq, &args);
            (direct.max_block_size == result.max_block_size
                && direct.emc_value.to_bits() == result.emc_value.to_bits()
                && direct.multi_information.to_bits() == result.multi_information.to_bits()
                && direct.summands == result.summands)
                .to_string()
        } else {
            "null".into()
        };
        // All fields are numbers or booleans; Rust's Debug vectors are valid
        // JSON for these finite quantities. No user text is interpolated.
        println!(
            concat!(
                "{{\"schema_version\":1,\"length\":{},\"alphabet\":{},",
                "\"complexity\":{},\"requested_cap\":{},\"max_block_size\":{},",
                "\"jobs\":{},\"log_base\":{},\"value\":{:?},",
                "\"summands\":{:?},\"multi_information\":{:?},",
                "\"raw_weighted_ladder\":{:?},\"surrogate_counts\":{:?},",
                "\"direct_equals_reused\":{}}}"
            ),
            seq.len(),
            seq.alphabet_size(),
            complexity,
            cap,
            maximum,
            jobs,
            log_base,
            result.emc_value,
            result.summands,
            result.multi_information,
            raw,
            &counts[1..=maximum],
            direct_equals_reused,
        );
    });
    Ok(())
}
