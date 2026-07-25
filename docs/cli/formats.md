# Input formats

*Exactly which bytes of your file become which symbols, for every reader the CLI ships.*

Both binaries share one input layer. Given a path, it decides on a format, runs the matching
reader, and hands the resulting sequences to the LZ76 engine. Nothing downstream ever sees the file
again — if the reader picks the wrong bytes, every number in the report is wrong, and nothing warns
you.

This page documents the whole layer: the resolution order, each reader byte-for-byte, and the
places where the result will surprise you.

<div class="lz-stats" markdown>
<div class="lz-stat" markdown>
<span class="lz-stat__v">15</span>
<span class="lz-stat__k">Formats in the enum</span>
</div>
<div class="lz-stat" markdown>
<span class="lz-stat__v">17</span>
<span class="lz-stat__k">Names accepted by -F</span>
</div>
<div class="lz-stat" markdown>
<span class="lz-stat__v">4</span>
<span class="lz-stat__k">Bytes the classifier reads</span>
</div>
<div class="lz-stat" markdown>
<span class="lz-stat__v">3</span>
<span class="lz-stat__k">Bytes actually tested</span>
</div>
</div>

The Python API does **not** go through this layer — it takes a sequence you already have in memory.
See [Python inputs](../api/inputs.md) for that path.

<hr class="lz-tickrule">

## Resolution order

Three steps, in this order. The first one that produces something other than `AUTO` wins.

| # | Step | Decided by |
|---|---|---|
| 1 | The explicit format name from `-F` (`lzcomplexity`) or `-I` / `-S` (`lzdistance`) | `parse_format` |
| 2 | The lower-cased **final** file extension | a per-tool table, below |
| 3 | A peek at the first four bytes of the file | `file_type_q` |

An unrecognised name in step 1 is **silently** treated as `AUTO`, so a typo falls through to steps
2 and 3 rather than raising an error.

### Step 1 — the accepted format names

Case-insensitive. Seventeen names map onto eleven formats.

| Name(s) | Format |
|---|---|
| `text`, `txt`, `rawtxt` | raw text |
| `raw`, `bin`, `rawbin` | raw binary |
| `csv` | CSV (comma) |
| `tcsv` | TSV (**space**) |
| `fasta` | FASTA |
| `dna` | DNA |
| `rna` | RNA |
| `pbmtxt` | PBM plain (P1) |
| `pbm`, `pbmbin` | PBM raw (P4) |
| `pgmtxt` | PGM plain (P2) |
| `pgm`, `pgmbin` | PGM raw (P5) |
| *anything else* | `AUTO` |

!!! warning "`-F tsv` and `-F pnm` do nothing, despite appearing in `--help`."

    Both names fall through to `AUTO`, and the file is then classified by its magic bytes — so you
    get whatever the peek decides, usually raw text or raw binary, not a table. The help line reads
    `Input file format (TXT, CSV, TSV, PBM, PGM, PNM, DNA, RNA, FASTA)`; `TSV` and `PNM` are not in
    the parser. The real name for space-delimited data is `tcsv`.

    ```console
    $ lzcomplexity -n -m table_sp.dat -F tsv     # -> "format":"AUTO",  5 text lines
    $ lzcomplexity -n -m table_sp.dat -F pnm     # -> "format":"AUTO",  5 text lines
    $ lzcomplexity -n -m table_sp.dat -F tcsv    # -> "format":"CSV",   3 columns
    ```

    `table_sp.dat` is a 5-row, 3-column space-delimited table. Only the third command reads it as
    a table.

There is no name for P3, P6 or P7. Those formats are reachable only through magic detection, and
they produce an empty sequence either way.

### Step 2 — extension rules, which differ between the two tools

Only three extensions are mapped, and `lzdistance` is missing one of them.

| Extension (lower-cased) | `lzcomplexity` | `lzdistance` |
|---|---|---|
| `fna`, `fasta`, `gz` | FASTA | FASTA |
| `csv` | CSV | CSV |
| `tsv` | TSV (space-delimited) | **not mapped** → `AUTO` |
| anything else | `AUTO` | `AUTO` |

Verified on one space-delimited file copied to `table.tsv`:

<div class="lz-run" markdown>

```console
$ lzcomplexity -n table.tsv       # "format":"CSV",  1 sequence  (first column, 5 symbols)
$ lzdistance table.tsv            # "first_data_source_format":"AUTO",  first_dim = 5 text lines
$ lzdistance table.tsv -I tcsv    # "CSV",  first_dim = 3 columns
```

</div>

Directory sources in `lzdistance` skip step 2 entirely — there is no single path to take an
extension from, so `-I` / `-S` (or the magic peek) is the only control you have over how the files
inside are read.

### Step 3 — the magic peek

`read_input` reads up to eight bytes, then classifies using at most the first four of them.

<div class="lz-formula">
  <p class="lz-math">read bytes until <i>b</i> = ␊ or <i>n</i> = 4, then test <i>c</i><sub>0</sub>, <i>c</i><sub>1</sub>, <i>c</i><sub>2</sub></p>
  <dl class="lz-formula__key">
    <dt>PNM</dt><dd><i>c</i><sub>0</sub> is <code>P</code> or <code>p</code> <b>and</b> <i>c</i><sub>1</sub> is not an ASCII letter. Then <i>c</i><sub>1</sub> ∈ <code>1</code>…<code>7</code> selects P1…P7; anything else falls back to raw binary.</dd>
    <dt>text</dt><dd>otherwise, if <i>c</i><sub>0</sub>, <i>c</i><sub>1</sub> and <i>c</i><sub>2</sub> are each alphanumeric, space, tab or newline → raw text.</dd>
    <dt>else</dt><dd>raw binary.</dd>
  </dl>
  <p class="lz-formula__cite">Ports the C++ <code>istream::get(c, 5)</code>, including the NUL it writes after a short first line. One deliberate divergence: where the C++ threw on a <code>P</code> magic outside <code>1</code>…<code>7</code>, the port falls back to raw binary.</p>
</div>

The scan stops at the first newline and writes a NUL there. A first line shorter than three
characters therefore fails the text test on <i>c</i><sub>2</sub> and the file is classified as
**raw binary**.

Measured classifications. Raw binary is recognisable because its sequence length is
`(filesize − 1) × 8`.

| First bytes | Detected | Evidence |
|---|---|---|
| `0101…` | raw text | `0101\n…` → 4 symbols |
| `010\n…` (3 characters) | raw text | 3 symbols |
| `01\n…` (2 characters) | **raw binary** | 10-byte file → 72 symbols |
| `\n0101…` (leading blank line) | **raw binary** | 11-byte file → 80 symbols |
| `#0101\n…` (leading comment) | **raw binary** | 11-byte file → 80 symbols |
| `>seq1…` (FASTA, unknown extension) | **raw binary** | 48-byte file → 376 symbols |
| ` 0101\n` (leading space) | raw text | space passes the test |
| `\t0101\n` (leading tab) | raw text | tab passes the test |
| `P1 …`, `p1 …` | P1 | lower-case `p` is accepted |
| `P8\n…` | **raw binary** | not in `1`…`7`; 8-byte file → 56 symbols |
| `P 1\n…` | **raw binary** | `' '` is not a letter, so the PNM branch is taken, then `' '` is not `1`…`7`; 8-byte file → 56 symbols |
| `Pabc…` | raw text | <i>c</i><sub>1</sub> is a letter, so this is not a PNM magic; 4 symbols |
| any 1-byte file | raw binary | 8 symbols |
| any 0-byte file | raw binary | the NUL terminator fails the text test; 0 symbols |

!!! warning "A text file that opens with a blank line, a `#`, a `>`, or a line under three characters is read as a bitstream."

    The report still looks plausible — a sequence of `0`/`1` symbols, a complexity, an entropy
    density — but the length is `(filesize − 1) × 8` instead of your line length, and the symbols
    are the file's bits rather than its characters. Pass `-F txt` (or `-F fasta` for FASTA)
    whenever the first line of your file is not at least three plain alphanumeric characters.

<hr class="lz-tickrule">

## The readers

### Raw text — `PNM_RAWTXT`

Lines are split on `\n` only; each line is then trimmed of ASCII whitespace at both ends, which
includes `\r`, so **CRLF files are handled correctly**. Lines that are empty after trimming are
skipped.

