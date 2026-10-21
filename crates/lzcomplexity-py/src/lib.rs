//! Python bindings for the `lzcomplexity` core library.
//!
//! Public surface (deliberately small, all top-level):
//!
//!   `factorization`, `h`, `emc`, `nid`, `lz76`
//!
//! Spectral analysis was removed from this library — it now lives in a separate
//! package. See the README for the migration note.

// pyo3's `#[pyfunction]` macro expands to code that `.into()`s an already-`PyErr`
// error; clippy attributes the resulting `useless_conversion` lint to our return
// types. It is macro noise, not our code.
#![allow(clippy::useless_conversion)]

use pyo3::exceptions::PyTypeError;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBytes, PyDict, PyList};

use lzcomplexity_core as core;

// ── Input coercion ──────────────────────────────────────────────────────────
//
// Accept the same forms the C++ binding accepted: `str`, `bytes`,
// `list[int]`, `list[str]`, or any iterable of ints (covers `numpy.ndarray`,
// `array.array`, etc.). `list[int]` follows the documented C++ rule of
// concatenating decimal representations of each element.

fn coerce_sequence(obj: &Bound<'_, PyAny>) -> PyResult<core::Sequence> {
    if let Ok(s) = obj.extract::<String>() {
        return Ok(core::Sequence::from_str(&s));
    }
    if let Ok(b) = obj.downcast::<PyBytes>() {
        return Ok(core::Sequence::from_bytes(b.as_bytes().to_vec()));
    }
    if let Ok(list) = obj.downcast::<PyList>() {
        if list.is_empty() {
            return Ok(core::Sequence::new());
        }
        if let Ok(first) = list.get_item(0).and_then(|x| x.extract::<String>()) {
            let mut out = String::with_capacity(list.len() + first.len());
            for item in list.iter() {
                let s: String = item.extract()?;
                out.push_str(&s);
            }
            return Ok(core::Sequence::from_str(&out));
        }
    }
    if let Ok(v) = obj.extract::<Vec<i64>>() {
        let s: String = v.iter().map(|n| n.to_string()).collect();
        return Ok(core::Sequence::from_str(&s));
    }
    Err(PyTypeError::new_err(
        "expected str, bytes, list[str], list[int], or iterable of ints",
    ))
}

fn build_args(
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    max_block_size: i32,
    shuffle_terms: bool,
) -> core::lz76::LzArgs {
    let mut a = core::lz76::LzArgs::new();
    a.chunks = partitions;
    // `NO_ALPHABET` is the "auto-detect from sequence" sentinel that the
    // core library recognises.
    a.alphabet = alphabet.unwrap_or(core::NO_ALPHABET);
    a.log_base = log_base.unwrap_or(core::NO_ALPHABET);
    a.block_size = max_block_size;
    a.get_shuffle_terms = shuffle_terms;
    a
}

// ── Top-level functions ─────────────────────────────────────────────────────

/// Compute the LZ76 factorization of a sequence.
///
/// Returns ``(complexity, factors)``. For nonconstant input, ``complexity``
/// counts complete LZ76 components; a trailing copied suffix without an additional
/// symbol is omitted. Thus ``"banana"`` has count 3 although its textbook
/// exhaustive history has four components. ``factors`` contains byte offsets;
/// the last offset may be one past the sequence length. Slice each factor as
/// ``data[factors[i]:min(factors[i+1], len(data))]`` on the encoded bytes.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     Bytes are raw symbols; strings use UTF-8 bytes. An integer sequence
///     (e.g. a tuple or one-dimensional NumPy array) is converted
///     by concatenating each element's decimal representation — so
///     ``[0, 1, 10] -> "0110"``. ``list[str]`` is joined directly.
///     Generators and iterators are not accepted. Prefer ``bytes`` for
///     categorical symbols: decimal concatenation can merge different inputs.
/// partitions : int, default 1
///     Reserved compatibility parameter. Currently ignored.
/// alphabet : int | None, default None
///     Has no effect on the returned count or boundaries.
/// log_base : int | None, default None
///     Has no effect on the returned count or boundaries.
/// jobs : int, default 0
///     Reserved for future use. Currently ignored (rayon manages its pool).
///
/// Returns
/// -------
/// tuple[int, list[int]]
///     ``(complexity, factor_boundary_indices)``.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> lz.factorization("banana")
/// (3, [0, 1, 2, 3, 7])
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn factorization(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    jobs: u32,
) -> PyResult<PyObject> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, -1, false);
    let r = core::lz76::lz76_factors(&s, &args);
    Python::with_gil(|py| Ok((r.factorization, r.lzf).to_object(py)))
}

