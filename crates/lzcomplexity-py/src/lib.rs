//! Python bindings for the `lzcomplexity` core library.
//!
//! Public surface (deliberately small):
//!
//! Top-level:        `factorization`, `entropy_density`, `emc`, `lz76`
//! `metrics` module: `nid` (a.k.a. `information_distance`)
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
/// Returns a tuple ``(complexity, factors)`` where ``complexity`` is the
/// un-normalized LZ76 factor count and ``factors`` is the list of boundary
/// indices at which the factors separate — the i-th factor spans
/// ``[factors[i], factors[i+1])``. Returning the boundary indices (rather than
/// the substrings) keeps the result compact.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. Strings/bytes are treated as raw symbols. A
///     ``list[int]`` (or any iterable of ints, e.g. a NumPy array) is converted
///     by concatenating each element's decimal representation — so
///     ``[0, 1, 10] -> "0110"``. ``list[str]`` is joined directly.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob; no effect on the result.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects (distinct symbols, min 2).
/// log_base : int | None, default None
///     Logarithm base for downstream entropy. ``None`` matches the alphabet size.
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
/// Estimates the entropy rate as ``h ~= c(S) * log_k(n) / n``, where ``c(S)``
/// is the LZ76 factor count and ``k`` is the alphabet size. This is the
/// *normalized* complexity (entropy-rate estimator): it converges to the true
/// entropy rate for ergodic sources as ``n -> infinity``.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size used in the entropy formula. ``None`` auto-detects.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size (the normalized form);
///     pass ``2`` for entropy in bits.
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
/// >>> lz.entropy_density("01010101")
/// 0.75
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn entropy_density(
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

/// Effective measure complexity (EMC) via random block shuffling.
///
/// Compares the LZ76 complexity of the original sequence against shuffled
/// versions at several block sizes to estimate the non-random information
/// content. Higher values mean more structure than random noise of the same
/// length and alphabet.
///
/// The shuffle is seeded deterministically from the sequence content, so
/// repeated calls on the same input produce identical results.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base for the entropy formula. ``None`` matches the alphabet size.
/// max_block_size : int, default -1
///     Maximum shuffle block size. ``-1`` picks a value based on length.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// tuple[float, list[float]]
///     ``(emc_value, summands)`` where ``emc_value`` is the effective measure
///     complexity and ``summands`` is the list of per-block-size terms whose
///     sum is ``emc_value``. ``summands[0]`` is the multi-information term.
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
/// The "give me everything" call: computes the factor count, the normalized
/// entropy density, the factor boundaries, the random-shuffle EMC (with its
/// summand terms), the error estimates, and the extra pairwise measures in a
/// single pass.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size (normalized entropy).
/// max_block_size : int, default -1
///     Maximum shuffle block size for the EMC stage. ``-1`` auto-selects.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// dict
///     A dictionary with keys:
///
///     - ``complexity`` (int): LZ76 factor count.
///     - ``entropy_density`` (float): normalized entropy density.
///     - ``factors`` (list[int]): factor boundary indices.
///     - ``emc`` (dict): ``{"value", "summands", "max_block_size",
///       "multi_information"}`` from the random-shuffle analysis.
///     - ``epsilon`` (float): finite-size correction term.
///     - ``factors_stddev`` (float): standard deviation of factor lengths.
///     - ``normal_error`` / ``poison_error`` (float): error estimators.
///     - ``extras`` (dict): ``{"rajski_distance", "redundancy",
///       "fh_uncertainty", "lh_uncertainty", "pearson_coefficient"}``.
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
        out.set_item("entropy_density", r.entropy_density)?;
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

// ── metrics submodule ───────────────────────────────────────────────────────

/// Normalized information distance (NID) between two sequences.
///
/// Computes ``max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))``, an
/// LZ76-based, conditional-entropy-flavoured normalized distance. The result
/// lies in ``[0, 1]`` for well-behaved inputs — 0 means the two sequences
/// carry the same information, 1 means they are maximally distinct.
///
/// This function is also exported as ``metrics.information_distance`` (the name
/// used by the C++ backend).
///
/// Parameters
/// ----------
/// seq1, seq2 : str | bytes | list[int] | list[str] | Iterable[int]
///     The two input sequences. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size. (Does not affect the
///     ratio of factor counts; present for signature symmetry.)
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
/// >>> lz.metrics.nid("abcd", "abce")
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
/// - :func:`entropy_density` — normalized entropy density.
/// - :func:`emc` — effective measure complexity (value + summand terms).
/// - :func:`lz76` — the full analysis (everything, as a dict).
///
/// Submodule
/// ---------
/// - :mod:`metrics` — information distance (``nid`` / ``information_distance``).
///
/// All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
/// ``list[str]``, or any iterable of ints (e.g. NumPy arrays). For
/// ``list[int]``, each element's decimal representation is concatenated:
/// ``[0, 1, 10]`` becomes the symbolic string ``"0110"``.
#[pymodule]
fn lzcomplexity(py: Python<'_>, m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;

    m.add_function(wrap_pyfunction!(factorization, m)?)?;
    m.add_function(wrap_pyfunction!(entropy_density, m)?)?;
    m.add_function(wrap_pyfunction!(emc, m)?)?;
    m.add_function(wrap_pyfunction!(lz76, m)?)?;

    let metrics = PyModule::new_bound(py, "metrics")?;
    metrics.add_function(wrap_pyfunction!(nid, &metrics)?)?;
    // `information_distance` is the C++-compatible alias for `nid`.
    metrics.add("information_distance", metrics.getattr("nid")?)?;
    m.add_submodule(&metrics)?;

    // Lock the public surface — anything not listed here is hidden from
    // `from lzcomplexity import *` and from generic tooling that respects
    // `__all__`.
    let all = pyo3::types::PyList::new_bound(
        py,
        ["factorization", "entropy_density", "emc", "lz76", "metrics"],
    );
    m.add("__all__", all)?;

    Ok(())
}
