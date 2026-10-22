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
// `list[int]`, `list[str]`, or compatible integer sequences (e.g. `numpy.ndarray`,
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
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. Strings use UTF-8 bytes; bytes are unchanged. A
///     ``list[int]`` (or a compatible integer sequence, e.g. a NumPy array) is converted
///     by concatenating each element's decimal representation — so
///     ``[0, 1, 10] -> "0110"``. ``list[str]`` is joined directly.
/// partitions : int, default 1
///     Unused.
/// alphabet : int | None, default None
///     Alphabet parameter for finite-size diagnostics. ``None`` auto-detects.
/// log_base : int | None, default None
///     Logarithm base for downstream entropy. ``None`` uses the observed input
///     alphabet size, floored at two, even if ``alphabet`` is explicit.
/// jobs : int, default 0
///     Unused. Rayon manages native parallelism.
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
/// Estimates entropy density as ``h = c(S) * log_b(n) / n``, where ``c(S)``
/// is the LZ76 factor count, ``n`` is the byte length, and ``b`` is the
/// logarithm base. This captures the production of new information along
/// the sequence.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Unused.
/// alphabet : int | None, default None
///     Alphabet parameter for the finite-size ``epsilon`` diagnostic.
///     ``None`` auto-detects. This does not set the default logarithm base.
/// log_base : int | None, default None
///     Logarithm base. ``None`` uses the observed input alphabet, floored at
///     two, independently of an explicit ``alphabet``. Pass ``2`` for bits
///     per input byte.
/// jobs : int, default 0
///     Unused. Rayon manages native parallelism.
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

/// Compute structural complexity E, which is sensitive to correlations across
/// different ranges of a sequence.
///
/// Returns the final level of the fitted scale profile and its nonnegative
/// increments. Identical input bytes and parameters give the same result.
/// Compare sequences with matched lengths, encodings, log bases and ranges.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Unused.
/// alphabet : int | None, default None
///     Alphabet parameter for the finite-size ``epsilon`` diagnostic.
///     ``None`` uses the observed input alphabet, floored at two. An explicit
///     value does not change the default logarithm base.
/// log_base : int | None, default None
///     Logarithm base for normalization. ``None`` uses the observed input
///     alphabet size, floored at two, independently of ``alphabet``.
/// max_block_size : int, default -1
///     Maximum block range in bytes. Nonpositive values select it automatically.
/// jobs : int, default 0
///     Unused. Rayon manages native parallelism.
///
/// Returns
/// -------
/// tuple[float, list[float]]
///     ``(value, summands)``. The score and fitted increments are nonnegative,
///     and the increments sum to the score. ``summands[l-1]`` is the fitted
///     increase from scale ``l-1`` to ``l``.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> value, summands = lz.emc("01001010101101010101110101010101010000100101011", log_base=2)
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
/// Computes the factor count, entropy density h, factor boundaries,
/// structural complexity E, fitted increments and diagnostic fields.
/// The original factorization is shared by the calculations.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Sequence[int]
///     The input sequence. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Unused.
/// alphabet : int | None, default None
///     Alphabet parameter for finite-size diagnostics. ``None`` auto-detects.
/// log_base : int | None, default None
///     Logarithm base. ``None`` uses the observed input alphabet, floored at
///     two, independently of an explicit ``alphabet``.
/// max_block_size : int, default -1
///     Maximum block range for E. Nonpositive values select it automatically.
/// jobs : int, default 0
///     Unused. Rayon manages native parallelism.
///
/// Returns
/// -------
/// dict
///     A dictionary with keys:
///
///     - ``complexity`` (int): LZ76 factor count.
///     - ``h`` (float): normalized entropy density.
///     - ``factors`` (list[int]): factor boundary indices.
///     - ``emc`` (dict): ``{"value", "summands", "max_block_size",
///       "multi_information"}`` for E. The ``multi_information`` field is the
///       raw first-range contrast before fitting.
///     - ``epsilon`` (float): finite-size diagnostic.
///     - ``factors_stddev`` (float): factor-dispersion diagnostic.
///     - ``normal_error`` / ``poison_error`` (float): diagnostic formulas based
///       on h, sequence length and factor dispersion.
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
/// LZ76-based normalized distance. Lower values indicate more shared patterns.
/// For finite sequences, self-distance can be nonzero.
///
/// Parameters
/// ----------
/// seq1, seq2 : str | bytes | list[int] | list[str] | Sequence[int]
///     The two input sequences. See :func:`factorization` for the conversion rules.
/// partitions : int, default 1
///     Unused.
/// alphabet : int | None, default None
///     Alphabet parameter for finite-size diagnostics. ``None`` auto-detects.
/// log_base : int | None, default None
///     Logarithm base; unused by this ratio of factor counts.
/// jobs : int, default 0
///     Unused. Rayon manages native parallelism.
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
/// - :func:`emc` — structural complexity E (value + fitted increments).
/// - :func:`nid` — normalized information distance between two sequences.
/// - :func:`lz76` — the full analysis (everything, as a dict).
///
/// All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
/// ``list[str]``, or compatible integer sequences (e.g. NumPy arrays).
/// Materialize generators with ``list(...)`` or ``bytes(...)`` first. For
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