/// Compute the LZ76-based normalized entropy density of a sequence.
///
/// Computes ``h = C(S) * log_b(n) / n``, where ``C(S)`` is the library's
/// complete-factor count and ``b`` is the resolved log base. The asymptotic
/// LZ76 result for stationary ergodic finite-alphabet sources is stated as
/// ``limsup h_n = h_source`` almost surely (Ziv 1978; Estevez-Rams et al.
/// 2013, Eq. 4). At finite length the estimate can be biased and can exceed
/// the alphabet's maximum entropy; no accuracy bound is supplied.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Reserved compatibility parameter. Currently ignored.
/// alphabet : int | None, default None
///     Effective alphabet size; the default log base follows this value.
///     ``None`` auto-detects distinct bytes, with a minimum of 2.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the effective alphabet size;
///     pass ``2`` for bits per input byte.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// float
///     Normalized entropy density.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> lz.h("01010101")
/// 0.75
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn h(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    jobs: u32,
) -> PyResult<f64> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, -1, false);
    Ok(core::lz76::lz76_entropy_density(&s, &args))
}

/// Estimate excess entropy from adjacent past and future windows.
///
/// At total window length ``m``, estimate ``I(past; future)`` with a past of
/// length ``floor(m/2)`` and a future of length ``ceil(m/2)``. Joint counts
/// and both marginal counts come from the same observations. All scales use
/// the same ``n - M + 1`` starts and a common past/future boundary, where
/// ``M`` is the resolved maximum window length. The smaller windows are
/// nested, so their empirical mutual informations are non-negative and
/// non-decreasing, apart from floating point roundoff. The returned terms
/// are their successive increments, beginning with zero at ``m = 1``.
///
/// This path uses empirical counts, with no random shuffle, Chao-Shen
/// correction, LZ-rate subtraction, peak selection, or isotonic projection.
/// With ``log_base=2`` the value is in bits, not bits per symbol.
///
/// .. warning::
///
///    The population finite-window mutual information approaches excess
///    entropy for a stationary source and is a lower bound on it. The sample
///    estimate is not a lower bound: plug-in mutual information has finite-
///    sample bias, while limited windows can miss long memory. Compare at
///    matched lengths, window caps, alphabets and estimator versions, and
///    calibrate against suitable null data. No confidence interval is returned.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Reserved compatibility parameter. Currently ignored.
/// alphabet : int | None, default None
///     Effective alphabet size for the default log base and auto window cap.
///     ``None`` auto-detects distinct bytes, with a minimum of 2.
/// log_base : int | None, default None
///     Output log base. ``None`` matches the effective alphabet size; ``2``
///     gives bits. Changing units does not change the selected window cap.
/// max_block_size : int, default -1
///     Maximum total past-plus-future window length. Auto mode (``<= 0``)
///     chooses the largest ``M`` with ``M * alphabet**M <= n`` (minimum 1).
///     An explicit positive value is clipped to the input length.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// tuple[float, list[float]]
///     ``(emc_value, summands)``. The value is the empirical mutual
///     information at the resolved cap ``M``. ``summands[m-1]`` is the
///     increment from total length ``m-1`` to ``m``; it is not the conditional-
///     entropy excess ``h(m)-h`` used by earlier versions. The non-negative
///     increments sum to the value. Empty input returns zero and no terms.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> emc_value, summands = lz.emc("01001010101101010101110101010101010000100101011")
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, max_block_size=-1, jobs=0))]
fn emc(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    max_block_size: i32,
    jobs: u32,
) -> PyResult<PyObject> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, max_block_size, true);
    let r = core::shuffle::lz76_random_shuffle_complexity(&s, &args);
    Python::with_gil(|py| Ok((r.emc_value, r.summands).to_object(py)))
}

