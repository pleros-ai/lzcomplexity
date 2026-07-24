//! `lzdistance` — standalone LZ76 information-distance engine.
//!
//! A Rust port of the C++ `standalone/lzdistance.cpp`. Computes pairwise
//! information-distance and shuffle-distance matrices between one or two data
//! sources (files or directories), with optional directed-graph output. The
//! JSON schema matches the C++ tool's `save_data`.

use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::Parser;
use lzcomplexity_cli::formats::{self, Format};
use lzcomplexity_cli::term;
use lzcomplexity_core as core;
use rayon::prelude::*;
use serde_json::{json, Map, Value};

const VERSION: &str = env!("CARGO_PKG_VERSION");

// Line-range sentinels (mirror lz::details).
const ALL_LINES: i64 = -2;
const UNDEFINED_LINES: i64 = -1;

#[derive(Parser, Debug)]
#[command(
    name = "lzdistance",
    disable_version_flag = true,
    about = "LempelZiv-76 Information Distance engine. Suited for information distance analysis of time series.\n\
             Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com."
)]
struct Cli {
    /// First data source (file or directory).
    #[arg(value_name = "first source")]
    first: Option<String>,
    /// Second data source (file or directory).
    #[arg(value_name = "second source")]
    second: Option<String>,

    /// Distance between two sets of DNA sequences (complement-aware).
    #[arg(short = 'a', long = "adn", default_value_t = false)]
    adn: bool,
    /// Distance between sequences in binary format (flip/reverse-aware).
    #[arg(short = 'b', long = "binary", default_value_t = false)]
    binary: bool,
    /// Distance between two sets of sequences (default; dead flag).
    #[arg(short = 'd', long = "default", default_value_t = true)]
    default_flag: bool,
    /// Save the factorization to this file.
    #[arg(short = 'f', long = "factors", value_name = "file_name")]
    factors: Option<String>,
    /// Compute the LZ76 directed graph. Optional threshold (implicit 0).
    #[arg(short = 'g', long = "get-direction", value_name = "threshold", num_args = 0..=1, default_missing_value = "0")]
    get_direction: Option<i64>,
    /// First data source format. Default: guess.
    #[arg(
        short = 'I',
        long = "first-format",
        value_name = "value",
        default_value = "AUTO"
    )]
    first_format: String,
    /// Second data source format. Default: guess.
    #[arg(
        short = 'S',
        long = "second-format",
        value_name = "value",
        default_value = "AUTO"
    )]
    second_format: String,
    /// Range of lines/files to process from the first source (`#:#`).
    #[arg(short = 'i', long = "first", value_name = "#:#", value_delimiter = ':')]
    first_range: Vec<String>,
    /// Number of threads.
    #[arg(short = 'j', long = "jobs", value_name = "value")]
    jobs: Option<u32>,
    /// Logarithm base. Default: the alphabet cardinality.
    #[arg(short = 'l', long = "log-base", value_name = "value")]
    log_base: Option<String>,
    /// Verbose output.
    #[arg(short = 'L', long = "logs", default_value_t = false)]
    logs: bool,
    /// Distance between the first set and the reverse of the second set.
    #[arg(short = 'r', long = "reverse", default_value_t = false)]
    reverse: bool,
    /// Output filename. Default: `<first source>.lzdist.json`.
    #[arg(short = 'o', long = "output", value_name = "file_name")]
    output: Option<String>,
    /// Number of partitions for the parallel suffix array.
    #[arg(
        short = 'p',
        long = "partitions",
        value_name = "value",
        default_value_t = 2
    )]
    partitions: i32,
    /// Range of lines/files to process from the second source (`#:#`).
    #[arg(
        short = 's',
        long = "second",
        value_name = "#:#",
        value_delimiter = ':'
    )]
    second_range: Vec<String>,
    /// Distance between two texts (dead flag).
    #[arg(short = 't', long = "text", default_value_t = false)]
    text: bool,
    /// Distance between two sets of trajectories (rotation-aware).
    #[arg(short = 'y', long = "trajectory", default_value_t = false)]
    trajectory: bool,
    /// Print the version and exit.
    #[arg(short = 'v', long = "version", default_value_t = false)]
    version: bool,
}

