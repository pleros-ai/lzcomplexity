//! Suffix array + LCP construction.
//!
//! The C++ library has its own CaPS algorithm (a parallel SA construction).
//! We delegate SA construction to the `suffix` crate (or a small SA-IS-style
//! implementation) and compute LCP with Kasai's algorithm.

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
    // Use a simple sort-of-suffixes implementation. For n <= LARGE_N we sort
    // indices directly by their suffix slice. For longer inputs we delegate to
    // the `suffix` crate, which gives O(n) construction over UTF-8 strings.
    // The text we get is raw bytes (potentially with embedded zeros), so the
    // sort path is the safer general default — `suffix` requires a valid &str.
    const SMALL_N: usize = 4096;
    if text.len() <= SMALL_N {
        sort_based_sa(text)
    } else if let Ok(s) = std::str::from_utf8(text) {
        // `suffix::SuffixTable` allocates and runs SA-IS-like construction.
        let table = suffix::SuffixTable::new(s);
        // `table.table()` returns &[u32] of the byte-indexed suffix array.
        table.table().to_vec()
    } else {
        // Fallback: still sort-based, but for large inputs this is O(n^2 log n).
        sort_based_sa(text)
    }
}

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
            if h > 0 {
                h -= 1;
            }
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
    fn lcp_banana() {
        let sa = build_suffix_array(b"banana");
        let lcp = build_lcp(b"banana", &sa);
        // Standard Kasai LCP for "banana" (lcp[0] = 0):
        // SA = [5,3,1,0,4,2] for suffixes a,ana,anana,banana,na,nana
        // lcp[0..] = [0, 1, 3, 0, 0, 2]
        assert_eq!(lcp, vec![0, 1, 3, 0, 0, 2]);
    }
}
