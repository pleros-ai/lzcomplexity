//! Input format detection and readers — a faithful port of the C++
//! `pnm.cpp` readers + `config_utils.hpp` (`read_input`, `read_csv`,
//! `read_dna`, `FileTypeQ`).
//!
//! Every reader turns a file into one or more [`Sequence`]s whose bytes match
//! what the C++ standalone would have produced, so the downstream LZ76
//! computations (already proven equal to the C++ core) yield identical results.

use std::fs;
use std::io::{self, BufRead, Read};
use std::path::Path;

use flate2::read::MultiGzDecoder;
use lzcomplexity_core::Sequence;

/// Mirror of the C++ `MagickNumber` enum (same ordinal values).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Format {
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    P7,
    RawTxt,
    RawBin,
    Csv,
    Tsv,
    Dna,
    Rna,
    Fasta,
    Auto,
}

impl Format {
    /// Name used in JSON output — mirrors the C++ `MagicValues` map. Note the
    /// C++ map has no entries for DNA/RNA/FASTA (nlohmann returns ""), and maps
    /// both CSV and TSV to the string "CSV".
    pub fn magic_name(self) -> &'static str {
        match self {
            Format::P1 => "PNM_P1",
            Format::P2 => "PNM_P2",
            Format::P3 => "PNM_P3",
            Format::P4 => "PNM_P4",
            Format::P5 => "PNM_P5",
            Format::P6 => "PNM_P6",
            Format::P7 => "PNM_P7",
            Format::RawTxt => "PNM_RAWTXT",
            Format::RawBin => "PNM_RAWBIN",
            Format::Csv => "CSV",
            Format::Tsv => "CSV",
            Format::Auto => "AUTO",
            Format::Dna | Format::Rna | Format::Fasta => "",
        }
    }

    /// Ordinal value of the matching C++ `MagickNumber` enum. The C++
    /// `save_factors` writes the raw enum (an int), unlike `save_data` which
    /// writes the `MagicValues` string.
    pub fn magic_ordinal(self) -> u32 {
        match self {
            Format::P1 => 0,
            Format::P2 => 1,
            Format::P3 => 2,
            Format::P4 => 3,
            Format::P5 => 4,
            Format::P6 => 5,
            Format::P7 => 6,
            Format::RawTxt => 7,
            Format::RawBin => 8,
            Format::Csv => 9,
            Format::Tsv => 10,
            Format::Dna => 11,
            Format::Rna => 12,
            Format::Fasta => 13,
            Format::Auto => 14,
        }
    }
}

/// Mirror of `detail::parseFormat` in `config_complexity.hpp`.
pub fn parse_format(s: &str) -> Format {
    match s.to_ascii_lowercase().as_str() {
        "pbm" | "pbmbin" => Format::P4,
        "pbmtxt" => Format::P1,
        "pgm" | "pgmbin" => Format::P5,
        "pgmtxt" => Format::P2,
        "raw" | "bin" | "rawbin" => Format::RawBin,
        "text" | "txt" | "rawtxt" => Format::RawTxt,
        "csv" => Format::Csv,
        "tcsv" => Format::Tsv,
        "dna" => Format::Dna,
        "rna" => Format::Rna,
        "fasta" => Format::Fasta,
        _ => Format::Auto,
    }
}

/// Mirror of `pnm::FileTypeQ`: peek the first (up to) 4 bytes, stopping at a
/// newline, and classify. Bytes past the newline / end are treated as the
/// sentinel `'A'` the C++ pre-fills, except the C++ null-terminates what it
/// reads (so a short first line leaves `'\0'` in `c[2]`).
pub fn file_type_q(bytes: &[u8]) -> Format {
    let mut c = [b'A'; 5];
    // istream::get(c, 5) reads up to 4 chars, stops before '\n', null-terminates.
    let mut n = 0usize;
    for &b in bytes.iter() {
        if b == b'\n' || n >= 4 {
            break;
        }
        c[n] = b;
        n += 1;
    }
    c[n] = 0; // null terminator written by is.get

    let is_alpha = |x: u8| x.is_ascii_alphabetic();
    let ok = |x: u8| x.is_ascii_alphanumeric() || x == b' ' || x == b'\t' || x == b'\n';

    if (c[0] == b'P' || c[0] == b'p') && !is_alpha(c[1]) {
        match c[1] {
            b'1' => Format::P1,
            b'2' => Format::P2,
            b'3' => Format::P3,
            b'4' => Format::P4,
            b'5' => Format::P5,
            b'6' => Format::P6,
            b'7' => Format::P7,
            _ => Format::RawBin, // C++ throws; we fall back to binary
        }
    } else if ok(c[0]) && ok(c[1]) && ok(c[2]) {
        Format::RawTxt
    } else {
        Format::RawBin
    }
}