#[derive(Clone, Copy, Debug)]
struct LineRange {
    init: i64,
    end: i64,
}
impl Default for LineRange {
    fn default() -> Self {
        LineRange {
            init: UNDEFINED_LINES,
            end: UNDEFINED_LINES,
        }
    }
}

/// Mirror of `detail::parseLineRange` (tokens already split on `:`).
fn parse_line_range(tokens: &[String]) -> LineRange {
    let mut r = LineRange::default();
    if tokens.is_empty() {
        return r;
    }
    if !tokens[0].is_empty() {
        if let Ok(v) = tokens[0].parse::<i64>() {
            r.init = v;
        }
    }
    if tokens.len() > 1 {
        r.end = if tokens[1].is_empty() {
            ALL_LINES
        } else {
            tokens[1].parse::<i64>().unwrap_or(ALL_LINES)
        };
    }
    r
}

/// Mirror of `internal::canProcessTheLine` (1-based lines).
fn can_process(idx: usize, init: i64, end: i64) -> bool {
    if init == ALL_LINES || (init == UNDEFINED_LINES && end == UNDEFINED_LINES) {
        return true;
    }
    let line = idx as i64 + 1;
    // single-line selection
    if line == init && (end == UNDEFINED_LINES || init == end) {
        return true;
    }
    let after_start = init == ALL_LINES || init == UNDEFINED_LINES || init <= line;
    let before_end = end == ALL_LINES || end == UNDEFINED_LINES || end >= line;
    after_start && before_end
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Strategy {
    Default,
    Revert,
    Binary,
    Adn,
    Rna,
    Trajectory,
}

struct Options {
    first_input: String,
    second_input: String,
    first_input_dir: String,
    second_input_dir: String,
    is_first_directory: bool,
    is_second_directory: bool,
    output: String,
    first_format: Format,
    second_format: Format,
    first_range: LineRange,
    second_range: LineRange,
    matrix_threshold: i64,
    strategy: Strategy,
    args: core::lz76::LzArgs,
    verbose: bool,
}

fn resolve_format(spec: &str, path: &str) -> Format {
    let mut f = formats::parse_format(spec);
    if f == Format::Auto {
        let ext = Path::new(path)
            .extension()
            .and_then(|e| e.to_str())
            .unwrap_or("")
            .to_ascii_lowercase();
        f = match ext.as_str() {
            "fna" | "fasta" | "gz" => Format::Fasta,
            "csv" => Format::Csv,
            _ => Format::Auto,
        };
    }
    f
}

fn build_options(cli: &Cli) -> Options {
    let first = cli.first.clone().unwrap_or_default();
    let second = cli.second.clone().unwrap_or_default();
    let is_first_dir = Path::new(&first).is_dir();
    let is_second_dir = !second.is_empty() && Path::new(&second).is_dir();

    let (first_input, first_input_dir) = if is_first_dir {
        (String::new(), first.clone())
    } else {
        (first.clone(), String::new())
    };
    let (second_input, second_input_dir) = if is_second_dir {
        (String::new(), second.clone())
    } else {
        (second.clone(), String::new())
    };

    let primary = if is_first_dir {
        &first_input_dir
    } else {
        &first_input
    };
    let output = cli
        .output
        .clone()
        .filter(|o| !o.is_empty())
        .unwrap_or_else(|| format!("{primary}.lzdist.json"));

    let mut args = core::lz76::LzArgs::new();
    args.chunks = cli.partitions;
    args.alphabet = core::NO_ALPHABET;
    args.log_base = match &cli.log_base {
        Some(v) if v.is_empty() => args.alphabet,
        Some(v) => v.parse::<u32>().unwrap_or(args.alphabet),
        None => args.alphabet,
    };

    let strategy = if cli.binary {
        Strategy::Binary
    } else if cli.adn {
        Strategy::Adn
    } else if cli.trajectory {
        Strategy::Trajectory
    } else if cli.reverse {
        Strategy::Revert
    } else {
        Strategy::Default
    };

    let full_first_range = parse_line_range(&cli.first_range);
    let full_second_range = parse_line_range(&cli.second_range);

    Options {
        output,
        first_format: resolve_format(&cli.first_format, &first_input),
        second_format: resolve_format(&cli.second_format, &second_input),
        // The C++ keeps separate file/dir line-ranges and routes them by
        // `is_second_directory`; that path only differs for the (rare) case of
        // ranges combined with directory sources, so we apply the parsed range
        // directly. File+range (the common case) is unaffected.
        first_range: full_first_range,
        second_range: full_second_range,
        matrix_threshold: cli.get_direction.unwrap_or(i64::MAX),
        strategy,
        args,
        verbose: cli.logs,
        first_input,
        second_input,
        first_input_dir,
        second_input_dir,
        is_first_directory: is_first_dir,
        is_second_directory: is_second_dir,
    }
}

/// One sequence per non-`.json`/`.log` file in a directory (read with
/// multiline=false → first sequence of each file). Mirrors `read_dir`.
fn read_dir(dir: &str, format: Format) -> std::io::Result<Vec<core::Sequence>> {
    let mut files: Vec<PathBuf> = std::fs::read_dir(dir)?
        .filter_map(|e| e.ok())
        .map(|e| e.path())
        .filter(|p| {
            p.is_file()
                && !matches!(
                    p.extension().and_then(|x| x.to_str()),
                    Some("json") | Some("log")
                )
        })
        .collect();
    files.sort();
    let mut out = Vec::with_capacity(files.len());
    for f in files {
        let seqs = formats::read_input(&f, false, format)?;
        out.push(seqs.into_iter().next().unwrap_or_else(core::Sequence::new));
    }
    Ok(out)
}

fn load_source(
    is_dir: bool,
    dir: &str,
    file: &str,
    format: Format,
) -> std::io::Result<Vec<core::Sequence>> {
    if is_dir {
        read_dir(dir, format)
    } else if !file.is_empty() {
        // File sources are read with multiline=true → one sequence per line.
        formats::read_input(Path::new(file), true, format)
    } else {
        Ok(Vec::new())
    }
}

// ── Sequence transforms for the dispatch strategies ─────────────────────────

fn flip_bit(c: u8) -> u8 {
    if c == b'0' {
        b'1'
    } else {
        b'0'
    }
}

fn complement(ch: u8, is_rna: bool) -> u8 {
    match ch {
        b'A' => {
            if is_rna {
                b'U'
            } else {
                b'T'
            }
        }
        b'a' => {
            if is_rna {
                b'u'
            } else {
                b't'
            }
        }
        b'T' | b'U' => b'A',
        b't' | b'u' => b'a',
        b'C' => b'G',
        b'c' => b'g',
        b'G' => b'C',
        b'g' => b'c',
        other => other,
    }
}

/// Mirror of `swap_base(ch, target)`: swap `ch` with its complement only when
/// `toupper(ch)` is `target` or `target`'s complement; result is lowercased.
fn swap_base(ch: u8, target: u8, is_rna: bool) -> u8 {
    let upper = ch.to_ascii_uppercase();
    let comp_target = complement(target, is_rna);
    if upper == target || upper == comp_target {
        complement(upper, is_rna).to_ascii_lowercase()
    } else {
        ch
    }
}

/// The variants of the second operand tried by each strategy (the cell value is
/// the minimum distance over all variants).
fn second_variants(b: &core::Sequence, strategy: Strategy) -> Vec<core::Sequence> {
    match strategy {
        Strategy::Default => vec![b.clone()],
        Strategy::Revert => vec![b.reverse_copy()],
        Strategy::Binary => {
            let flipped = b.map(flip_bit);
            vec![
                b.clone(),
                b.reverse_copy(),
                flipped.clone(),
                flipped.reverse_copy(),
            ]
        }
        Strategy::Adn | Strategy::Rna => {
            let is_rna = strategy == Strategy::Rna;
            let at = b.map(|c| swap_base(c, b'A', is_rna));
            let cg = b.map(|c| swap_base(c, b'C', is_rna));
            vec![
                b.clone(),
                b.reverse_copy(),
                at.clone(),
                at.reverse_copy(),
                cg.clone(),
                cg.reverse_copy(),
            ]
        }
        Strategy::Trajectory => {
            let mut out = Vec::with_capacity(16);
            for tr in 0..8i64 {
                let rot = b.map(move |x| {
                    let val = x as i64 - b'0' as i64;
                    let idx = (val + tr - 1).rem_euclid(8);
                    b'1' + idx as u8
                });
                out.push(rot.reverse_copy());
                out.push(rot);
            }
            out
        }
    }
}

fn matrix_cell(
    a: &core::Sequence,
    b: &core::Sequence,
    strategy: Strategy,
    args: &core::lz76::LzArgs,
    dist: fn(&core::Sequence, &core::Sequence, &core::lz76::LzArgs) -> f64,
) -> f64 {
    second_variants(b, strategy)
        .iter()
        .map(|v| dist(a, v, args))
        .fold(f64::INFINITY, f64::min)
}

/// Compute a distance matrix (`first_len × second_len`) applying line-range
/// gating and the dispatch strategy. Cells outside the ranges stay 0.0.
fn distance_matrix(
    first: &[core::Sequence],
    second: &[core::Sequence],
    opt: &Options,
    dist: fn(&core::Sequence, &core::Sequence, &core::lz76::LzArgs) -> f64,
) -> Vec<Vec<f64>> {
    first
        .par_iter()
        .enumerate()
        .map(|(i, a)| {
            let mut row = vec![0.0f64; second.len()];
            if !can_process(i, opt.first_range.init, opt.first_range.end) {
                return row;
            }
            for (j, b) in second.iter().enumerate() {
                if !can_process(j, opt.second_range.init, opt.second_range.end) {
                    continue;
                }
                row[j] = matrix_cell(a, b, opt.strategy, &opt.args, dist);
            }
            row
        })
        .collect()
}

/// `lz76DirectedMatrix`: antisymmetric complexity-difference graph.
fn directed_matrix(
    first: &[core::Sequence],
    second: &[core::Sequence],
    is_symmetric: bool,
    threshold: i64,
    args: &core::lz76::LzArgs,
) -> Vec<Vec<i64>> {
    let first_len = first.len();
    let second_len = second.len();
    let dim = if is_symmetric {
        first_len
    } else {
        first_len + second_len
    };
    let mut m = vec![vec![0i64; dim]; dim];
    #[allow(clippy::needless_range_loop)]
    for i in 0..first_len {
        let j_start = if is_symmetric { i + 1 } else { 0 };
        for j in j_start..second_len {
            let a_lz = core::lz76::lz76_factorization(&(&first[i] + &second[j]), args) as i64;
            let b_lz = core::lz76::lz76_factorization(&(&second[j] + &first[i]), args) as i64;
            let diff = b_lz - a_lz;
            let j_idx = if is_symmetric { j } else { j + first_len };
            let over = diff.abs() > threshold;
            m[i][j_idx] = if over { -diff } else { 1 };
            m[j_idx][i] = if over { diff } else { 1 };
        }
    }
    m
}

fn dist_info(a: &core::Sequence, b: &core::Sequence, args: &core::lz76::LzArgs) -> f64 {
    core::metrics::lz76_information_distance(a, b, args)
}
fn dist_shuffle(a: &core::Sequence, b: &core::Sequence, args: &core::lz76::LzArgs) -> f64 {
    core::metrics::lz76_random_shuffle_distance(a, b, args)
}

fn run(opt: &Options) -> std::io::Result<()> {
    let first_data = load_source(
        opt.is_first_directory,
        &opt.first_input_dir,
        &opt.first_input,
        opt.first_format,
    )?;
    let second_data = load_source(
        opt.is_second_directory,
        &opt.second_input_dir,
        &opt.second_input,
        opt.second_format,
    )?;

    let has_second = !second_data.is_empty();
    // Effective second operand set: empty second → self-comparison.
    let effective_second: &[core::Sequence] = if has_second {
        &second_data
    } else {
        &first_data
    };

    if opt.verbose {
        println!(
            "{}",
            term::print_msg(
                term::Msg::Info,
                &format!(
                    "first_dim: {}  second_dim: {}",
                    first_data.len(),
                    effective_second.len()
                )
            )
        );
    }

    let info = distance_matrix(&first_data, effective_second, opt, dist_info);
    let shuffle = distance_matrix(&first_data, effective_second, opt, dist_shuffle);

    let mut out = Map::new();
    out.insert(
        "first_data_source".into(),
        json!(if opt.is_first_directory {
            &opt.first_input_dir
        } else {
            &opt.first_input
        }),
    );
    out.insert("first_dim".into(), json!(first_data.len()));
    out.insert(
        "first_data_source_format".into(),
        json!(opt.first_format.magic_name()),
    );

    if !opt.second_input.is_empty() || !opt.second_input_dir.is_empty() {
        out.insert(
            "second_data_source".into(),
            json!(if opt.is_second_directory {
                &opt.second_input_dir
            } else {
                &opt.second_input
            }),
        );
        out.insert("second_dim".into(), json!(second_data.len()));
        out.insert(
            "second_data_source_format".into(),
            json!(opt.second_format.magic_name()),
        );
    }

    out.insert("information_distance".into(), json!(info));
    out.insert("shuffle_information_distance".into(), json!(shuffle));

    // Directed matrix: computed only when a threshold was supplied
    // (`!= i64::MAX`); written whenever the threshold is truthy (`!= 0`).
    // This reproduces the C++ compute/save asymmetry exactly.
    let directed = if opt.matrix_threshold != i64::MAX {
        directed_matrix(
            &first_data,
            effective_second,
            !has_second,
            opt.matrix_threshold,
            &opt.args,
        )
    } else {
        Vec::new()
    };
    if opt.matrix_threshold != 0 {
        out.insert("directed_matrix".into(), json!(directed));
    }

    std::fs::write(&opt.output, serde_json::to_string(&Value::Object(out))?)?;
    if opt.verbose {
        println!(
            "{}",
            term::print_msg(
                term::Msg::Info,
                &format!("Saved results in: {}", opt.output)
            )
        );
    }
    Ok(())
}

fn main() -> ExitCode {
    let cli = Cli::parse();

    if cli.version {
        println!(
            "{}",
            term::print_msg(
                term::Msg::Info,
                &format!("Version of lzdistance: v{VERSION}")
            )
        );
        return ExitCode::SUCCESS;
    }

    if cli.first.is_none() {
        // No positional source: with no arguments at all, print help and exit
        // success (matching the C++ tool); otherwise it is a usage error.
        if std::env::args().len() <= 1 {
            let _ = <Cli as clap::CommandFactory>::command().print_help();
            return ExitCode::SUCCESS;
        }
        eprintln!(
            "{}",
            term::print_msg(term::Msg::Error, "Input data source is missing")
        );
        return ExitCode::FAILURE;
    }

    // Honour -j/--jobs by sizing the global rayon pool (mirrors EnabledMT).
    if let Some(j) = cli.jobs {
        if j > 0 {
            let _ = rayon::ThreadPoolBuilder::new()
                .num_threads(j as usize)
                .build_global();
        }
    }

    let opt = build_options(&cli);

    let first_ok = if opt.is_first_directory {
        Path::new(&opt.first_input_dir).is_dir()
    } else {
        Path::new(&opt.first_input).exists()
    };
    if !first_ok {
        eprintln!(
            "{}",
            term::print_msg(term::Msg::Error, "First data source doesn't exist")
        );
        return ExitCode::FAILURE;
    }

    match run(&opt) {
        Ok(()) => ExitCode::SUCCESS,
        Err(e) => {
            eprintln!("{}", term::print_msg(term::Msg::Error, &e.to_string()));
            ExitCode::FAILURE
        }
    }
}
