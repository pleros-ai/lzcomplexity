//! Byte sequences, alphabet metadata and sequence transformations.

use rand::Rng;
use rand::SeedableRng;
use rand_chacha::ChaCha8Rng;
use std::sync::OnceLock;

use crate::ALPHABET_SIZE;

#[derive(Clone, Debug)]
struct AlphabetInfo {
    symbols: Vec<u8>,
    size: u32,
}

impl AlphabetInfo {
    fn from_bytes(data: &[u8], legacy_hint: Option<u32>) -> Self {
        let mut seen = [false; 256];
        for &symbol in data {
            seen[symbol as usize] = true;
        }
        let symbols: Vec<u8> = (0..=255u16)
            .rev()
            .filter(|&symbol| seen[symbol as usize])
            .map(|symbol| symbol as u8)
            .collect();
        let observed = symbols.len() as u32;
        // Preserve exact-match constructor hints, including
        // zero for empty data and one for a singleton alphabet.
        let size = if legacy_hint == Some(observed) {
            observed
        } else {
            observed.max(ALPHABET_SIZE)
        };
        Self { symbols, size }
    }
}

/// A symbolic sequence stored as raw bytes, with lazily cached alphabet data.
/// Symbol-changing mutations invalidate the observed alphabet. An explicit
/// size declaration remains in force until `determine_alphabet` or `clear`.
#[derive(Clone, Debug)]
pub struct Sequence {
    data: Vec<u8>,
    alphabet: OnceLock<AlphabetInfo>,
    declared_alphabet_size: Option<u32>,
}

impl Sequence {
    pub fn new() -> Self {
        Self {
            data: Vec::new(),
            alphabet: OnceLock::new(),
            declared_alphabet_size: None,
        }
    }

    pub fn with_capacity(cap: usize) -> Self {
        Self {
            data: Vec::with_capacity(cap),
            alphabet: OnceLock::new(),
            declared_alphabet_size: None,
        }
    }

    // Infallible UTF-8 constructor; this is an inherent method rather than FromStr.
    #[allow(clippy::should_implement_trait)]
    pub fn from_str(s: &str) -> Self {
        Self::from_bytes(s.as_bytes().to_vec())
    }

    pub fn from_bytes(b: Vec<u8>) -> Self {
        let alphabet = AlphabetInfo::from_bytes(&b, None);
        Self {
            data: b,
            alphabet: OnceLock::from(alphabet),
            declared_alphabet_size: None,
        }
    }

    /// Construct from bytes with an alphabet-size hint. The observed
    /// alphabet replaces a mismatching hint; use `set_alphabet_size` to declare
    /// a size that remains fixed through subsequent symbol changes.
    pub fn from_bytes_with_alphabet(b: Vec<u8>, alphabet_size: u32) -> Self {
        let alphabet = AlphabetInfo::from_bytes(&b, Some(alphabet_size));
        Self {
            data: b,
            alphabet: OnceLock::from(alphabet),
            declared_alphabet_size: None,
        }
    }

    /// Construct an empty sequence with an explicit alphabet-size declaration.
    pub fn with_alphabet_size(alphabet_size: u32) -> Self {
        Self {
            data: Vec::new(),
            alphabet: OnceLock::new(),
            declared_alphabet_size: Some(alphabet_size),
        }
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.data
    }