| Mode | Result |
|---|---|
| default | the **first non-blank line**, trimmed, as one sequence |
| `-m` | **every** non-blank line, each trimmed, as its own sequence |

Symbols are the bytes of the line. For the line `0101` that means the bytes `48` and `49`
(ASCII `'0'` and `'1'`), not the integers 0 and 1.

!!! warning "Bytes that are not valid UTF-8 are replaced, not preserved."

    The text and CSV readers decode the file lossily before splitting it, so every invalid byte
    becomes the three bytes of U+FFFD (`239, 191, 189`). A Latin-1 line is therefore longer than the
    file says, over a larger alphabet, with no diagnostic.

    ```console
    $ printf 'ab\xe9cd\nxyz\n' > latin1.txt          # first line is 5 bytes
    $ lzcomplexity -n -F txt latin1.txt -o l.json    # "size":7
    #   alphabet [-17, -65, -67, 100, 99, 98, 97] — the 0xe9 became ef bf bd
    ```

    Only the raw-binary and P4/P5 readers are byte-exact. Transcode to UTF-8 (or use `-F raw`)
    before measuring anything that is not already ASCII.

!!! danger "Comment lines are data. `#` and `>` lines become sequences of their own under `-m`."

    They are factorized and reported like any other line, inflating `size` and adding entries whose
    alphabet is English prose. The C++ original had a `#`-comment `continue` that was a no-op — it
    re-tested a condition that was already false — and the Rust port reproduces the no-op faithfully
    rather than fixing it. Strip comments before handing a file to the CLI.

!!! example "Five lines in, five sequences out, two of them prose"

    ```console
    $ printf '0101\n# a comment\n1100\n> header\n1111\n' > cmt.txt
    $ lzcomplexity -n -m cmt.txt -o cmt.json
    ```

    | # | Line | `size` | `alphabet` (bytes, sorted descending) |
    |---|---|---|---|
    | 1 | `0101` | 4 | `[49, 48]` |
    | 2 | `# a comment` | 11 | `[116, 111, 110, 109, 101, 99, 97, 35, 32]` |
    | 3 | `1100` | 4 | `[49, 48]` |
    | 4 | `> header` | 8 | `[114, 104, 101, 100, 97, 62, 32]` |
    | 5 | `1111` | 4 | `[49]` |

    Entries 2 and 4 are the comment and the header line, read as text.

### Raw binary — `PNM_RAWBIN`

The whole file is expanded into a bit sequence, **most significant bit first**, and the symbols are
the integers `0` and `1` — byte values, not ASCII characters. The number of bits is not
`filesize × 8`:

<div class="lz-formula">
  <p class="lz-math"><i>n</i><sub>bits</sub> = (<i>N</i> − 1) · 8 &nbsp;for <i>N</i> &gt; 1, &nbsp;&nbsp; <i>n</i><sub>bits</sub> = 8 &nbsp;for <i>N</i> = 1, &nbsp;&nbsp; <i>n</i><sub>bits</sub> = 0 &nbsp;for <i>N</i> = 0</p>
  <dl class="lz-formula__key">
    <dt><i>N</i></dt><dd>file size in bytes</dd>
    <dt><i>n</i><sub>bits</sub></dt><dd>symbols produced</dd>
  </dl>
  <p class="lz-formula__cite">A bit-for-bit reproduction of the C++ <code>fsize = (filesize − 1) · CHARBITS</code>. The 1-byte case is the C++ <code>size == 0</code> branch, which emitted all 8 bits.</p>
</div>

!!! danger "The last byte of every raw-binary file larger than one byte is dropped."

    Two files that differ only in their final byte produce identical reports — same length, same
    complexity, same entropy density — with no warning. A 4-byte file yields 24 symbols, not 32.
    If the last byte matters to your analysis, append a throwaway byte to the file or convert it to
    text before measuring.

    ```console
    $ printf '\x0f\x00\xff\xaa' > bin_x.bin
    $ printf '\x0f\x00\xff\x55' > bin_y.bin
    $ lzcomplexity -n bin_x.bin -o x.json    # "size":24
    $ lzcomplexity -n bin_y.bin -o y.json    # "size":24 — and every other field identical
    ```

