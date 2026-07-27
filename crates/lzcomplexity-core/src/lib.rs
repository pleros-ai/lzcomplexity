//! lzcomplexity-core — LZ76 factorization and related complexity measures.
//!
//! This crate is a Rust port of the C++ `lzcomplexity` library. It contains
//! the algorithm implementations, independent of any Python bindings.

mod block_entropy;
pub mod lpf;
pub mod lz76;
pub mod metrics;
pub mod sequence;
pub mod shuffle;
pub mod suffix_array;

pub use lz76::{LempelZiv, Lz76Result, LzArgs, LzExtra, LzShuffle};
pub use sequence::Sequence;
pub use suffix_array::{build_suffix_array, SuffixArray};

/// Sentinel value used in `LzArgs` to indicate "auto-detect from sequence".
pub const NO_ALPHABET: u32 = u32::MAX;

/// Default minimum alphabet size, mirroring `details::ALPHABET_SIZE` from the C++.
pub const ALPHABET_SIZE: u32 = 2;