fn trim(s: &str) -> &str {
    s.trim_matches(|c: char| c.is_ascii_whitespace())
}

// ── RAW readers ─────────────────────────────────────────────────────────────

/// `ReadRAW` text, single sequence: the first non-blank line, byte-for-byte.
fn read_raw_txt_single(bytes: &[u8]) -> Sequence {
    let text = String::from_utf8_lossy(bytes);
    for line in text.split('\n') {
        let t = trim(line);
        if !t.is_empty() {
            return Sequence::from_bytes(t.as_bytes().to_vec());
        }
    }
    Sequence::new()
}

/// `ReadRAW` text, multi sequence: every non-blank line → a sequence. The C++
/// loop only skips blank lines; its `#`-comment `continue` is a no-op (it
/// re-tests `line.size()==0`, already false), so comment lines ARE emitted.
fn read_raw_txt_multi(bytes: &[u8]) -> Vec<Sequence> {
    let text = String::from_utf8_lossy(bytes);
    let mut out = Vec::new();
    for line in text.split('\n') {
        let t = trim(line);
        if t.is_empty() {
            continue;
        }
        out.push(Sequence::from_bytes(t.as_bytes().to_vec()));
    }
    out
}

/// `ReadBin` / RAW binary: expand the file's bytes into a bit sequence.
/// The C++ uses `fsize = (filesize - 1) * CHARBITS` bits, reading each byte
/// most-significant-bit first and pushing 0/1 byte symbols.
fn read_raw_bin(bytes: &[u8]) -> Sequence {
    if bytes.is_empty() {
        return Sequence::new();
    }
    // For a 1-byte file the C++ `fsize = (1-1)*CHARBITS = 0`, which trips the
    // `size==0` branch in `ReadBin` and emits the 8 bits of that single byte.
    let nbits = if bytes.len() == 1 {
        8
    } else {
        (bytes.len() - 1) * 8
    };
    let mut data = Vec::with_capacity(nbits);
    'outer: for &byte in bytes {
        for bit in (0..8).rev() {
            if data.len() >= nbits {
                break 'outer;
            }
            data.push((byte >> bit) & 1);
        }
    }
    Sequence::from_bytes(data)
}

// ── CSV / TSV ───────────────────────────────────────────────────────────────

/// `read_csv`: first row seeds one sequence per column; subsequent rows append
/// their columns. `multiline=false` keeps only the first column. TSV uses a
/// space delimiter (matching the C++ `read_csv(path, seqs, multiline, ' ')`).
fn read_csv(bytes: &[u8], multiline: bool, delim: char) -> Vec<Sequence> {
    let text = String::from_utf8_lossy(bytes);
    let mut lines = text.split('\n');
    let first = loop {
        match lines.next() {
            Some(l) if !l.is_empty() => break l,
            Some(_) => continue,
            None => return Vec::new(),
        }
    };
    let header: Vec<&str> = split_keep_empty(first, delim);
    let num_cols = if multiline {
        header.len()
    } else {
        header.len().min(1)
    };
    let mut cols: Vec<Vec<u8>> = header
        .iter()
        .take(num_cols)
        .map(|c| c.as_bytes().to_vec())
        .collect();
    for line in lines {
        if line.is_empty() {
            continue;
        }
        let row = split_keep_empty(line, delim);
        for (i, cell) in row.iter().enumerate().take(num_cols) {
            cols[i].extend_from_slice(cell.as_bytes());
        }
    }
    cols.into_iter().map(Sequence::from_bytes).collect()
}

fn split_keep_empty(s: &str, delim: char) -> Vec<&str> {
    // The C++ `split(..., false)` keeps empty tokens and trims trailing '\r'.
    let s = s.strip_suffix('\r').unwrap_or(s);
    s.split(delim).collect()
}