    /// Mutable bytes. Observed alphabet data is rebuilt lazily after the
    /// mutable borrow ends; an explicit alphabet-size declaration is retained.
    pub fn as_bytes_mut(&mut self) -> &mut [u8] {
        self.alphabet.take();
        &mut self.data
    }

    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(&self.data).into_owned()
    }

    pub fn alphabet(&self) -> &[u8] {
        &self
            .alphabet
            .get_or_init(|| AlphabetInfo::from_bytes(&self.data, None))
            .symbols
    }

    pub fn alphabet_size(&self) -> u32 {
        self.declared_alphabet_size.unwrap_or_else(|| {
            self.alphabet
                .get_or_init(|| AlphabetInfo::from_bytes(&self.data, None))
                .size
        })
    }

    /// Declare the size used by downstream normalization without changing the
    /// observed symbol list. `determine_alphabet` restores observed-size mode.
    pub fn set_alphabet_size(&mut self, s: u32) {
        self.declared_alphabet_size = Some(s);
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    /// Scan the data and rebuild the alphabet (sorted descending), replacing
    /// any explicit size declaration. An existing size matching a zero- or
    /// one-symbol alphabet is preserved.
    pub fn determine_alphabet(&mut self) -> Vec<u8> {
        let hint = self
            .declared_alphabet_size
            .or_else(|| self.alphabet.get().map(|cached| cached.size));
        let alphabet = AlphabetInfo::from_bytes(&self.data, hint);
        let symbols = alphabet.symbols.clone();
        self.alphabet = OnceLock::from(alphabet);
        self.declared_alphabet_size = None;
        symbols
    }

    pub fn push(&mut self, c: u8) -> usize {
        self.alphabet.take();
        self.data.push(c);
        self.data.len()
    }

    pub fn pop(&mut self) -> usize {
        if !self.data.is_empty() {
            self.alphabet.take();
            self.data.pop();
        }
        self.data.len()
    }

    pub fn clear(&mut self) {
        self.alphabet.take();
        self.declared_alphabet_size = None;
        self.data.clear();
    }

    pub fn get(&self, idx: usize) -> Option<u8> {
        self.data.get(idx).copied()
    }

    pub fn first(&self) -> Option<u8> {
        self.data.first().copied()
    }

    pub fn last(&self) -> Option<u8> {
        self.data.last().copied()
    }

    pub fn min_byte(&self) -> Option<u8> {
        self.data.iter().copied().min()
    }

    pub fn max_byte(&self) -> Option<u8> {
        self.data.iter().copied().max()
    }

    pub fn min_range(&self, start: usize, end: usize) -> Option<u8> {
        self.data
            .get(start..end)
            .and_then(|s| s.iter().copied().min())
    }

    pub fn max_range(&self, start: usize, end: usize) -> Option<u8> {
        self.data
            .get(start..end)
            .and_then(|s| s.iter().copied().max())
    }

    /// Copy the first `l` bytes, clamping the length to the available data.
    pub fn take(&self, l: usize) -> Self {
        let n = l.min(self.data.len());
        Sequence::from_bytes_with_alphabet(self.data[..n].to_vec(), self.alphabet_size())
    }

    /// Copy the sequence after skipping its first `l` bytes.
    pub fn drop(&self, l: usize) -> Self {
        if l >= self.data.len() {
            return Self {
                data: Vec::new(),
                // Preserve the empty-result alphabet size without treating an
                // inherited auto size for an explicit user declaration.
                alphabet: OnceLock::from(AlphabetInfo {
                    symbols: Vec::new(),
                    size: self.alphabet_size(),
                }),
                declared_alphabet_size: self.declared_alphabet_size,
            };
        }
        Sequence::from_bytes_with_alphabet(self.data[l..].to_vec(), self.alphabet_size())
    }

    /// Copy the prefix and suffix at byte position `l`, clamped to the length.
    pub fn split_at(&self, l: usize) -> (Self, Self) {
        let n = l.min(self.data.len());
        (
            Sequence::from_bytes_with_alphabet(self.data[..n].to_vec(), self.alphabet_size()),
            Sequence::from_bytes_with_alphabet(self.data[n..].to_vec(), self.alphabet_size()),
        )
    }

    /// Sum each complete group of `gr` bytes with wrapping byte arithmetic.
    /// Discard an incomplete trailing group; zero group size returns empty.
    pub fn granularity(&self, gr: u32) -> Self {
        if gr == 0 {
            return Self::new();
        }
        let mut buf = Vec::with_capacity(self.data.len() / gr as usize);
        let mut seen = [false; 256];
        let mut temp: u8 = 0;
        let mut count: u32 = 0;
        for &c in &self.data {
            temp = temp.wrapping_add(c);
            count += 1;
            if count == gr {
                buf.push(temp);
                seen[temp as usize] = true;
                temp = 0;
                count = 0;
            }
        }
        let unique = seen.iter().filter(|b| **b).count() as u32;
        Sequence::from_bytes_with_alphabet(buf, unique)
    }

    pub fn reverse(&mut self) -> &mut Self {
        self.data.reverse();
        self
    }

    pub fn reverse_copy(&self) -> Self {
        let mut s = self.clone();
        s.data.reverse();
        s
    }

    pub fn pi(&mut self) -> &mut Self {
        self.pop();
        self
    }

    pub fn right_shift(&mut self, ls: u32) -> &mut Self {
        if self.data.is_empty() {
            return self;
        }
        let shift = (ls as usize) % self.data.len();
        if shift == 0 {
            return self;
        }
        // The named right shift moves the byte at `shift` to the beginning.
        self.data.rotate_left(shift);
        self
    }

    pub fn left_shift(&mut self, ls: u32) -> &mut Self {
        if self.data.is_empty() {
            return self;
        }
        let n = self.data.len();
        let shift = n - ((ls as usize) % n);
        if shift == n {
            return self;
        }
        self.data.rotate_left(shift);
        self
    }

    /// Count occurrences of each byte in sorted byte order.
    pub fn char_density(&self) -> std::collections::BTreeMap<u8, u32> {
        let mut m = std::collections::BTreeMap::new();
        for &c in &self.data {
            *m.entry(c).or_insert(0u32) += 1;
        }
        m
    }

    /// Return a new sequence with `f` applied to each byte.
    pub fn map<F: Fn(u8) -> u8>(&self, f: F) -> Self {
        let mut data = Vec::with_capacity(self.data.len());
        for &c in &self.data {
            data.push(f(c));
        }
        Self {
            data,
            alphabet: OnceLock::new(),
            declared_alphabet_size: self.declared_alphabet_size,
        }
    }

    pub fn extend_from_slice(&mut self, other: &[u8]) {
        if !other.is_empty() {
            self.alphabet.take();
        }
        self.data.extend_from_slice(other);
    }

    /// In-place XOR: 0 where bytes match, 1 otherwise.
    pub fn xor_with(&mut self, other: &Sequence) -> Result<(), &'static str> {
        if self.data.len() != other.data.len() {
            return Err("SequenceNoMatchSize");
        }
        if !self.data.is_empty() {
            self.alphabet.take();
        }
        for i in 0..self.data.len() {
            self.data[i] = if self.data[i] == other.data[i] { 0 } else { 1 };
        }
        Ok(())
    }
}

