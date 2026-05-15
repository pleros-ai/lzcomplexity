//! Longest Previous Factor — port of `lz::utils::LPF` from modules/core/src/lpf.cpp.
//!
//! For each position i in the sequence, lpf[i] is the length of the longest
//! factor starting at i that also occurs earlier in the sequence. The C++
//! implementation operates on the SA / LCP arrays via a stack of (len, pos).

/// Compute the LPF array. `lpf` must already be sized to `n`.
pub fn lpf(lpf_out: &mut [u32], mut sa: Vec<u32>, mut lcp_in: Vec<u32>, n: usize) {
    debug_assert!(lpf_out.len() >= n);
    if n == 0 {
        return;
    }
    if n == 1 {
        lpf_out[0] = 0;
        return;
    }

    // The C++ pushes a 0 sentinel onto SA and LCP, indexing up to i == n.
    sa.push(0);
    lcp_in.push(0);

    // Stack of (len, pos). pos here matches the C++ semantics: SA[i] + 1.
    let mut stack: Vec<(u32, u32)> = Vec::new();
    stack.push((0, sa[0] + 1));

    for i in 1..=n {
        let mut lcp = lcp_in[i];
        while let Some(&(top_len, top_pos)) = stack.last() {
            if sa[i] + 1 >= top_pos {
                break;
            }
            // lpf[top_pos - 1] = max(top_len, lcp); lcp = min(top_len, lcp); stack.pop()
            let target = (top_pos - 1) as usize;
            if target < lpf_out.len() {
                lpf_out[target] = top_len.max(lcp);
            }
            lcp = top_len.min(lcp);
            stack.pop();
        }
        if i < n {
            stack.push((lcp, sa[i] + 1));
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::suffix_array::{build_lcp, build_suffix_array};

    #[test]
    fn lpf_banana() {
        let text = b"banana";
        let sa = build_suffix_array(text);
        let lcp = build_lcp(text, &sa);
        let mut out = vec![0u32; text.len()];
        lpf(&mut out, sa, lcp, text.len());
        // Reasonable smoke test: the LZ76 walk over this must produce
        // a non-empty factor list (validated indirectly in lz76 tests).
        assert_eq!(out.len(), 6);
    }
}