// ── FASTA / DNA / RNA ───────────────────────────────────────────────────────

/// `read_dna`: parse FASTA records (optionally gzip-compressed). Header lines
/// (`>`), name and comment are dropped; sequence lines are concatenated
/// byte-for-byte. `multiline=false` keeps only the first record.
fn read_dna(path: &Path, multiline: bool) -> io::Result<Vec<Sequence>> {
    let file = fs::File::open(path)?;
    let is_gz = path.extension().map(|e| e == "gz").unwrap_or(false);
    let reader: Box<dyn Read> = if is_gz {
        Box::new(MultiGzDecoder::new(file))
    } else {
        Box::new(file)
    };
    let reader = io::BufReader::new(reader);

    let mut records: Vec<Vec<u8>> = Vec::new();
    let mut cur: Option<Vec<u8>> = None;
    for line in reader.lines() {
        let line = line?;
        let t = trim(&line);
        if t.starts_with('>') || t.starts_with(';') {
            if let Some(seq) = cur.take() {
                records.push(seq);
                if !multiline {
                    break;
                }
            }
            cur = Some(Vec::new());
        } else if let Some(seq) = cur.as_mut() {
            seq.extend_from_slice(t.as_bytes());
        } else if !t.is_empty() {
            // sequence data before any header — start an implicit record
            cur = Some(t.as_bytes().to_vec());
        }
    }
    if let Some(seq) = cur.take() {
        if multiline || records.is_empty() {
            records.push(seq);
        }
    }
    Ok(records.into_iter().map(Sequence::from_bytes).collect())
}

// ── PBM / PGM ───────────────────────────────────────────────────────────────

/// Parse the `want` header integers of a PNM file, starting right after the
/// 2-character magic (so dimensions on the magic line are NOT lost), skipping
/// `#` comment lines and whitespace. Returns the integers and the byte offset
/// where the raster begins (just past the newline ending the last header line —
/// matching the C++ `getline`-based header consumption).
fn parse_pnm_header(bytes: &[u8], want: usize) -> Option<(Vec<u64>, usize)> {
    let mut i = 2usize.min(bytes.len()); // skip "Pn"
    let mut ints = Vec::with_capacity(want);
    while ints.len() < want {
        while i < bytes.len() && bytes[i].is_ascii_whitespace() {
            i += 1;
        }
        if i >= bytes.len() {
            break;
        }
        if bytes[i] == b'#' {
            while i < bytes.len() && bytes[i] != b'\n' {
                i += 1;
            }
            continue;
        }
        let start = i;
        while i < bytes.len() && bytes[i].is_ascii_digit() {
            i += 1;
        }
        if i == start {
            break; // not a digit or comment → malformed header
        }
        let v: u64 = std::str::from_utf8(&bytes[start..i]).ok()?.parse().ok()?;
        ints.push(v);
    }
    if ints.len() < want {
        return None;
    }
    // Raster begins after the newline that ends the last header line.
    while i < bytes.len() && bytes[i] != b'\n' {
        i += 1;
    }
    if i < bytes.len() {
        i += 1;
    }
    Some((ints, i))
}

/// Split a flat pixel buffer into the shape the C++ readers produce: one
/// concatenated sequence when `multiline` is false, or one sequence per image
/// row (`width` pixels each) when true.
fn shape_pixels(data: Vec<u8>, width: usize, multiline: bool) -> Vec<Sequence> {
    if !multiline {
        return vec![Sequence::from_bytes(data)];
    }
    if width == 0 {
        return vec![Sequence::from_bytes(data)];
    }
    data.chunks(width)
        .map(|row| Sequence::from_bytes(row.to_vec()))
        .collect()
}

