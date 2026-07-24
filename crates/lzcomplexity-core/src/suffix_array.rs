//! Suffix array + LCP construction.
//!
//! The C++ library has its own CaPS algorithm (a parallel SA construction).
//! We build the suffix array in linear time with `cdivsufsort` (a byte-level
//! divsufsort) and compute LCP with Kasai's algorithm. The suffix array of a
//! string is unique, so this produces the same order as a comparison sort would
//! — bit-identical downstream results — but in O(n) instead of O(n² log n).

/// Suffix array + LCP array for a text of length `n`.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct SuffixArray {
    pub sa: Vec<u32>,
    pub lcp: Vec<u32>,
    pub n: u32,
}

impl SuffixArray {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn clear(&mut self) {
        self.sa.clear();
        self.lcp.clear();
        self.n = 0;
    }
}

/// Build a suffix array for the given text.
///
/// The output `sa` does **not** contain a sentinel suffix — its length is
/// exactly `text.len()`, matching what the C++ `CaPS_SA` produces.
pub fn build_suffix_array(text: &[u8]) -> Vec<u32> {
    if text.is_empty() {
        return Vec::new();
    }
    // Both paths produce the (unique) suffix array, so results are identical.
    // For tiny inputs a plain suffix sort has a much lower constant than
    // divsufsort's setup, and even its worst case is sub-millisecond at this
    // size; above the cutoff we switch to the byte-level linear-time divsufsort
    // so repetitive/large inputs never hit the O(n² log n) sort.
    const SORT_CUTOFF: usize = 2048;
    if text.len() < SORT_CUTOFF {
        sort_based_sa(text)
    } else {
        let (_, sa) = cdivsufsort::sort(text).into_parts();
        sa.into_iter().map(|x| x as u32).collect()
    }
}

/// Comparison suffix sort — low constant, but O(n² log n) on repetitive data,
/// so it is only used below `SORT_CUTOFF` (and as the fuzz-test reference).
fn sort_based_sa(text: &[u8]) -> Vec<u32> {
    let n = text.len();
    let mut idx: Vec<u32> = (0..n as u32).collect();
    idx.sort_unstable_by(|&a, &b| text[a as usize..].cmp(&text[b as usize..]));
    idx
}

/// Kasai's algorithm — computes the LCP array from a text and its suffix array.
///
/// `lcp[i]` is the longest common prefix length between `sa[i-1]` and `sa[i]`
/// (with `lcp[0] = 0`). Same convention as `lz::utils::LZ_SuffixArray::LCP`.
pub fn build_lcp(text: &[u8], sa: &[u32]) -> Vec<u32> {
    let n = sa.len();
    if n == 0 {
        return Vec::new();
    }
    let mut rank = vec![0u32; n];
    for i in 0..n {
        rank[sa[i] as usize] = i as u32;
    }
    let mut lcp = vec![0u32; n];
    let mut h: u32 = 0;
    for i in 0..n {
        let r = rank[i] as usize;
        if r > 0 {
            let j = sa[r - 1] as usize;
            while i + (h as usize) < n
                && j + (h as usize) < n
                && text[i + (h as usize)] == text[j + (h as usize)]
            {
                h += 1;
            }
            lcp[r] = h;
            h = h.saturating_sub(1);
        } else {
            h = 0;
        }
    }
    lcp
}

/// Convenience: build both SA and LCP in one go.
pub fn build(text: &[u8]) -> SuffixArray {
    let sa = build_suffix_array(text);
    let lcp = build_lcp(text, &sa);
    SuffixArray {
        sa,
        lcp,
        n: text.len() as u32,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn banana_sa() {
        let sa = build_suffix_array(b"banana");
        assert_eq!(sa, vec![5, 3, 1, 0, 4, 2]);
    }

    #[test]
    fn test_text_sa() {
        let sa = build_suffix_array(b"test text");
        assert_eq!(sa, vec![4, 1, 6, 2, 8, 3, 0, 5, 7]);
    }

    #[test]
    fn fuzz_sa_matches_reference() {
        // The fast (divsufsort) SA must equal the reference comparison sort for
        // every input — including non-UTF-8 bytes and degenerate repeats.
        let mut state: u64 = 0x1234_5678_9abc_def0;
        let mut next = || {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            state
        };
        let alphabets: [&[u8]; 5] = [b"01", b"012", b"ACGT", &[0u8, 1, 255], b"abcdefghij"];
        for _ in 0..300 {
            let ab = alphabets[(next() as usize) % alphabets.len()];
            // Span both branches of build_suffix_array: below the sort cutoff
            // (sort path) and above it (divsufsort path, vs the sort reference).
            let n = 1 + (next() as usize) % 5000;
            let text: Vec<u8> = (0..n).map(|_| ab[(next() as usize) % ab.len()]).collect();
            assert_eq!(
                build_suffix_array(&text),
                sort_based_sa(&text),
                "SA mismatch for prefix {:?}",
                &text[..text.len().min(24)]
            );
        }
        // degenerate: all-equal bytes
        assert_eq!(build_suffix_array(&[7u8; 500]), sort_based_sa(&[7u8; 500]));
        assert_eq!(build_suffix_array(b"a"), sort_based_sa(b"a"));
    }

    #[test]
    fn lcp_banana() {
        let sa = build_suffix_array(b"banana");
        let lcp = build_lcp(b"banana", &sa);
        // Standard Kasai LCP for "banana" (lcp[0] = 0):
        // SA = [5,3,1,0,4,2] for suffixes a,ana,anana,banana,na,nana
        // lcp[0..] = [0, 1, 3, 0, 0, 2]
        assert_eq!(lcp, vec![0, 1, 3, 0, 0, 2]);
    }
}
