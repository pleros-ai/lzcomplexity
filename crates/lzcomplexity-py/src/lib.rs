//! Python bindings for the `lzcomplexity` core library.
//!
//! The public surface is intentionally small: it matches the C++/nanobind
//! `PyCore.cpp` / `PyMetrics.cpp` / `PySpectral.cpp` layout, no more.
//!
//! Top-level:        `lz76`, `factorization`, `factors`, `entropy_density`, `emc`
//! `metrics` module: `nid`, `rid`
//! `spectral` module: `psd`, `entropy`, `semc`

use pyo3::exceptions::PyTypeError;
use pyo3::prelude::*;
use pyo3::types::{PyAny, PyBytes, PyList};

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
) -> core::lz76::LzArgs {
    let mut a = core::lz76::LzArgs::new();
    a.chunks = partitions;
    // `NO_ALPHABET` is the "auto-detect from sequence" sentinel that the
    // core library recognises.
    a.alphabet = alphabet.unwrap_or(core::NO_ALPHABET);
    a.log_base = log_base.unwrap_or(core::NO_ALPHABET);
    a.block_size = max_block_size;
    a
}

// ── Top-level functions ─────────────────────────────────────────────────────

/// Run the full LZ76 analysis on a sequence.
///
/// Computes the LZ76 factor count, the normalized entropy density, the
/// factor-boundary list, and the random-shuffle complexity in a single call.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. Strings/bytes are treated as raw symbols.
///     A ``list[int]`` (or any iterable of ints, e.g. a NumPy array) is
///     converted by concatenating each element's decimal representation —
///     so ``[0, 1, 10] -> "0110"``. ``list[str]`` is joined directly.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob; does not affect
///     the result.
/// alphabet : int | None, default None
///     Effective alphabet size used in the entropy denominator. ``None``
///     means auto-detect from the sequence (number of distinct symbols,
///     minimum 2).
/// log_base : int | None, default None
///     Logarithm base for the entropy formula. ``None`` matches the
///     (auto-detected or explicit) alphabet size, giving a normalized
///     entropy in [0, 1]. Pass ``2`` if you want entropy in bits.
/// max_block_size : int, default -1
///     Maximum block size for the random-shuffle stage. ``-1`` lets the
///     library pick a sensible value from the sequence length.
/// jobs : int, default 0
///     Reserved for future use. Currently ignored (rayon manages its pool).
///
/// Returns
/// -------
/// tuple
///     A 4-tuple ``(complexity, entropy, factors, shuffle)`` where:
///
///     - ``complexity`` (int): LZ76 factor count.
///     - ``entropy`` (float): normalized entropy density.
///     - ``factors`` (list[int]): factor boundary positions; the i-th factor
///       spans ``[factors[i], factors[i+1])``.
///     - ``shuffle`` (tuple): ``(max_block_size, emc_value, multi_information)``
///       from the random-shuffle analysis.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> c, h, f, s = lz.lz76("ABRACADABRA")
/// >>> c
/// 7
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
    let args = build_args(partitions, alphabet, log_base, max_block_size);
    let r = core::metrics::lz76(&s, &args);
    Python::with_gil(|py| {
        let shuffle = (
            r.random_shuffle_complexity.max_block_size,
            r.random_shuffle_complexity.emc_value,
            r.random_shuffle_complexity.multi_information,
        )
            .to_object(py);
        let tuple = (
            r.complexity,
            r.entropy_density,
            r.factors,
            shuffle,
        )
            .to_object(py);
        Ok(tuple)
    })
}

/// Compute the LZ76 complexity (number of factors) of a sequence.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`lz76` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base used by downstream entropy. ``None`` matches the
///     alphabet size (normalized entropy).
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// int
///     The LZ76 factor count.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> lz.factorization("01010101")
/// 2
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn factorization(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    jobs: u32,
) -> PyResult<u32> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, -1);
    Ok(core::lz76::lz76_factorization(&s, &args))
}