/// `ReadPBM`: `width*height` 0/1 pixels → symbols with byte value 0/1 (NOT ASCII
/// '0'/'1'). P4 (binary) reads the raster as one contiguous MSB-first bitstream
/// (the C++ `ReadBin(is, s, width*height)` — no row padding).
fn read_pbm(bytes: &[u8], bin: bool, multiline: bool) -> Vec<Sequence> {
    let Some((dims, raster)) = parse_pnm_header(bytes, 2) else {
        return vec![Sequence::new()];
    };
    let (w, h) = (dims[0] as usize, dims[1] as usize);
    let total = w.saturating_mul(h);
    let mut data = Vec::with_capacity(total);
    if bin {
        // Contiguous MSB-first bitstream; pad past EOF with 1s (C++ get()→0xFF).
        let body = &bytes[raster.min(bytes.len())..];
        'outer: for &byte in body {
            for bit in (0..8).rev() {
                if data.len() >= total {
                    break 'outer;
                }
                data.push((byte >> bit) & 1);
            }
        }
        while data.len() < total {
            data.push(1);
        }
    } else {
        // P1 text: push (c=='1') for every '0'/'1' character in the raster.
        let text = String::from_utf8_lossy(bytes);
        for ch in text[raster.min(text.len())..].chars() {
            if ch == '0' || ch == '1' {
                data.push((ch == '1') as u8);
                if data.len() >= total {
                    break;
                }
            }
        }
    }
    shape_pixels(data, w, multiline)
}

/// `ReadPGM` (P2 text / P5 binary): each pixel's grey value becomes a symbol
/// (byte value = raw sample, truncated to 8 bits; `maxvalue` is not applied).
fn read_pgm(bytes: &[u8], bin: bool, multiline: bool) -> Vec<Sequence> {
    let Some((hdr, raster)) = parse_pnm_header(bytes, 3) else {
        return vec![Sequence::new()];
    };
    let (w, h) = (hdr[0] as usize, hdr[1] as usize);
    let total = w.saturating_mul(h);
    let mut data = Vec::with_capacity(total);
    if bin {
        // Exactly `total` raw bytes; pad past EOF with 0xFF (C++ get()→-1).
        let body = &bytes[raster.min(bytes.len())..];
        for &b in body.iter().take(total) {
            data.push(b);
        }
        while data.len() < total {
            data.push(0xff);
        }
    } else {
        let text = String::from_utf8_lossy(bytes);
        for tok in text[raster.min(text.len())..].split_ascii_whitespace() {
            if let Ok(v) = tok.parse::<u32>() {
                data.push((v & 0xff) as u8);
                if data.len() >= total {
                    break;
                }
            }
        }
    }
    shape_pixels(data, w, multiline)
}

// ── Top-level dispatch ──────────────────────────────────────────────────────

/// Resolve `Auto` by peeking the file, then read into sequences. Mirrors
/// `read_input` + `ReadPNM`.
pub fn read_input(path: &Path, multiline: bool, format: Format) -> io::Result<Vec<Sequence>> {
    let effective = if format == Format::Auto {
        let mut head = [0u8; 8];
        let mut f = fs::File::open(path)?;
        let n = f.read(&mut head)?;
        file_type_q(&head[..n])
    } else {
        format
    };

    match effective {
        Format::Csv => Ok(read_csv(&fs::read(path)?, multiline, ',')),
        Format::Tsv => Ok(read_csv(&fs::read(path)?, multiline, ' ')),
        Format::Dna | Format::Rna | Format::Fasta => read_dna(path, multiline),
        Format::P1 => Ok(read_pbm(&fs::read(path)?, false, multiline)),
        Format::P4 => Ok(read_pbm(&fs::read(path)?, true, multiline)),
        Format::P2 => Ok(read_pgm(&fs::read(path)?, false, multiline)),
        Format::P5 => Ok(read_pgm(&fs::read(path)?, true, multiline)),
        // PPM (P3/P6) and PAM (P7): the C++ `ReadPNM` switch has no case for
        // these and throws, leaving one empty sequence.
        Format::P3 | Format::P6 | Format::P7 => Ok(vec![Sequence::new()]),
        // RAW binary is always one sequence (the whole file as a bitstream),
        // even under multi-line.
        Format::RawBin => Ok(vec![read_raw_bin(&fs::read(path)?)]),
        // RawTxt (and Auto that resolved to text) → raw text.
        _ => {
            let bytes = fs::read(path)?;
            if multiline {
                Ok(read_raw_txt_multi(&bytes))
            } else {
                Ok(vec![read_raw_txt_single(&bytes)])
            }
        }
    }
}