Raw binary also **ignores `-m`**: the whole file is always exactly one sequence, whatever you pass.

### CSV and TSV — one reader, two delimiters

The first non-empty line seeds one accumulator per column; every subsequent non-empty line appends
its cells to those accumulators. A single trailing `\r` is stripped from each line, so CRLF input is
safe. Cells themselves are **not** trimmed — leading and trailing spaces inside a cell become
symbols. Like the text reader, this one decodes lossily, so non-UTF-8 cell bytes turn into U+FFFD.

| `-F` name | Delimiter |
|---|---|
| `csv` | `,` |
| `tcsv` | `' '` — a **space**, not a tab |

Feeding a genuinely tab-separated file through `-F tcsv` gives you one giant "column" containing the
literal tab bytes. Convert tabs to spaces first.

!!! warning "The header row is data, and without `-m` only the first column is read."

    A column that should read `0110` comes back as `A0110` — five symbols over a three-symbol
    alphabet — because the header cell is prepended to the column it names. There is no
    header-skip option. Delete the header row before measuring if you do not want it counted.

!!! example "One 4-row CSV with a header, with and without `-m`"

    ```console
    $ printf 'A,B,C\n0,1,2\n1,0,2\n1,1,0\n0,0,1\n' > table.csv
    $ lzcomplexity -n -m table.csv -o multi.json    # "format":"CSV", "size":3
    $ lzcomplexity -n    table.csv -o single.json   # "format":"CSV", "size":1
    ```

    | Mode | Sequences | Contents |
    |---|---|---|
    | `-m` | 3 | `A0110`, `B1010`, `C2201` |
    | default | 1 | `A0110` |

Ragged rows are tolerated: cells missing from a short row are absent for that row, and the
column keeps growing from the rows that do have them. `A,B,C` / `0,1` / `1` / `2,2,2` under `-m`
yields columns of 4, 3 and 2 symbols — `A012`, `B12`, `C2`.

An empty file read as CSV produces **zero** sequences — `{"sequences":[],"size":0}` — rather than
one empty sequence.

### FASTA, DNA and RNA — the same reader three times

`fasta`, `dna` and `rna` route to an identical reader. There is no U↔T handling, no alphabet
validation and no complement logic anywhere in it; the three names differ only in the ordinal
written to the factors file.

| Input line | Effect |
|---|---|
| starts with `>` **or `;`** after trimming | closes the current record and opens a new one; the header text is discarded |
| anything else | trimmed and appended byte-for-byte to the current record |

Line wrapping is therefore transparent. Without `-m` the reader stops after the **first** record.
Data appearing before any header line starts an implicit record, so a headerless file yields
exactly **one** sequence even under `-m`.

Gzip is decompressed **if and only if the path extension is exactly `gz`**, compared
case-sensitively — the format name plays no part in that decision. A file named `.GZ` still routes
to the FASTA reader (that extension test *is* case-folded) but is handed to it still compressed, and
fails. Three failure modes follow, all fatal:

```console
$ lzcomplexity -n nogzext.dat -F fasta    # gzip bytes, no .gz extension
 [ Error ] stream did not contain valid UTF-8            exit 1
$ lzcomplexity -n fake.gz                 # .gz extension, plain text inside
 [ Error ] invalid gzip header                           exit 1
$ lzcomplexity -n GENES.FNA.GZ            # upper-case extension, real gzip
 [ Error ] stream did not contain valid UTF-8            exit 1
```

The reader uses line-based UTF-8 I/O, so **non-UTF-8 input is a hard error**, never a fallback.

<div class="lz-run" markdown>

```console
$ lzcomplexity -n    genes.fasta -o a.json          # "format":"",     size 1, [20]   first record only
$ lzcomplexity -n -m genes.fasta -o b.json          # "format":"",     size 2, [20, 14]
$ lzcomplexity -n    genes.fna.gz -o c.json         # "format":"",     size 1, [20]   gunzipped
$ lzcomplexity -n    genes.dat -o d.json            # "format":"AUTO", size 1, [376]  read as a bitstream
$ lzcomplexity -n    genes.dat -F fasta -o e.json   # "format":"",     size 1, [20]
```