/// Compute the LZ76 factorization (complexity + factor boundaries).
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`lz76` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base used by downstream entropy. ``None`` matches the
///     alphabet size (normalized entropy).
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// tuple[int, list[int]]
///     ``(complexity, factor_positions)``. The i-th factor spans
///     ``[positions[i], positions[i+1])``.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> lz.factors("banana")
/// (3, [0, 1, 2, 3, 7])
#[pyfunction]
#[pyo3(signature = (seq, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn factors(
    seq: &Bound<'_, PyAny>,
    partitions: i32,
    alphabet: Option<u32>,
    log_base: Option<u32>,
    jobs: u32,
) -> PyResult<PyObject> {
    let _ = jobs;
    let s = coerce_sequence(seq)?;
    let args = build_args(partitions, alphabet, log_base, -1);
    let r = core::lz76::lz76_factors(&s, &args);
    Python::with_gil(|py| Ok((r.factorization, r.lzf).to_object(py)))
}

/// Compute the LZ76-based normalized entropy density of a sequence.
///
/// Estimates the entropy rate as ``h ~= c(S) * log_k(n) / n``, where ``c(S)``
/// is the LZ76 factor count and ``k`` is the alphabet size. Converges to the
/// true entropy rate for ergodic sources as ``n -> infinity``.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`lz76` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size used in the entropy formula. ``None``
///     auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size (normalized
///     entropy in [0, 1]); pass ``2`` for entropy in bits.
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
    let args = build_args(partitions, alphabet, log_base, -1);
    Ok(core::lz76::lz76_entropy_density(&s, &args))
}

/// Effective measure complexity (EMC) via random block shuffling.
///
/// Compares the LZ76 complexity of the original sequence against shuffled
/// versions at several block sizes to estimate the non-random information
/// content. Higher ``emc_value`` means more structure than random noise of the
/// same length and alphabet.
///
/// The shuffle is seeded deterministically from the sequence content, so
/// repeated calls on the same input produce identical results.
///
/// Parameters
/// ----------
/// seq : str | bytes | list[int] | list[str] | Iterable[int]
///     The input sequence. See :func:`lz76` for the conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions. Performance knob.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base for the entropy formula. ``None`` matches the
///     alphabet size.
/// max_block_size : int, default -1
///     Maximum shuffle block size. ``-1`` picks a value based on length.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// tuple[int, float, float]
///     ``(max_block_size, emc_value, multi_information)``.
///
/// Examples
/// --------
/// >>> import lzcomplexity as lz
/// >>> mbs, emc_val, mi = lz.emc("01001010101101010101110101010101010000100101011")
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
    let args = build_args(partitions, alphabet, log_base, max_block_size);
    let r = core::shuffle::lz76_random_shuffle_complexity(&s, &args);
    Python::with_gil(|py| Ok((r.max_block_size, r.emc_value, r.multi_information).to_object(py)))
}

// ── metrics submodule ───────────────────────────────────────────────────────

/// Normalized information distance between two sequences.
///
/// Computes ``max(C(XY) - C(X), C(YX) - C(Y)) / max(C(X), C(Y))``, an
/// LZ76-based approximation of the normalized information distance. The
/// result lies in ``[0, 1]`` for well-behaved inputs — 0 means the two
/// sequences carry the same information, 1 means they are maximally distinct.
///
/// Parameters
/// ----------
/// seq1, seq2 : str | bytes | list[int] | list[str] | Iterable[int]
///     The two input sequences. See :func:`lzcomplexity.lz76` for the
///     conversion rules.
/// partitions : int, default 1
///     Number of suffix-array partitions.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size.
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
    let args = build_args(partitions, alphabet, log_base, -1);
    Ok(core::metrics::lz76_information_distance(&a, &b, &args))
}

/// Random-shuffle information distance between two sequences.
///
/// Uses random-shuffle complexity on the concatenation ``seq1 + seq2`` as a
/// mutual-information estimator and returns ``1 - MI``. Useful when you want
/// a distance that downweights structure shared between the two halves.
///
/// Parameters
/// ----------
/// seq1, seq2 : str | bytes | list[int] | list[str] | Iterable[int]
///     The two input sequences.
/// partitions : int, default 1
///     Number of suffix-array partitions.
/// alphabet : int | None, default None
///     Effective alphabet size. ``None`` auto-detects from the sequence.
/// log_base : int | None, default None
///     Logarithm base. ``None`` matches the alphabet size.
/// jobs : int, default 0
///     Reserved for future use.
///
/// Returns
/// -------
/// float
///     The shuffle-based information distance.
#[pyfunction]
#[pyo3(signature = (seq1, seq2, partitions=1, alphabet=None, log_base=None, jobs=0))]
fn rid(
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
    let args = build_args(partitions, alphabet, log_base, -1);
    Ok(core::metrics::lz76_random_shuffle_distance(&a, &b, &args))
}

