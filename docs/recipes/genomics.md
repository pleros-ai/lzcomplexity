# DNA, RNA and FASTA

A genome is already symbolic, but the file around it is not: headers, line wrapping, lowercase letters and ambiguity codes can all change the bytes presented to LZ76. Decide what counts as one sequence before measuring it.

For example, `ACGT`, `acgt` and `ACGTN` are different byte alphabets. `N` is a symbol, not an instruction to skip an unknown base. An explicit `alphabet=4` does not remove ambiguity codes or convert them to four states.

## Read FASTA records explicitly in Python

This small reader separates records, joins wrapped sequence lines and normalizes case. It preserves `N` as a fifth possible symbol and rejects other characters instead of silently deleting them:

```python
import lzcomplexity as lz


def read_fasta(text, allowed=frozenset("ACGTN")):
    name = None
    parts = []
    records = []
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith(">"):
            if name is not None:
                records.append((name, "".join(parts).encode("ascii")))
            name = line[1:].strip() or f"record-{len(records) + 1}"
            parts = []
        else:
            if name is None:
                raise ValueError("Sequence data appears before a FASTA header")
            fragment = "".join(line.split()).upper()
            if not set(fragment) <= allowed:
                raise ValueError(f"Unexpected bases in {name}")
            parts.append(fragment)
    if name is not None:
        records.append((name, "".join(parts).encode("ascii")))
    return records


example = ">region-a\n" + "ACGT" * 1024 + "\n>region-b\n" + "ACGA" * 1024
records = read_fasta(example)
for name, sequence in records:
    if not sequence:
        raise ValueError(f"Empty record: {name}")
    result = lz.lz76(sequence, log_base=2, max_block_size=16)
    print(name, len(sequence), result["h"], result["emc"]["value"])

print("distance:", lz.nid(records[0][1], records[1][1]))
```

For a real file, replace `example` with `Path("regions.fasta").read_text(encoding="ascii")`, importing `Path` from `pathlib`. For RNA, pass `allowed=frozenset("ACGUN")` and decide whether conversion between `T` and `U` is appropriate for your comparison.

Deleting ambiguous bases connects neighbors that were separated in the original data. Alternatives include keeping ambiguity symbols, excluding whole windows containing them, or applying a documented imputation method. Each changes the question being measured.

## Match windows and conventions

Use the same sequence length, alphabet policy, case normalization, strand convention, logarithm base and block-size cap when comparing scores. A different region length changes both finite-sample LZ behavior and an automatic shuffle cutoff. Explicit fixed caps make that parameter visible; they do not remove all length dependence.

LZ-based information distance compares the supplied strings through their literal patterns. It is not an alignment, an edit distance or a phylogenetic model. A low value does not establish common ancestry or a shared biological mechanism.

## Reverse complements

If orientation should be ignored, construct a **full** reverse complement explicitly:

```python
DNA_COMPLEMENT = bytes.maketrans(b"ACGTN", b"TGCAN")


def reverse_complement(sequence):
    return sequence.translate(DNA_COMPLEMENT)[::-1]


a = records[0][1]
b = records[1][1]
d_same_orientation = lz.nid(a, b)
d_reverse_complement = lz.nid(a, reverse_complement(b))
d_orientation_free = min(d_same_orientation, d_reverse_complement)
print(d_same_orientation, d_reverse_complement, d_orientation_free)
```

This minimum defines your orientation-handling convention. Keep it consistent across every pair.

!!! note "The CLI's `-a` is a legacy transformation strategy"

    `lzdistance -a` tests the original second operand, its reversal, a partial `A↔T` substitution and its reversal, and a separate partial `C↔G` substitution and its reversal. The partial substitutions also lowercase the changed bases. They are not combined into a full reverse complement. Use the explicit Python construction above when that is what your analysis requires. See the [strategy reference](../cli/lzdistance.md).

## Native FASTA input

Both CLI tools have FASTA-related readers; `lzdistance` also recognizes common FASTA suffixes. The current reader's record joining, multiline and normalization conventions are documented in [input formats](../cli/formats.md). Check those rules before treating one file as a collection of biological records.

For the most explicit batch workflow, preprocess to one record per file, preserving a sorted manifest of record names. Then use [`lzdistance`](batch-distance.md) or the Python pairwise loop. Keep analysis outputs outside the input directory so a later run cannot accidentally ingest them.