</div>

`genes.dat` is the same FASTA file under an unmapped extension: 48 bytes, so the magic peek sends it
to the raw-binary reader and you get `(48 − 1) × 8 = 376` bit symbols. Always name the format for
FASTA files that are not `.fasta`, `.fna` or `.gz`.

### PBM — P1 (plain) and P4 (raw)

Header parsing is shared by all four PNM readers: skip the two magic bytes, then read the required
integers (2 for PBM, 3 for PGM), skipping ASCII whitespace and `#`-to-end-of-line comments.
Dimensions on the same physical line as the magic are handled — `P1 2 2\n0 1\n1 0\n` gives four
pixels. A header with no parseable integer where one is expected yields **one empty sequence**
(measured: `P1\nnotanumber\n1 0\n` → `size` 0, `alphabet` `[]`).

Pixels become the integers `0` and `1`; `total = width × height`.

| Variant | Raster handling |
|---|---|
| **P1** (plain) | scan the raster text, push `1` for every `'1'` and `0` for every `'0'`, stop at `total` |
| **P4** (raw) | read the raster as one contiguous MSB-first bitstream with **no per-row byte padding**; if the data runs out, pad with `1` |

Measured: a P4 declaring `8 2` (16 pixels) but supplying a single byte returns 16 symbols with
alphabet `[1, 0]` — the missing eight are the padding.

!!! warning "P4 rasters are read without row padding, which is not what the PBM spec says."

    A width that is a multiple of 8 reads correctly; any other width desynchronises row by row,
    because the spec pads each row to a byte boundary and this reader does not. It reproduces the
    C++ `ReadBin(is, s, width*height)` call it was ported from. For non-multiple-of-8 widths,
    convert to P1 first.

### PGM — P2 (plain) and P5 (raw)

Each pixel's grey value becomes one symbol byte; `total = width × height`.

| Variant | Raster handling |
|---|---|
| **P2** (plain) | whitespace-split the raster, parse each token as an integer, push its **low 8 bits** |
| **P5** (raw) | take `total` raw bytes; if short, pad with `0xff` |

!!! danger "`maxvalue` is parsed and then ignored — 16-bit samples are truncated to their low 8 bits."

    Nothing is rescaled. A P2 image with `maxvalue` 65535 and samples `256 257 511 300` returns the
    symbols `0, 1, 255, 44`: distinct grey levels collapse onto each other, and the complexity you
    measure is the complexity of the truncated image, not the original. Down-convert to 8 bits
    yourself before measuring.

Measured on the P2 image `4 3` / `255` / `0 64 128 255` / `255 128 64 0` / `10 20 30 40`:

| Mode | Sequences | `size` | `alphabet` |
|---|---|---|---|
| default | 1 | 12 | `[-1, -128, 64, 40, 30, 20, 10, 0]` |
| `-m` | 3 | 4, 4, 4 | `[-1, -128, 64, 0]`, `[-1, -128, 64, 0]`, `[40, 30, 20, 10]` |

The equivalent P5 file produces the same sequences — same sizes, same alphabets, same complexity.
Only `filename` and `format` (`PNM_P5` instead of `PNM_P2`) differ. The negative entries are not a bug: the
`alphabet` array is serialised as signed 8-bit, so 255 prints as `-1` and 128 as `-128`. See
[Alphabets](../concepts/alphabets.md).

### PPM and PAM — P3, P6, P7

There is no reader for any of the three. All of them return **one empty sequence**, and the tool
exits 0 with a well-formed report.

!!! danger "A PPM or PAM file yields a valid-looking report about nothing, and exits 0."

    You get `"size":0`, `"alphabet":[]`, `"lz76EntropyDensity":0.0` and no diagnostic on stdout or
    stderr — the only signal that anything went wrong is the zero length. The C++ `ReadPNM` switch
    threw for these cases, leaving one default-constructed sequence behind; the port returns that
    same empty sequence rather than an error. Convert colour images to PGM before measuring.

    ```console
    $ lzcomplexity -n img.ppm -o ppm.json ; echo $?
    0
    $ cat ppm.json
    {"filename":"img.ppm","format":"AUTO","sequences":[{"alphabet":[],"alphabet_size":2,
    "lz76Complexity":1,"lz76EntropyDensity":0.0,"lz76RandomShuffleComplexity":
    {"max_block_size":-1,"multi_information":0.0,"value":0.0},"size":0}],"size":1}
    ```

    Wrapped over three lines here for the page; the real file is one compact line with no trailing
    newline.

