"""LZ76-based complexity analysis for symbolic sequences.

Top-level functions
-------------------
- ``factorization`` — complexity (factor count) and factor boundary list.
- ``h`` — normalized entropy density (entropy-rate estimator).
- ``emc`` — shuffle-based EMC-like measure (score and fitted increments).
- ``nid`` — normalized information distance between two sequences.
- ``lz76`` — the full analysis, returned as a dict with everything.

All sequence-accepting functions accept ``str``, ``bytes``, ``list[int]``,
``list[str]``, or any iterable of ints (e.g. NumPy arrays). For
``list[int]``, each element's decimal representation is concatenated:
``[0, 1, 10]`` becomes the symbolic string ``"0110"``.

The historical ``emc`` name denotes a magnitude qualitatively similar to
effective measure complexity, not an established numerical estimator of EMC
or excess entropy. Its score depends on length, shuffle scales and input
encoding; the fitted nonnegative increments do not make it mutual information.

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
