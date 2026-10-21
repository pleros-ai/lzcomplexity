"""LZ76-based complexity analysis for symbolic sequences.

Top-level functions
-------------------
- ``factorization`` — complexity (factor count) and factor boundary list.
- ``h`` — normalized entropy density (entropy-rate estimator).
- ``emc`` — finite-window excess-entropy estimate and its scale increments.
- ``nid`` — LZ76 phrase-count dissimilarity between two sequences.
- ``lz76`` — the full analysis, returned as a dict with everything.

All sequence-accepting functions accept ``str``, ``bytes``, ``list[str]``,
and integer sequences (e.g. lists, tuples, or one-dimensional NumPy arrays).
Generators and iterators are not accepted. Integer elements' decimal
representations are concatenated: ``[0, 1, 10]`` becomes ``"0110"``.
Prefer ``bytes`` for one symbol per category; strings use UTF-8 bytes.

``emc`` uses empirical mutual information between adjacent windows, without
random shuffling. Its finite-window target approaches excess entropy for a
stationary source, but sample estimates can be biased and miss long memory.

Use ``help(lzcomplexity.<name>)`` on any of the names above for full
parameter docs.

.. note::
   Spectral analysis (``psd``, spectral ``entropy``, ``semc``) was removed
   from this library and now lives in a separate package.
"""

from .lzcomplexity import (
    __version__,
    lz76,
    factorization,
    h,
    emc,
    nid,
)

__all__ = [
    "lz76",
    "factorization",
    "h",
    "emc",
    "nid",
]

# Hide the compiled-extension submodule from `dir(lzcomplexity)` and from
# `import lzcomplexity; lzcomplexity.lzcomplexity` — the public surface is the
# names listed in `__all__` above.
del globals()["lzcomplexity"]