/// Run the full LZ76 analysis on a sequence.
///
/// Computes the complete-factor count, entropy-rate estimate, byte boundaries,
/// EMC estimate and scale increments, and legacy diagnostic ratios. Whole-
/// sequence factorization is reused across these calculations.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Reserved compatibility parameter. Currently ignored.
/// alphabet : int | None, default None
///     Effective alphabet size for the default log base and auto window cap.
///     ``None`` auto-detects distinct bytes, with a minimum of 2.
/// log_base : int | None, default None
///     Output log base. ``None`` matches the effective alphabet size;
///     ``2`` gives ``h`` in bits per byte and ``emc.value`` in bits.
/// max_block_size : int, default -1
///     Maximum total past-plus-future window length. See :func:`emc`.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// dict
///     A dictionary with keys:
///
///     - ``complexity`` (int): complete LZ76 factor count.
///     - ``h`` (float): entropy-rate estimate in the requested log base.
///     - ``factors`` (list[int]): byte boundaries; the last may be ``n+1``.
///     - ``emc`` (dict): ``{"value", "summands", "max_block_size",
///       "multi_information"}``; see :func:`emc` for estimator details.
///       ``multi_information`` is the separate legacy contrast
///       ``H_plugin(1) - h_LZ`` over the whole input. It may be negative and
///       does not enter the finite-window mutual information estimate.
///     - ``epsilon`` (float): finite-size correction term.
///     - ``factors_stddev`` (float): legacy dispersion formula, not a standard
///       deviation (it divides by the longest factor length).
///     - ``normal_error`` / ``poison_error`` (float): legacy diagnostics, not
///       calibrated standard errors or confidence intervals.
///     - ``extras`` (dict): ``{"rajski_distance", "redundancy",
///       "fh_uncertainty", "lh_uncertainty", "pearson_coefficient"}``.
///       These compare phrase counts of the sequence's two halves; they are
///       not Shannon mutual information or statistical correlation estimates.
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, max_block_size=-1, jobs=0))]
fn lz76(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    max_block_size: i32,
    jobs: u32,
) -> PyResult<PyObject> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, max_block_size, true);
    let r = core::metrics::lz76(&s, &args);
    Python::with_gil(|py| {
        let emc = PyDict::new_bound(py);
        emc.set_item("value", r.random_shuffle_complexity.emc_value)?;
        emc.set_item("summands", r.random_shuffle_complexity.summands.clone())?;
        emc.set_item("max_block_size", r.random_shuffle_complexity.max_block_size)?;
        emc.set_item(
            "multi_information",
            r.random_shuffle_complexity.multi_information,
        )?;

        let extras = PyDict::new_bound(py);
        extras.set_item("rajski_distance", r.extras.lz_rajski_distance)?;
        extras.set_item("redundancy", r.extras.redundancy)?;
        extras.set_item("fh_uncertainty", r.extras.fh_uncertainty)?;
        extras.set_item("lh_uncertainty", r.extras.lh_uncertainty)?;
        extras.set_item("pearson_coefficient", r.extras.lz_pearson_coefficient)?;

        let out = PyDict::new_bound(py);
        out.set_item("complexity", r.complexity)?;
        out.set_item("h", r.entropy_density)?;
        out.set_item("factors", r.factors)?;
        out.set_item("emc", emc)?;
        out.set_item("epsilon", r.epsilon)?;
        out.set_item("factors_stddev", r.factors_stddev)?;
        out.set_item("normal_error", r.lz_normal_errors)?;
        out.set_item("poison_error", r.lz_poison_errors)?;
        out.set_item("extras", extras)?;
        Ok(out.to_object(py))
    })
}

// ── Information distance ─────────────────────────────────────────────────────

/// Normalized information distance (NID) between two sequences.
///
/// Computes ``max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))``, an
/// LZ76 phrase-count dissimilarity. This is a heuristic surrogate, not the
/// uncomputable Kolmogorov normalized information distance. Self-distance may
/// be nonzero, different strings can have distance zero, and the triangle
/// inequality can fail. A value of 1 is not proof of source independence.
///
/// Parameters
/// ----------
/// seq1, seq2 : str | bytes | list[int] | list[str] | Sequence[int]
///     The two input sequences. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Reserved compatibility parameter. Currently ignored.
/// alphabet : int | None, default None
///     Does not affect this ratio of factor counts.
/// log_base : int | None, default None
///     Does not affect this ratio of factor counts.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// float
///     The normalized information distance.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> lz.nid("abcd", "abce")
/// 0.25
#[pyfunction]
#[pyo3(signature = (seq1, seq2, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn nid(
    seq1: &Bound<'_, PyAny>,
    seq2: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    jobs: u32,
) -> PyResult<f64> {
    let _ = jobs;
    let a = coerce_sequence(seq1)?;
    let b = coerce_sequence(seq2)?;
    let args = build_args(partitions, alphabet, log_base, -1, false);
    Ok(core::metrics::lz76_information_distance(&a, &b, &args))
}

// ── Module entry point ──────────────────────────────────────────────────────

/// LZ76-based complexity analysis for symbolic sequences.
///
/// Top-level functions
/// -------------------
/// - :func:`factorization` — complexity + factor boundaries.
/// - :func:`h` — normalized entropy density (entropy-rate estimator).
/// - :func:`emc` — effective measure complexity (value + summand terms).
/// - :func:`nid` — normalized information distance between two sequences.
/// - :func:`lz76` — the full analysis (everything, as a dict).
///
/// All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
/// ``list[str]``, or any iterable of ints (e.g. NumPy arrays). For
/// ``list[int]``, each element's decimal representation is concatenated:
/// ``[0, 1, 10]`` becomes the symbolic string ``"0110"``.
#[pymodule]
fn lzcomplexity(py: Python<'_>, m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;

    m.add_function(wrap_pyfunction!(factorization, m)?)?;
    m.add_function(wrap_pyfunction!(h, m)?)?;
    m.add_function(wrap_pyfunction!(emc, m)?)?;
    m.add_function(wrap_pyfunction!(nid, m)?)?;
    m.add_function(wrap_pyfunction!(lz76, m)?)?;

    // Lock the public surface — anything not listed here is hidden from
    // `from lzcomplexity import *` and from generic tooling that respects
    // `__all__`.
    let all = pyo3::types::PyList::new_bound(py, ["factorization", "h", "emc", "nid", "lz76"]);
    m.add("__all__", all)?;

    Ok(())
}