// ── spectral submodule ──────────────────────────────────────────────────────

fn build_signal(
    samples: Vec<f64>,
    sample_frequency: usize,
    use_complex: bool,
    cut: bool,
    apply_window: bool,
    win: String,
) -> core::spectral::Signal {
    core::spectral::Signal {
        samples,
        sample_rate: sample_frequency,
        use_complex,
        use_window: apply_window,
        cut,
        window: win,
    }
}

/// Power spectral density (PSD) of a real-valued signal, via FFT.
///
/// Returns the magnitude-squared of the FFT bins, scaled by
/// ``1 / sample_frequency**2``. Only the first half of the output carries
/// independent information (Hermitian symmetry).
///
/// Parameters
/// ----------
/// signal : list[float] | Iterable[float]
///     Real-valued time-domain samples.
/// sample_frequency : int
///     Sampling rate in Hz. Determines the frequency-axis scaling.
/// use_complex : bool, default True
///     If True use a complex-to-complex FFT; otherwise real-to-complex.
/// cut : bool, default False
///     If True, segment the signal into ``sample_frequency``-length windows
///     and average the per-window PSDs (Welch-style averaging).
/// step : int, default 10
///     Stride between consecutive segments when ``cut=True``.
/// apply_window : bool, default False
///     If True, multiply each segment by ``win`` before the FFT.
/// win : {"hann", "han", "hamming", "hamm", "ham"}, default "hann"
///     Window function name. Ignored when ``apply_window=False``.
///
/// Returns
/// -------
/// list[float]
///     The PSD; length equal to the input signal length (or to one segment
///     when ``cut=True``).
#[pyfunction]
#[pyo3(signature = (signal, sample_frequency, use_complex=true, cut=false, step=10, apply_window=false, win=String::from("hann")))]
fn psd(
    signal: Vec<f64>,
    sample_frequency: usize,
    use_complex: bool,
    cut: bool,
    step: usize,
    apply_window: bool,
    win: String,
) -> Vec<f64> {
    let sig = build_signal(signal, sample_frequency, use_complex, cut, apply_window, win);
    core::spectral::process_signal(&sig, step)
}

/// Spectral entropy of a real-valued signal.
///
/// Computes the Shannon entropy of the normalized PSD over the first half of
/// the spectrum. Low values indicate a peaky spectrum (e.g. pure tone),
/// high values indicate broad-band noise.
///
/// Parameters
/// ----------
/// signal : list[float] | Iterable[float]
///     Real-valued samples.
/// sample_frequency : int
///     Sampling rate in Hz.
/// use_complex : bool, default True
///     Complex-to-complex (True) vs real-to-complex (False) FFT.
/// cut : bool, default False
///     Whether to average over segments of ``sample_frequency`` samples.
/// step : int, default 10
///     Stride between segments when ``cut=True``.
/// apply_window : bool, default False
///     Apply a window before the FFT.
/// win : {"hann", "han", "hamming", "hamm", "ham"}, default "hann"
///     Window name.
///
/// Returns
/// -------
/// float
///     Spectral entropy, in bits.
#[pyfunction]
#[pyo3(signature = (signal, sample_frequency, use_complex=true, cut=false, step=10, apply_window=false, win=String::from("hann")))]
fn entropy(
    signal: Vec<f64>,
    sample_frequency: usize,
    use_complex: bool,
    cut: bool,
    step: usize,
    apply_window: bool,
    win: String,
) -> f64 {
    let sig = build_signal(signal, sample_frequency, use_complex, cut, apply_window, win);
    core::spectral::spectral_entropy(&sig, step)
}

