//! LZ76 factorization and the user-facing result containers.

use crate::lpf::lpf;
use crate::sequence::Sequence;
use crate::suffix_array::build as build_sa_lcp;
use crate::NO_ALPHABET;

/// Mirror of `LZ_Args` in the C++ — configuration knobs for all `lz76*`
/// functions. Defaults match the C++ no-arg constructor (which uses the
/// `NO_ALPHABET` sentinel for auto-detection).
#[derive(Clone, Debug)]
pub struct LzArgs {
    pub chunks: i32,
    pub max_context: i32,
    pub block_size: i32,
    pub get_shuffle_terms: bool,
    pub alphabet: u32,
    pub log_base: u32,
}

impl LzArgs {
    pub fn new() -> Self {
        Self {
            chunks: 0,
            max_context: 0,
            block_size: -1,
            get_shuffle_terms: false,
            alphabet: NO_ALPHABET,
            log_base: NO_ALPHABET,
        }
    }
}

impl Default for LzArgs {
    fn default() -> Self {
        Self::new()
    }
}

impl PartialEq for LzArgs {
    fn eq(&self, other: &Self) -> bool {
        self.chunks == other.chunks
            && self.max_context == other.max_context
            && self.block_size == other.block_size
            && self.get_shuffle_terms == other.get_shuffle_terms
            && self.alphabet == other.alphabet
    }
}

/// Mirror of `LZ_Result`.
#[derive(Clone, Debug, Default)]
pub struct Lz76Result {
    pub factorization: u32,
    pub epsilon: f64,
    pub lzf: Vec<u32>,
}

/// Mirror of `LZ_Shuffle`.
#[derive(Clone, Debug)]
pub struct LzShuffle {
    pub max_block_size: i32,
    pub emc_value: f64,
    pub multi_information: f64,
    pub summands: Vec<f64>,
}

impl LzShuffle {
    pub fn new(max_block_size: i32, emc_value: f64, multi_information: f64) -> Self {
        Self {
            max_block_size,
            emc_value,
            multi_information,
            summands: Vec::new(),
        }
    }
}

impl Default for LzShuffle {
    fn default() -> Self {
        Self::new(-1, 0.0, 0.0)
    }
}

/// Mirror of `LZ_Extra`.
#[derive(Clone, Debug, Default)]
pub struct LzExtra {
    pub lz_rajski_distance: f64,
    pub redundancy: f64,
    pub fh_uncertainty: f64,
    pub lh_uncertainty: f64,
    pub lz_pearson_coefficient: f64,
}

/// Mirror of `LempelZiv` — the rich result returned by `lz76(seq, args)`.
#[derive(Clone, Debug, Default)]
pub struct LempelZiv {
    pub complexity: u32,
    pub entropy_density: f64,
    pub random_shuffle_complexity: LzShuffle,
    pub paired_shuffle_complexity: LzShuffle,
    pub lz_normal_errors: f64,
    pub lz_poison_errors: f64,
    pub epsilon: f64,
    pub factors_stddev: f64,
    pub factors: Vec<u32>,
    pub extras: LzExtra,
}

/// Core: factorize a sequence into LZ76 factors. Mirrors `LempelZiv76::Factorize`.
///
/// Returns `(factorization_count, epsilon, factor_boundaries, factor_length_stddev)`.
pub fn factorize(seq: &Sequence, args: &LzArgs) -> (u32, f64, Vec<u32>, f64) {
    let n = seq.len();
    if n == 0 {
        return (0, 0.0, Vec::new(), 0.0);
    }

    let sa = build_sa_lcp(seq.as_bytes());
    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size()
    } else {
        args.log_base
    };
    let alphabet = if args.alphabet == NO_ALPHABET {
        seq.alphabet_size()
    } else {
        args.alphabet
    };

    let alphabet_f = alphabet.max(2) as f64;
    let log_base_f = log_base.max(2) as f64;
    let logn = log_base_f.ln();
    let n_f = n as f64;
    let epsilon = 2.0
        * (1.0 + ((alphabet_f * n_f).ln() / logn).ln() / logn)
        / (n_f.ln() / logn);

    let mut lpf_arr = vec![0u32; n];
    lpf(&mut lpf_arr, sa.sa.clone(), sa.lcp.clone(), n);

    let mut lzf: Vec<u32> = Vec::with_capacity(n);
    lzf.push(0);
    lzf.push(1);
    let mut i: usize = 1;
    while i < n {
        let last = *lzf.last().unwrap() as usize;
        let advance = lpf_arr.get(i).copied().unwrap_or(0) as usize;
        i = last + advance + 1;
        lzf.push(i as u32);
    }

    let factorization = if (*lzf.last().unwrap() as usize) <= n {
        (lzf.len() - 1) as u32
    } else {
        (lzf.len() - 2) as u32
    };

    let stddev = factors_stddev(&lzf);
    (factorization, epsilon, lzf, stddev)
}

fn factors_stddev(lzf: &[u32]) -> f64 {
    if lzf.len() < 2 {
        return 0.0;
    }
    let mut lens: Vec<u32> = Vec::with_capacity(lzf.len());
    let mut max_factor_size = 0u32;
    for i in 1..lzf.len() {
        let sz = lzf[i].saturating_sub(lzf[i - 1]);
        lens.push(sz);
        if sz > max_factor_size {
            max_factor_size = sz;
        }
    }
    let last = lzf[lzf.len() - 1] as f64;
    let mean = (last - 1.0) / lzf.len() as f64;
    let mut sq_sum = 0.0;
    for &l in &lens {
        let diff = l as f64 - mean;
        sq_sum += diff * diff;
    }
    if max_factor_size == 0 {
        0.0
    } else {
        (sq_sum / max_factor_size as f64).sqrt()
    }
}

/// True iff the sequence has at least 2 distinct symbols.
fn has_multiple_symbols(seq: &Sequence) -> bool {
    seq.char_density().len() > 1
}

/// Compute just the LZ76 complexity (factor count).
pub fn lz76_factorization(seq: &Sequence, args: &LzArgs) -> u32 {
    if !has_multiple_symbols(seq) {
        return 1;
    }
    let (c, _, _, _) = factorize(seq, args);
    c
}

/// Compute LZ76 complexity + factor boundaries.
pub fn lz76_factors(seq: &Sequence, args: &LzArgs) -> Lz76Result {
    if !has_multiple_symbols(seq) {
        return Lz76Result {
            factorization: 1,
            epsilon: 0.0,
            lzf: vec![0, 1, seq.len() as u32],
        };
    }
    let (factorization, epsilon, lzf, _) = factorize(seq, args);
    Lz76Result {
        factorization,
        epsilon,
        lzf,
    }
}

/// Compute the normalized LZ76 entropy density.
pub fn lz76_entropy_density(seq: &Sequence, args: &LzArgs) -> f64 {
    let factorization = lz76_factorization(seq, args) as f64;
    let log_base = if args.log_base == NO_ALPHABET {
        seq.alphabet_size().max(2)
    } else {
        args.log_base.max(2)
    } as f64;
    let n = seq.len() as f64;
    if n <= 1.0 {
        return 0.0;
    }
    let div = n / (n.ln() / log_base.ln());
    factorization / div
}