/// Multi-line file → one concatenated sequence (skipping `#`/`>` comment lines).
/// Mirrors `multiLineToOneLine(concatenate=true)`.
pub fn multiline_to_one(path: &Path) -> io::Result<Sequence> {
    let f = fs::File::open(path)?;
    let reader = io::BufReader::new(f);
    let mut data: Vec<u8> = Vec::new();
    for line in reader.lines() {
        let line = line?;
        if line.is_empty() || line.starts_with('#') || line.starts_with('>') {
            continue;
        }
        data.extend_from_slice(line.trim_end_matches('\n').as_bytes());
    }
    Ok(Sequence::from_bytes(data))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_format_maps() {
        assert_eq!(parse_format("csv"), Format::Csv);
        assert_eq!(parse_format("TCSV"), Format::Tsv);
        assert_eq!(parse_format("pbmtxt"), Format::P1);
        assert_eq!(parse_format("pbm"), Format::P4);
        assert_eq!(parse_format("fasta"), Format::Fasta);
        assert_eq!(parse_format("nonsense"), Format::Auto);
    }

    #[test]
    fn detect_raw_text_vs_binary_vs_pnm() {
        assert_eq!(file_type_q(b"0101010101\n"), Format::RawTxt);
        assert_eq!(file_type_q(b"P1\n2 2\n"), Format::P1);
        assert_eq!(file_type_q(b"P4\n"), Format::P4);
        assert_eq!(file_type_q(b"\x00\x01\x02rest"), Format::RawBin);
    }

    #[test]
    fn raw_binary_expands_bits_msb_first_dropping_last_byte() {
        // 2 bytes -> (2-1)*8 = 8 bits from the first byte, MSB first.
        let s = read_raw_bin(&[0x0f, 0x00]);
        assert_eq!(s.as_bytes(), &[0, 0, 0, 0, 1, 1, 1, 1]);
    }

    #[test]
    fn pbm_text_pushes_bit_values() {
        let s = read_pbm(b"P1\n2 2\n1 0 0 1\n", false, false);
        assert_eq!(s.len(), 1);
        assert_eq!(s[0].as_bytes(), &[1u8, 0, 0, 1]);
    }

    #[test]
    fn pbm_header_on_magic_line_is_not_lost() {
        // dimensions on the same physical line as the magic number
        let s = read_pbm(b"P1 2 2\n1 0 0 1\n", false, false);
        assert_eq!(s[0].as_bytes(), &[1u8, 0, 0, 1]);
    }

    #[test]
    fn pbm_multiline_splits_rows() {
        let s = read_pbm(b"P1\n2 2\n1 0 0 1\n", false, true);
        assert_eq!(s.len(), 2);
        assert_eq!(s[0].as_bytes(), &[1u8, 0]);
        assert_eq!(s[1].as_bytes(), &[0u8, 1]);
    }

    #[test]
    fn pbm_binary_is_contiguous_bitstream() {
        // 4x1 image, one raster byte 0b1010_0000 -> first 4 bits 1,0,1,0
        let s = read_pbm(b"P4\n4 1\n\xa0", true, false);
        assert_eq!(s[0].as_bytes(), &[1u8, 0, 1, 0]);
    }

    #[test]
    fn raw_multi_keeps_comment_lines() {
        let multi = read_raw_txt_multi(b"abc\n# note\nxyz\n");
        assert_eq!(multi.len(), 3);
        assert_eq!(multi[1].as_bytes(), b"# note");
    }

    #[test]
    fn one_byte_rawbin_reads_eight_bits() {
        let s = read_raw_bin(&[0b1100_0000]);
        assert_eq!(s.as_bytes(), &[1, 1, 0, 0, 0, 0, 0, 0]);
    }

    #[test]
    fn csv_columns_include_header_row() {
        let single = read_csv(b"A,B\n0,1\n1,0\n", false, ',');
        assert_eq!(single.len(), 1);
        assert_eq!(single[0].as_bytes(), b"A01");
        let multi = read_csv(b"A,B\n0,1\n1,0\n", true, ',');
        assert_eq!(multi.len(), 2);
        assert_eq!(multi[0].as_bytes(), b"A01");
        assert_eq!(multi[1].as_bytes(), b"B10");
    }

    #[test]
    fn raw_text_single_reads_first_nonblank_line() {
        assert_eq!(read_raw_txt_single(b"\n\n  abc \nxyz\n").as_bytes(), b"abc");
    }
}