/// Spectral effective measure complexity (SEMC).
///
/// Compares the spectral entropy of the original signal against the mean
/// spectral entropy of random block-shuffled copies. Positive values
/// indicate non-random spectral structure beyond what amplitude statistics
/// alone explain.
///
/// Parameters
/// ----------
/// signal : list[float] | Iterable[float]
///     Real-valued samples.
/// sample_frequency : int
///     Sampling rate in Hz.
/// block_size : int, default 0
///     Maximum shuffle block size. ``0`` lets the library pick a value from
///     the signal length.
/// use_complex : bool, default True
///     Complex-to-complex (True) vs real-to-complex (False) FFT.
/// cut : bool, default False
///     Average across windowed segments.
/// change_shuffle : bool, default False
///     If True, shuffle the PSD itself rather than the time-domain signal.
/// step : int, default 10
///     Segment stride when ``cut=True``.
/// apply_window : bool, default False
///     Apply a window before the FFT.
/// win : {"hann", "han", "hamming", "hamm", "ham"}, default "hann"
///     Window name.
///
/// Returns
/// -------
/// float
///     SEMC value (sum of per-block entropy differences).
#[pyfunction]
#[pyo3(signature = (signal, sample_frequency, block_size=0, use_complex=true, cut=false, change_shuffle=false, step=10, apply_window=false, win=String::from("hann")))]
fn semc(
    signal: Vec<f64>,
    sample_frequency: usize,
    block_size: i32,
    use_complex: bool,
    cut: bool,
    change_shuffle: bool,
    step: usize,
    apply_window: bool,
    win: String,
) -> f64 {
    let sig = build_signal(signal, sample_frequency, use_complex, cut, apply_window, win);
    core::spectral::effective_spectral_complexity(&sig, block_size, step, change_shuffle)
}

// ── Module entry point ──────────────────────────────────────────────────────

/// LZ76-based complexity analysis for symbolic sequences.
///
/// Top-level functions
/// -------------------
/// - :func:`lz76` — full analysis (complexity, entropy, factors, shuffle).
/// - :func:`factorization` — just the LZ76 factor count.
/// - :func:`factors` — complexity and factor boundary list.
/// - :func:`entropy_density` — normalized entropy density.
/// - :func:`emc` — effective measure complexity (random shuffle).
///
/// Submodules
/// ----------
/// - :mod:`metrics` — information distances (``nid``, ``rid``).
/// - :mod:`spectral` — FFT-based spectral analysis (``psd``, ``entropy``, ``semc``).
///
/// All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
/// ``list[str]``, or any iterable of ints (e.g. NumPy arrays). For
/// ``list[int]``, each element's decimal representation is concatenated:
/// ``[0, 1, 10]`` becomes the symbolic string ``"0110"``.
#[pymodule]
fn lzcomplexity(py: Python<'_>, m: &Bound<'_, PyModule>) -> PyResult<()> {
    m.add("__version__", env!("CARGO_PKG_VERSION"))?;

    m.add_function(wrap_pyfunction!(lz76, m)?)?;
    m.add_function(wrap_pyfunction!(factorization, m)?)?;
    m.add_function(wrap_pyfunction!(factors, m)?)?;
    m.add_function(wrap_pyfunction!(entropy_density, m)?)?;
    m.add_function(wrap_pyfunction!(emc, m)?)?;

    let metrics = PyModule::new_bound(py, "metrics")?;
    metrics.add_function(wrap_pyfunction!(nid, &metrics)?)?;
    metrics.add_function(wrap_pyfunction!(rid, &metrics)?)?;
    m.add_submodule(&metrics)?;

    let spectral = PyModule::new_bound(py, "spectral")?;
    spectral.add_function(wrap_pyfunction!(psd, &spectral)?)?;
    spectral.add_function(wrap_pyfunction!(entropy, &spectral)?)?;
    spectral.add_function(wrap_pyfunction!(semc, &spectral)?)?;
    m.add_submodule(&spectral)?;

    // Lock the public surface — anything not listed here is hidden from
    // `from lzcomplexity import *` and from generic tooling that respects
    // `__all__`.
    let all = pyo3::types::PyList::new_bound(
        py,
        [
            "lz76",
            "factorization",
            "factors",
            "entropy_density",
            "emc",
            "metrics",
            "spectral",
        ],
    );
    m.add("__all__", all)?;

    Ok(())
}