impl Default for Sequence {
    fn default() -> Self {
        Self::new()
    }
}

impl std::cmp::PartialEq for Sequence {
    fn eq(&self, other: &Self) -> bool {
        self.data == other.data
    }
}

impl std::cmp::Eq for Sequence {}

impl std::cmp::PartialOrd for Sequence {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl std::cmp::Ord for Sequence {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.data.cmp(&other.data)
    }
}

impl std::ops::Add for &Sequence {
    type Output = Sequence;
    fn add(self, rhs: &Sequence) -> Sequence {
        let mut data = Vec::with_capacity(self.data.len() + rhs.data.len());
        data.extend_from_slice(&self.data);
        data.extend_from_slice(&rhs.data);
        Sequence {
            data,
            alphabet: OnceLock::new(),
            declared_alphabet_size: if self.declared_alphabet_size.is_some()
                || rhs.declared_alphabet_size.is_some()
            {
                Some(self.alphabet_size().max(rhs.alphabet_size()))
            } else {
                None
            },
        }
    }
}

/// In-place block shuffle of `s`. Picks two non-overlapping aligned blocks of
/// `block_size` and swaps them. Deterministic when called via [`shuffle_copy_seeded`].
/// The final suffix outside the eligible aligned blocks stays fixed.
pub fn shuffle_in_place(s: &mut Sequence, block_size: u32, rng: &mut ChaCha8Rng) {
    let seq_size = s.len();
    let bs = block_size as usize;
    if bs == 0 || seq_size <= bs + 1 {
        return;
    }
    let max_block_idx = (seq_size - bs - 1) / bs;
    if max_block_idx == 0 {
        return;
    }
    // Find first valid block
    let mut op1 = bs * rng.gen_range(0..=max_block_idx);
    while op1 + bs > seq_size.saturating_sub(1) {
        op1 = bs * rng.gen_range(0..=max_block_idx);
    }
    if seq_size <= 10 {
        return;
    }
    // Find second non-overlapping block
    loop {
        let op2 = bs * rng.gen_range(0..=max_block_idx);
        let no_overlap = op2 + bs <= op1 || op2 >= op1 + bs;
        let in_bounds = op2 + bs <= seq_size.saturating_sub(1);
        if no_overlap && in_bounds {
            // swap blocks [op1..op1+bs] and [op2..op2+bs]
            for i in 0..bs {
                s.data.swap(op1 + i, op2 + i);
            }
            return;
        }
    }
}

