"""Type stubs for the `lzcomplexity` package.

These stubs mirror the runtime surface declared in :mod:`lzcomplexity.__init__`
and the compiled extension :mod:`lzcomplexity.lzcomplexity`. They exist so
editors and type-checkers (mypy, pyright, ruff) can show signatures and
docstrings without loading the compiled extension.
"""

from typing import Iterable, List, Tuple, Union

# A sequence-like input accepted by every `lz76*` function.
SeqLike = Union[str, bytes, List[int], List[str], Iterable[int]]
SignalLike = Union[List[float], Iterable[float]]

__version__: str

__all__ = [
    "lz76",
    "factorization",
    "factors",
    "entropy_density",
    "emc",
    "metrics",
    "spectral",
]


def lz76(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    max_block_size: int = -1,
    jobs: int = 0,
) -> Tuple[int, float, List[int], Tuple[int, float, float]]:
    """Full LZ76 analysis.

    Returns ``(complexity, entropy, factors, (max_block_size, emc_value,
    multi_information))``. See ``help(lzcomplexity.lz76)`` for parameter
    semantics.
    """
    ...


def factorization(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> int:
    """Return the LZ76 factor count of ``seq``."""
    ...


def factors(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> Tuple[int, List[int]]:
    """Return ``(complexity, factor_positions)`` for ``seq``."""
    ...


def entropy_density(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    jobs: int = 0,
) -> float:
    """Return the LZ76-based normalized entropy density of ``seq``."""
    ...


def emc(
    seq: SeqLike,
    partitions: int = 1,
    alphabet: int | None = None,
    log_base: int | None = None,
    max_block_size: int = -1,
    jobs: int = 0,
) -> Tuple[int, float, float]:
    """Effective measure complexity via random block shuffling.

    Returns ``(max_block_size, emc_value, multi_information)``.
    """
    ...


# ── Submodule stubs ─────────────────────────────────────────────────────────

class _MetricsModule:
    """Information distances between two sequences."""

    @staticmethod
    def nid(
        seq1: SeqLike,
        seq2: SeqLike,
        partitions: int = 1,
        alphabet: int = 2,
        log_base: int = 2,
        jobs: int = 0,
    ) -> float:
        """Normalized information distance between ``seq1`` and ``seq2``."""
        ...

    @staticmethod
    def rid(
        seq1: SeqLike,
        seq2: SeqLike,
        partitions: int = 1,
        alphabet: int = 2,
        log_base: int = 2,
        jobs: int = 0,
    ) -> float:
        """Random-shuffle information distance between ``seq1`` and ``seq2``."""
        ...


class _SpectralModule:
    """FFT-based spectral analysis."""

    @staticmethod
    def psd(
        signal: SignalLike,
        sample_frequency: int,
        use_complex: bool = True,
        cut: bool = False,
        step: int = 10,
        apply_window: bool = False,
        win: str = "hann",
    ) -> List[float]:
        """Power spectral density of ``signal`` via FFT."""
        ...

    @staticmethod
    def entropy(
        signal: SignalLike,
        sample_frequency: int,
        use_complex: bool = True,
        cut: bool = False,
        step: int = 10,
        apply_window: bool = False,
        win: str = "hann",
    ) -> float:
        """Shannon entropy of the normalized PSD."""
        ...

    @staticmethod
    def semc(
        signal: SignalLike,
        sample_frequency: int,
        block_size: int = 0,
        use_complex: bool = True,
        cut: bool = False,
        change_shuffle: bool = False,
        step: int = 10,
        apply_window: bool = False,
        win: str = "hann",
    ) -> float:
        """Spectral effective measure complexity."""
        ...


metrics: _MetricsModule
spectral: _SpectralModule