<hr class="lz-tickrule">

## Support matrix

<div class="lz-scroll">
<table class="lz-compare">
<thead>
<tr>
  <th>Format</th>
  <th><code>-F</code> / <code>-I</code> / <code>-S</code> name(s)</th>
  <th>Magic</th>
  <th>Extension</th>
  <th>What <code>-m</code> splits on</th>
  <th>Symbol values</th>
  <th>JSON <code>format</code></th>
  <th>Ordinal</th>
</tr>
</thead>
<tbody>
<tr><td>Raw text</td><td><code>text</code>, <code>txt</code>, <code>rawtxt</code></td><td class="is-yes"></td><td class="is-no"></td><td>non-blank line</td><td>line bytes</td><td><code>PNM_RAWTXT</code></td><td>7</td></tr>
<tr><td>Raw binary</td><td><code>raw</code>, <code>bin</code>, <code>rawbin</code></td><td class="is-yes"></td><td class="is-no"></td><td class="is-no"></td><td><code>0</code> / <code>1</code></td><td><code>PNM_RAWBIN</code></td><td>8</td></tr>
<tr><td>CSV (comma)</td><td><code>csv</code></td><td class="is-no"></td><td><code>.csv</code></td><td>column</td><td>cell bytes</td><td><code>CSV</code></td><td>9</td></tr>
<tr><td>TSV (space)</td><td><code>tcsv</code></td><td class="is-no"></td><td><code>.tsv</code>, <code>lzcomplexity</code> only</td><td>column</td><td>cell bytes</td><td><code>CSV</code></td><td>10</td></tr>
<tr><td>DNA</td><td><code>dna</code></td><td class="is-no"></td><td class="is-no"></td><td>FASTA record</td><td>residue bytes</td><td><i>empty string</i></td><td>11</td></tr>
<tr><td>RNA</td><td><code>rna</code></td><td class="is-no"></td><td class="is-no"></td><td>FASTA record</td><td>residue bytes</td><td><i>empty string</i></td><td>12</td></tr>
<tr><td>FASTA</td><td><code>fasta</code></td><td class="is-no"></td><td><code>.fasta</code>, <code>.fna</code>, <code>.gz</code></td><td>FASTA record</td><td>residue bytes</td><td><i>empty string</i></td><td>13</td></tr>
<tr><td>PBM plain (P1)</td><td><code>pbmtxt</code></td><td class="is-yes"></td><td class="is-no"></td><td>image row</td><td><code>0</code> / <code>1</code></td><td><code>PNM_P1</code></td><td>0</td></tr>
<tr><td>PBM raw (P4)</td><td><code>pbm</code>, <code>pbmbin</code></td><td class="is-yes"></td><td class="is-no"></td><td>image row</td><td><code>0</code> / <code>1</code></td><td><code>PNM_P4</code></td><td>3</td></tr>
<tr><td>PGM plain (P2)</td><td><code>pgmtxt</code></td><td class="is-yes"></td><td class="is-no"></td><td>image row</td><td>sample &amp; 0xff</td><td><code>PNM_P2</code></td><td>1</td></tr>
<tr><td>PGM raw (P5)</td><td><code>pgm</code>, <code>pgmbin</code></td><td class="is-yes"></td><td class="is-no"></td><td>image row</td><td>raw grey byte</td><td><code>PNM_P5</code></td><td>4</td></tr>
<tr><td>PPM plain (P3)</td><td class="is-no"></td><td class="is-yes"></td><td class="is-no"></td><td class="is-no"></td><td><i>none, empty sequence</i></td><td><code>PNM_P3</code></td><td>2</td></tr>
<tr><td>PPM raw (P6)</td><td class="is-no"></td><td class="is-yes"></td><td class="is-no"></td><td class="is-no"></td><td><i>none, empty sequence</i></td><td><code>PNM_P6</code></td><td>5</td></tr>
<tr><td>PAM (P7)</td><td class="is-no"></td><td class="is-yes"></td><td class="is-no"></td><td class="is-no"></td><td><i>none, empty sequence</i></td><td><code>PNM_P7</code></td><td>6</td></tr>
<tr><td>AUTO (unresolved)</td><td><i>any unknown name</i></td><td class="is-no"></td><td class="is-no"></td><td class="is-no"></td><td class="is-no"></td><td><code>AUTO</code></td><td>14</td></tr>
</tbody>
</table>
</div>