/// Returns a shuffled copy after `times` iterations, with a deterministic seed.
pub fn shuffle_copy(s: &Sequence, block_size: u32, times: u32) -> Sequence {
    let mut out = s.clone();
    let mut rng = ChaCha8Rng::seed_from_u64(deterministic_seed(s, block_size));
    for _ in 0..times {
        shuffle_in_place(&mut out, block_size, &mut rng);
    }
    out
}

/// Same as [`shuffle_copy`] but with an explicit seed, so callers (LZ76 random
/// shuffle complexity) can vary the seed per inner block size.
pub fn shuffle_copy_seeded(s: &Sequence, block_size: u32, times: u32, seed: u64) -> Sequence {
    let mut out = s.clone();
    let mut rng = ChaCha8Rng::seed_from_u64(seed);
    for _ in 0..times {
        shuffle_in_place(&mut out, block_size, &mut rng);
    }
    out
}

fn deterministic_seed(s: &Sequence, block_size: u32) -> u64 {
    let mut hash: u64 = 5381;
    for &c in &s.data {
        hash = hash.wrapping_mul(33).wrapping_add(c as u64);
    }
    hash ^ (block_size as u64).wrapping_mul(0x9E3779B97F4A7C15)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn from_str_and_take() {
        let s = Sequence::from_str("hello");
        assert_eq!(s.len(), 5);
        assert_eq!(&s.take(3).data[..], b"hel");
    }

    #[test]
    fn split_drop_takes_match() {
        let s = Sequence::from_str("ABRACADABRA");
        let (l, r) = s.split_at(5);
        assert_eq!(&l.data[..], b"ABRAC");
        assert_eq!(&r.data[..], b"ADABRA");
        assert_eq!(&s.drop(5).data[..], b"ADABRA");
        assert_eq!(&s.take(5).data[..], b"ABRAC");
    }

    #[test]
    fn shifts() {
        let mut s = Sequence::from_str("ABCDE");
        s.right_shift(2);
        assert_eq!(&s.data[..], b"CDEAB");
        let mut s = Sequence::from_str("ABCDE");
        s.left_shift(1);
        assert_eq!(&s.data[..], b"EABCD");
    }

    #[test]
    fn alphabet_min_two() {
        let s = Sequence::from_str("aaaa");
        assert_eq!(s.alphabet_size(), 2);
    }

    #[test]
    fn shuffle_deterministic() {
        let s = Sequence::from_str(&"01".repeat(200));
        let a = shuffle_copy_seeded(&s, 3, 5, 42);
        let b = shuffle_copy_seeded(&s, 3, 5, 42);
        assert_eq!(a.data, b.data);
    }

    fn assert_observed_alphabet_matches_fresh(seq: &Sequence) {
        let fresh = Sequence::from_bytes(seq.as_bytes().to_vec());
        assert_eq!(seq.alphabet(), fresh.alphabet());
        assert_eq!(seq.alphabet_size(), fresh.alphabet_size());
    }

    #[test]
    fn legacy_constructor_hints_keep_existing_metadata() {
        for data in [Vec::new(), vec![7; 4], vec![0, 1], (0..=255).collect()] {
            let observed = data
                .iter()
                .copied()
                .collect::<std::collections::BTreeSet<_>>()
                .len() as u32;
            let fresh = Sequence::from_bytes(data.clone());
            assert_eq!(fresh.alphabet_size(), observed.max(2));
            for hint in [0, 1, 2, 4, 20, 256] {
                let mut seq = Sequence::from_bytes_with_alphabet(data.clone(), hint);
                let expected = if observed == hint {
                    observed
                } else {
                    observed.max(2)
                };
                assert_eq!(seq.alphabet_size(), expected);
                assert_eq!(seq.alphabet(), fresh.alphabet());
                assert_eq!(seq.clone().alphabet_size(), expected);
                seq.determine_alphabet();
                assert_eq!(seq.alphabet_size(), expected);
            }
        }
    }

    #[test]
    fn symbol_mutations_invalidate_without_rescanning_each_push() {
        let mut seq = Sequence::with_capacity(256);
        assert_eq!(seq.alphabet(), &[]);
        for symbol in 0..=255 {
            seq.push(symbol);
            // Metadata stays uninitialized throughout a bulk build: pushing
            // does not repeatedly scan the growing data buffer.
            assert!(seq.alphabet.get().is_none());
        }
        assert_observed_alphabet_matches_fresh(&seq);
        assert_eq!(seq.alphabet_size(), 256);
        seq.pop();
        assert_observed_alphabet_matches_fresh(&seq);
        assert_eq!(seq.alphabet_size(), 255);
        seq.pi();
        assert_observed_alphabet_matches_fresh(&seq);
        seq.extend_from_slice(&[254, 255]);
        assert_observed_alphabet_matches_fresh(&seq);
        let clone = seq.clone();
        seq.as_bytes_mut().fill(0);
        assert_observed_alphabet_matches_fresh(&seq);
        assert_eq!(seq.alphabet(), &[0]);
        assert_eq!(clone.alphabet_size(), 256);
        seq.clear();
        assert_observed_alphabet_matches_fresh(&seq);
        assert_eq!(seq.alphabet(), &[]);
        assert_eq!(seq.pop(), 0);
    }

    #[test]
    fn transforms_rebuild_auto_alphabets_and_preserve_declarations() {
        let source = Sequence::from_bytes((0..=255).collect());
        let mapped = source.map(|symbol| symbol % 2);
        assert_observed_alphabet_matches_fresh(&mapped);
        assert_eq!(mapped.alphabet(), &[1, 0]);

        let left = Sequence::from_str("AB");
        let right = Sequence::from_str("CD");
        let combined = &left + &right;
        assert_observed_alphabet_matches_fresh(&combined);
        assert_eq!(combined.alphabet_size(), 4);

        let mut emptied = combined.drop(combined.len());
        assert_eq!(emptied.alphabet_size(), 4); // Legacy initial empty size.
        emptied.extend_from_slice(b"XY");
        assert_observed_alphabet_matches_fresh(&emptied);
        assert_eq!(emptied.alphabet_size(), 2);

        let mut xor = Sequence::from_bytes(vec![0, 1, 2, 3]);
        assert_eq!(xor.alphabet_size(), 4);
        xor.xor_with(&Sequence::from_bytes(vec![0, 0, 2, 0]))
            .unwrap();
        assert_eq!(xor.as_bytes(), &[0, 1, 0, 1]);
        assert_observed_alphabet_matches_fresh(&xor);

        let mut declared = Sequence::with_alphabet_size(20);
        declared.extend_from_slice(b"AB");
        assert_eq!(declared.alphabet_size(), 20);
        assert_eq!(declared.alphabet(), b"BA");
        let mapped = declared.map(|_| 0);
        assert_eq!(mapped.alphabet_size(), 20);
        assert_eq!(mapped.alphabet(), &[0]);
        let combined = &declared + &right;
        assert_eq!(combined.alphabet_size(), 20);
        assert_eq!(combined.alphabet(), b"DCBA");
        let mut emptied = declared.drop(declared.len());
        emptied.extend_from_slice(b"XY");
        assert_eq!(emptied.alphabet_size(), 20);
        assert_eq!(emptied.alphabet(), b"YX");
    }

    #[test]
    fn explicit_size_survives_mutation_until_recomputed() {
        let mut seq = Sequence::from_str("ACGT");
        seq.set_alphabet_size(20);
        seq.push(b'N');
        assert_eq!(seq.alphabet_size(), 20);
        assert_eq!(seq.alphabet(), b"TNGCA");
        let mut clone = seq.clone();
        clone.as_bytes_mut().fill(b'A');
        assert_eq!(clone.alphabet_size(), 20);
        assert_eq!(clone.alphabet(), b"A");
        assert_eq!(seq.alphabet(), b"TNGCA");
        clone.determine_alphabet();
        assert_eq!(clone.alphabet_size(), 2);
        clone.extend_from_slice(b"BC");
        assert_eq!(clone.alphabet_size(), 3);
        seq.clear();
        assert_eq!(seq.alphabet_size(), 2);
        assert!(seq.alphabet().is_empty());
    }

    #[test]
    fn permutation_operations_keep_the_observed_cache() {
        let mut seq = Sequence::from_str("ABCDABCDABCDABCD");
        seq.reverse().left_shift(3).right_shift(2);
        assert!(seq.alphabet.get().is_some());
        assert_eq!(seq.alphabet(), b"DCBA");
        let shuffled = shuffle_copy_seeded(&seq, 2, 20, 42);
        assert!(shuffled.alphabet.get().is_some());
        assert_observed_alphabet_matches_fresh(&shuffled);
    }

    #[test]
    fn mutated_and_fresh_data_have_identical_h_and_shuffle_values() {
        use crate::lz76::{lz76_entropy_density, LzArgs};
        use crate::shuffle::lz76_random_shuffle_complexity;

        fn check(seq: &Sequence) {
            let fresh = Sequence::from_bytes(seq.as_bytes().to_vec());
            let mut args = LzArgs::new();
            args.block_size = 8;
            args.get_emc_terms = true;
            assert_eq!(
                lz76_entropy_density(seq, &args).to_bits(),
                lz76_entropy_density(&fresh, &args).to_bits()
            );
            let actual = lz76_random_shuffle_complexity(seq, &args);
            let expected = lz76_random_shuffle_complexity(&fresh, &args);
            assert_eq!(actual.emc_value.to_bits(), expected.emc_value.to_bits());
            assert_eq!(actual.summands, expected.summands);
            assert_eq!(
                actual.multi_information.to_bits(),
                expected.multi_information.to_bits()
            );
        }

        let data: Vec<u8> = b"ACGT".iter().copied().cycle().take(2048).collect();
        let mut pushed = Sequence::new();
        for &symbol in &data {
            pushed.push(symbol);
        }
        check(&pushed);
        let mut extended = Sequence::new();
        extended.extend_from_slice(&data);
        check(&extended);
        let mut borrowed = Sequence::from_bytes(vec![0; data.len()]);
        borrowed.as_bytes_mut().copy_from_slice(&data);
        check(&borrowed);
        check(
            &(&Sequence::from_bytes(data[..1024].to_vec())
                + &Sequence::from_bytes(data[1024..].to_vec())),
        );
        check(&Sequence::from_bytes(data).map(|symbol| symbol % 3));

        fn assert_send_sync_clone<T: Send + Sync + Clone>() {}
        assert_send_sync_clone::<Sequence>();
    }
}
