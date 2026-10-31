//! lzcomplexity-core — LZ76 factorization and related complexity measures.
//!
//! Algorithms for entropy density h, effective measure complexity E and information
//! distance, operating directly on byte sequences.

pub mod block_entropy;
pub mod emc;
pub mod lpf;
pub mod lz76;
pub mod metrics;
pub mod sequence;
pub mod shuffle;
pub mod suffix_array;

pub use emc::EmcMethod;
pub use lz76::{EmcResult, LempelZiv, Lz76Result, LzArgs, LzExtra};
pub use sequence::Sequence;
pub use suffix_array::{build_suffix_array, SuffixArray};

/// Sentinel value used in `LzArgs` to indicate "auto-detect from sequence".
pub const NO_ALPHABET: u32 = u32::MAX;

/// Minimum alphabet size used by automatic normalization.
pub const ALPHABET_SIZE: u32 = 2;