The **Magic** column means the format can be reached by the byte peek; **Extension** means it can be
reached by a filename rule. A format with neither can only be selected explicitly.

<hr class="lz-tickrule">

## What the JSON says about the format, and what it hides

Three quirks, all deliberate, all kept so the Rust output stays byte-comparable with the C++ tool's.

**The reported format is the name/extension decision, not the magic decision.** A plain `.txt` file
always reports `"format":"AUTO"` even though it was read as raw text. The field records steps 1 and
2 only; step 3 leaves no trace in the report.

**`CSV` and `TSV` both print as `"CSV"`.** The string is ambiguous — the file may have been split on
a comma or on a space. The C++ `MagicValues` map had no separate TSV entry.

**DNA, RNA and FASTA all print as the empty string `""`.** The C++ map had no entries for them, and
`nlohmann::json` returned `""` for the missing key.

!!! tip "The factors file records the format more precisely than the report does — at the cost of one extra factorization pass per sequence."

    `-f` writes the enum **ordinal** instead of the string, and the ordinal is the only place the
    CSV/TSV distinction (9 vs 10) and the DNA/RNA/FASTA distinction (11 vs 12 vs 13) survive into
    the output, where the report writes `"CSV"` and `""`.

    ```console
    $ lzcomplexity -n data.dat -F tcsv -f fac.json -o out.json
    $ cat fac.json    # {"factors":[[…]],"filename":"data.dat","format":10,"size":1}
    $ cat out.json    # …,"format":"CSV",…
    ```

Ordinals in full: `0` P1, `1` P2, `2` P3, `3` P4, `4` P5, `5` P6, `6` P7, `7` raw text,
`8` raw binary, `9` CSV, `10` TSV, `11` DNA, `12` RNA, `13` FASTA, `14` AUTO.

!!! note "`formats::multiline_to_one` is public but unused."

    The CLI crate exports a helper that concatenates every line of a file into one sequence,
    skipping blank lines and lines starting with `#` or `>` — a port of the C++
    `multiLineToOne(concatenate=true)`. No binary in this repository calls it. It is mentioned here
    only so you do not go looking for a flag that exposes it.

<hr class="lz-tickrule">

## Checklist before you trust a report

| Check | Why |
|---|---|
| Does `size` match the length you expect? | The commonest symptom of a misdetection is a length of `(filesize − 1) × 8`. |
| Is your first line at least three plain characters? | Otherwise the file is read as a bitstream. Pass `-F txt`. |
| Did you mean the header row to be data? | CSV and TSV prepend it to the column it names. |
| Did you pass `-m`? | Without it, CSV reads column 1 only and FASTA reads record 1 only. |
| Is your data tab-separated? | `-F tcsv` splits on **spaces**. Convert first. |
| Is every byte valid UTF-8? | The text and CSV readers replace the rest with U+FFFD, three bytes each. |
| Does the last byte matter? | The raw-binary reader drops it. |
| Is the image PPM or PAM? | You get an empty sequence and exit 0. |
| Is the FASTA file named `.fasta`, `.fna` or `.gz`? | If not, name the format explicitly. |

## See also

- [`lzcomplexity`](lzcomplexity.md) — `-F` and `-m` in the context of the full flag set.
- [`lzdistance`](lzdistance.md) — `-I` / `-S`, and how directory sources read one sequence per file.
- [CLI overview](index.md) — which of the two tools to reach for.
- [Python inputs](../api/inputs.md) — the in-memory path, which shares none of this machinery.
- [Alphabets](../concepts/alphabets.md) — why `alphabet` entries can be negative.
