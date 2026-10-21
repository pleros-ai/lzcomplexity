"""Type stubs for the `lzcomplexity` package.

These stubs mirror the runtime surface declared in :mod:`lzcomplexity.__init__`
and the compiled extension :mod:`lzcomplexity.lzcomplexity`. They exist so
editors and type-checkers (mypy, pyright, ruff) can show signatures and
docstrings without loading the compiled extension.
"""

from typing import Any, Dict, List, Sequence, Tuple, Union

# A sequence-like input accepted by every `lz76*` function.
SeqLike = Union[str, bytes, List[str], Sequence[int]]

__version__: str

__all__ = [
    "lz76",
    "factorization",
    "h",
    "emc",
    "nid",
]


def factorization(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> Tuple[int, List[int]]:
    """Return the complete-factor count and byte boundaries.

    A trailing copied suffix with no additional symbol is excluded from the count.
    The final boundary may be one past the byte length.
    """
    ...


def h(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> float:
    """LZ76 entropy-rate estimate for stationary ergodic finite-alphabet sources.

    ``log_base=2`` gives bits per byte; finite-sample estimates can be biased.
    ``partitions`` and ``jobs`` are currently ignored.
    """
    ...


def emc(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    max_block_size: int = -1,
    jobs: int = 0,
) -> Tuple[float, List[float]]:
    """Empirical mutual information between adjacent past and future windows.

    ``max_block_size`` caps their combined length, not each side separately.
    The past has floor(m/2) symbols and the future ceil(m/2) at scale m.
    Nested windows at all scales use the same observation starts and boundary.
    Returns ``(value, increments)``; non-negative increments sum to the value.
    ``log_base=2`` gives bits. No shuffling or LZ-rate subtraction is used.

    The population target approaches excess entropy for stationary sources;
    finite windows can miss memory and sample estimates can be biased upward.
    Compare matched lengths, caps, alphabets and estimator versions.
    """
    ...


def nid(
    seq1: SeqLike,
    seq2: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> float:
    """Heuristic LZ76 phrase-count dissimilarity between two sequences.

    Self-distance can be nonzero and the triangle inequality can fail.
    """
    ...


def lz76(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    max_block_size: int = -1,
    jobs: int = 0,
) -> Dict[str, Any]:
    """Full LZ76 analysis, returned as a dict.

    Keys: ``complexity``, ``h``, ``factors``, ``emc`` (a dict of
    ``value``/``summands``/``max_block_size``/``multi_information``),
    ``epsilon``, ``factors_stddev``, ``normal_error``, ``poison_error``,
    ``extras`` (a dict). The dispersion/error fields are legacy diagnostics,
    not calibrated statistical uncertainty. ``extras`` are phrase-count ratios.
    ``emc.multi_information`` is the separate legacy ``H_plugin(1)-h_LZ``
    contrast, not the finite-window mutual information estimate.
    See ``help(lzcomplexity.lz76)`` for details.
    """
    ...
