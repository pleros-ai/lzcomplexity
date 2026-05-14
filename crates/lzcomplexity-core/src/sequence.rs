//! Byte-sequence type mirroring the C++ `lz::sequence` class.

use rand::SeedableRng;
use rand::Rng;
use rand_chacha::ChaCha8Rng;

use crate::ALPHABET_SIZE;

/// A symbolic sequence stored as raw bytes, plus a cached alphabet.
#[derive(Clone, Debug)]
pub struct Sequence {
    pub(crate) data: Vec<u8>,
    pub(crate) alphabet: Vec<u8>,
    pub(crate) alphabet_size: u32,
}

impl Sequence {
    pub fn new() -> Self {
        Self {
            data: Vec::new(),
            alphabet: Vec::new(),
            alphabet_size: ALPHABET_SIZE,
        }
    }

    pub fn with_capacity(cap: usize) -> Self {
        Self {
            data: Vec::with_capacity(cap),
            alphabet: Vec::new(),
            alphabet_size: ALPHABET_SIZE,
        }
    }

    pub fn from_str(s: &str) -> Self {
        let mut seq = Self {
            data: s.as_bytes().to_vec(),
            alphabet: Vec::new(),
            alphabet_size: ALPHABET_SIZE,
        };
        seq.determine_alphabet();
        seq
    }

    pub fn from_bytes(b: Vec<u8>) -> Self {
        let mut seq = Self {
            data: b,
            alphabet: Vec::new(),
            alphabet_size: ALPHABET_SIZE,
        };
        seq.determine_alphabet();
        seq
    }

    pub fn from_bytes_with_alphabet(b: Vec<u8>, alphabet_size: u32) -> Self {
        let mut seq = Self {
            data: b,
            alphabet: Vec::new(),
            alphabet_size,
        };
        seq.determine_alphabet();
        seq
    }

    pub fn with_alphabet_size(alphabet_size: u32) -> Self {
        Self {
            data: Vec::new(),
            alphabet: Vec::new(),
            alphabet_size,
        }
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.data
    }

    pub fn as_bytes_mut(&mut self) -> &mut [u8] {
        &mut self.data
    }

    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(&self.data).into_owned()
    }

    pub fn alphabet(&self) -> &[u8] {
        &self.alphabet
    }

    pub fn alphabet_size(&self) -> u32 {
        self.alphabet_size
    }

    pub fn set_alphabet_size(&mut self, s: u32) {
        self.alphabet_size = s;
    }

    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn is_empty(&self) -> bool {
        self.data.is_empty()
    }

    /// Scan the data and rebuild the alphabet (sorted descending), updating
    /// `alphabet_size` if needed. Mirrors the C++ `DetermineAlphabet()`.
    pub fn determine_alphabet(&mut self) -> Vec<u8> {
        let mut seen = [false; 256];
        for &c in &self.data {
            seen[c as usize] = true;
        }
        let mut alph = Vec::with_capacity(64);
        for i in (0..=255u16).rev() {
            if seen[i as usize] {
                alph.push(i as u8);
            }
        }
        if alph.len() as u32 != self.alphabet_size {
            self.alphabet_size = if alph.len() < 2 { 2 } else { alph.len() as u32 };
        }
        self.alphabet = alph.clone();
        alph
    }

    pub fn push(&mut self, c: u8) -> usize {
        self.data.push(c);
        self.data.len()
    }

    pub fn pop(&mut self) -> usize {
        self.data.pop();
        self.data.len()
    }

    pub fn clear(&mut self) {
        self.alphabet_size = ALPHABET_SIZE;
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
        self.data.get(start..end).and_then(|s| s.iter().copied().min())
    }

    pub fn max_range(&self, start: usize, end: usize) -> Option<u8> {
        self.data.get(start..end).and_then(|s| s.iter().copied().max())
    }

    /// Mirror of `seq.Take(l)` — first l characters, clamped.
    pub fn take(&self, l: usize) -> Self {
        let n = l.min(self.data.len());
        Sequence::from_bytes_with_alphabet(self.data[..n].to_vec(), self.alphabet_size)
    }

    /// Mirror of `seq.Drop(l)` — skip first l characters.
    pub fn drop(&self, l: usize) -> Self {
        if l >= self.data.len() {
            return Sequence::with_alphabet_size(self.alphabet_size);
        }
        Sequence::from_bytes_with_alphabet(self.data[l..].to_vec(), self.alphabet_size)
    }

    /// Mirror of `seq.Split(l)`.
    pub fn split_at(&self, l: usize) -> (Self, Self) {
        let n = l.min(self.data.len());
        (
            Sequence::from_bytes_with_alphabet(self.data[..n].to_vec(), self.alphabet_size),
            Sequence::from_bytes_with_alphabet(self.data[n..].to_vec(), self.alphabet_size),
        )
    }

    /// Mirror of `seq.Granularity(gr)`.
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
        self.data.pop();
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
        // Equivalent to std::rotate(begin, begin+shift, end), which rotates LEFT by `shift`.
        // That's what the C++ rightShift does (rotates so beginning becomes [shift..]).
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

    /// Mirror of `seq.charDensity()`.
    pub fn char_density(&self) -> std::collections::BTreeMap<u8, u32> {
        let mut m = std::collections::BTreeMap::new();
        for &c in &self.data {
            *m.entry(c).or_insert(0u32) += 1;
        }
        m
    }

    /// Mirror of `seq.map(f)` — returns new sequence with `f` applied to each byte.
    pub fn map<F: Fn(u8) -> u8>(&self, f: F) -> Self {
        let mut data = Vec::with_capacity(self.data.len());
        for &c in &self.data {
            data.push(f(c));
        }
        Self {
            data,
            alphabet: Vec::new(),
            alphabet_size: self.alphabet_size,
        }
    }

    pub fn extend_from_slice(&mut self, other: &[u8]) {
        self.data.extend_from_slice(other);
    }

    /// In-place XOR: 0 where bytes match, 1 otherwise.
    pub fn xor_with(&mut self, other: &Sequence) -> Result<(), &'static str> {
        if self.data.len() != other.data.len() {
            return Err("SequenceNoMatchSize");
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
            alphabet: Vec::new(),
            alphabet_size: self.alphabet_size.max(rhs.alphabet_size),
        }
    }
}

/// In-place block shuffle of `s`. Picks two non-overlapping aligned blocks of
/// `block_size` and swaps them. Deterministic when called via [`Sequence::shuffle_seeded`].
///
/// Mirrors `lz::Shuffle(sequence&, lz_uint)` from the C++.
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
}
