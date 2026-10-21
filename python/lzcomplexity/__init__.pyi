"""Type stubs for the `lzcomplexity` package.

These stubs mirror the runtime surface declared in :mod:`lzcomplexity.__init__`
and the compiled extension :mod:`lzcomplexity.lzcomplexity`. They exist so
editors and type-checkers (mypy, pyright, ruff) can show signatures and
docstrings without loading the compiled extension.
"""

from typing import Any, Dict, Iterable, List, Tuple, Union

# A sequence-like input accepted by every `lz76*` function.
SeqLike = Union[str, bytes, List[int], List[str], Iterable[int]]

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
    """Return ``(complexity, factor_boundary_indices)`` for ``seq``."""
    ...


def h(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> float:
    """Return the LZ76-based normalized entropy density (entropy rate) of ``seq``."""
    ...


def emc(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    max_block_size: int = -1,
    jobs: int = 0,
) -> Tuple[float, List[float]]:
    """Effective measure complexity via random block shuffling.

    Returns ``(emc_value, summands)``. Both the value and every summand are
    non-negative, and the summands sum to the value; ``summands[l-1]``
    estimates the scale-``l`` conditional-entropy excess ``h(l) - h``.

    Read the value as an ordinal index at fixed sequence length, not as a bit
    count, and never compare it across lengths.
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
    """Normalized information distance (NID) between ``seq1`` and ``seq2``."""
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
    ``extras`` (a dict). See ``help(lzcomplexity.lz76)`` for details.
    """
    ...
