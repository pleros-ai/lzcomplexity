//! `lzcomplexity` — standalone LZ76 complexity engine.
//!
//! A Rust port of the C++ `standalone/lzcomplexity.cpp`. Reads a sequence file
//! (auto-detecting the format), computes LZ76 complexity, entropy density and
//! random-shuffle effective measure complexity, and writes a JSON report whose
//! schema matches the C++ tool's `save_data`.

use std::process::ExitCode;

use clap::Parser;
use lzcomplexity_cli::formats::{self, Format};
use lzcomplexity_cli::{default_output, term};
use lzcomplexity_core as core;
use serde_json::{json, Map, Value};

const VERSION: &str = env!("CARGO_PKG_VERSION");

#[derive(Parser, Debug)]
#[command(
    name = "lzcomplexity",
    disable_version_flag = true,
    about = "LempelZiv-76 complexity engine. Suited for complexity analysis of time series.\n\
             Send bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com."
)]
struct Cli {
    /// Input sequence file.
    #[arg(value_name = "file")]
    file: Option<String>,

    /// Alphabet cardinality: `auto` guesses the alphabet size. (default auto)
    #[arg(short = 'a', long = "alphabet", value_name = "value")]
    alphabet: Option<String>,

    /// LZ distance between consecutive lines (implies multi-line).
    #[arg(short = 'd', long = "dlz", default_value_t = false)]
    dlz: bool,

    /// Random-shuffle options `v1:f:v2:v3` (block size, summand factors, line range).
    #[arg(short = 'e', long = "rsemc-opt", value_name = "opts", num_args = 0..=1, default_missing_value = "a")]
    rsemc_opt: Option<String>,

    /// Save the factorization to this file.
    #[arg(short = 'f', long = "factors", value_name = "file_name")]
    factors: Option<String>,

    /// Input file format (TXT, CSV, TSV, PBM, PGM, PNM, DNA, RNA, FASTA). Default: guess.
    #[arg(
        short = 'F',
        long = "format",
        value_name = "value",
        default_value = "AUTO"
    )]
    format: String,

    /// Number of threads. Default: hardware concurrency.
    #[arg(short = 'j', long = "jobs", value_name = "value")]
    jobs: Option<u32>,

    /// Logarithm base. Default: the alphabet cardinality.
    #[arg(short = 'l', long = "log-base", value_name = "value")]
    log_base: Option<String>,

    /// Treat each line of the input as a separate sequence.
    #[arg(short = 'm', long = "multi-line", default_value_t = false)]
    multi_line: bool,

    /// Compute only the entropy density.
    #[arg(short = 'n', long = "entropy-density", default_value_t = false)]
    entropy_density: bool,

    /// Output filename. Default: `<input>.lz76.json`.
    #[arg(short = 'o', long = "output", value_name = "file_name")]
    output: Option<String>,

    /// Number of partitions for the parallel suffix array.
    #[arg(
        short = 'p',
        long = "partitions",
        value_name = "value",
        default_value_t = 0
    )]
    partitions: i32,

    /// Verbose output.
    #[arg(short = 'v', long = "verbose", default_value_t = false)]
    verbose: bool,

    /// Print the version and exit.
    #[arg(short = 'V', long = "version", default_value_t = false)]
    version: bool,

    /// Hide warning messages.
    #[arg(short = 'w', long = "warn-out", default_value_t = false)]
    warn_out: bool,
}

struct Options {
    input: String,
    output: String,
    factors_output: Option<String>,
    format: Format,
    multi_line: bool,
    entropy_only: bool,
    find_distance: bool,
    verbose: bool,
    args: core::lz76::LzArgs,
}

fn parse_alphabet(s: &Option<String>) -> u32 {
    match s {
        None => core::NO_ALPHABET,
        Some(v) if v.eq_ignore_ascii_case("auto") || v.is_empty() => core::NO_ALPHABET,
        Some(v) => v.parse::<u32>().unwrap_or(core::NO_ALPHABET),
    }
}

/// Parse `-e v1:f:v2:v3` — v1 is the max block size (or `a` = auto = 0), `f`
/// requests the summand terms. Mirrors `parseExcessOptions`.
fn apply_rsemc(args: &mut core::lz76::LzArgs, opt: &str) {
    let parts: Vec<&str> = opt.split(':').collect();
    if parts.is_empty() || parts[0].is_empty() {
        return;
    }
    let first = parts[0];
    if first != "a" && first != "f" {
        if let Ok(v) = first.parse::<i32>() {
            args.block_size = v;
        }
    } else if first == "a" {
        args.block_size = 0;
    }
    let factor_idx = if first == "f" { 0 } else { 1 };
    if let Some(tok) = parts.get(factor_idx) {
        if *tok == "f" {
            args.get_shuffle_terms = true;
        }
    }
}

fn build_options(cli: &Cli) -> Options {
    let input = cli.file.clone().unwrap_or_default();

    // Format: explicit -F wins; otherwise resolve AUTO by extension.
    let mut format = formats::parse_format(&cli.format);
    if format == Format::Auto {
        let ext = std::path::Path::new(&input)
            .extension()
            .and_then(|e| e.to_str())
            .unwrap_or("")
            .to_ascii_lowercase();
        format = match ext.as_str() {
            "fna" | "fasta" | "gz" => Format::Fasta,
            "csv" => Format::Csv,
            "tsv" => Format::Tsv,
            _ => Format::Auto,
        };
    }

    let mut args = core::lz76::LzArgs::new();
    args.chunks = cli.partitions;
    args.alphabet = parse_alphabet(&cli.alphabet);
    args.log_base = match &cli.log_base {
        Some(v) if v.is_empty() => args.alphabet,
        Some(v) => v.parse::<u32>().unwrap_or(args.alphabet),
        None => args.alphabet,
    };
    args.block_size = -1;
    if let Some(opt) = &cli.rsemc_opt {
        apply_rsemc(&mut args, opt);
    }

    Options {
        output: cli
            .output
            .clone()
            .filter(|o| !o.is_empty())
            .unwrap_or_else(|| default_output(&input, "lz76")),
        input,
        factors_output: cli.factors.clone(),
        format,
        multi_line: cli.multi_line || cli.dlz,
        entropy_only: cli.entropy_density,
        find_distance: cli.dlz,
        verbose: cli.verbose,
        args,
    }
}

/// Standalone entropy density — the C++ batch `lz76EntropyDensity(flags, lz)`
/// computes `c * log_alphabet(n) / n`, where the log base cancels out (so the
/// `-l` flag does not affect it). `alphabet` is the explicit `-a` value or the
/// sequence's distinct-symbol count (min 2), matching the batch path.
fn standalone_entropy(seq: &core::Sequence, complexity: u32, args: &core::lz76::LzArgs) -> f64 {
    let n = seq.len() as f64;
    if n <= 1.0 {
        return 0.0;
    }
    let alphabet = if args.alphabet == core::NO_ALPHABET {
        seq.alphabet_size()
    } else {
        args.alphabet
    }
    .max(2) as f64;
    // c / (n / log_alphabet(n)) = c * ln(n) / (n * ln(alphabet))
    complexity as f64 * n.ln() / (n * alphabet.ln())
}

fn info(verbose: bool, msg: &str) {
    if verbose {
        println!("{}", term::print_msg(term::Msg::Info, msg));
    }
}

fn run(opt: &Options) -> std::io::Result<()> {
    let seqs = formats::read_input(std::path::Path::new(&opt.input), opt.multi_line, opt.format)?;
    let n_seq = seqs.len();
    info(opt.verbose, &format!("Sequences to process: {n_seq}"));

    // Which sequences appear in the report: all when multi-line, else just [0].
    let report_upto = if opt.multi_line { n_seq } else { 1.min(n_seq) };

    let mut seq_entries: Vec<Value> = Vec::with_capacity(report_upto);
    for seq in seqs.iter().take(report_upto) {
        let res = core::lz76::lz76_factors(seq, &opt.args);
        let entropy = standalone_entropy(seq, res.factorization, &opt.args);

        let mut entry = Map::new();
        entry.insert("size".into(), json!(seq.len()));
        // C++ stores the alphabet as `std::vector<char>` (signed on x86-64), so
        // nlohmann serialises bytes >= 128 as negative numbers — match that.
        entry.insert(
            "alphabet".into(),
            Value::Array(seq.alphabet().iter().map(|&b| json!(b as i8)).collect()),
        );
        entry.insert("alphabet_size".into(), json!(seq.alphabet_size()));
        entry.insert("lz76Complexity".into(), json!(res.factorization));
        entry.insert("lz76EntropyDensity".into(), json!(entropy));

        // The C++ `save_data` always writes the random-shuffle object. In
        // entropy-only (-n) mode the shuffle stage is skipped, so it emits the
        // default-constructed values (-1 / 0.0 / 0.0, summands omitted).
        let mut rsc_map = Map::new();
        if opt.entropy_only {
            rsc_map.insert("value".into(), json!(0.0));
            rsc_map.insert("max_block_size".into(), json!(-1));
            rsc_map.insert("multi_information".into(), json!(0.0));
        } else {
            let rsc = core::shuffle::lz76_random_shuffle_complexity(seq, &opt.args);
            rsc_map.insert("value".into(), json!(rsc.emc_value));
            rsc_map.insert("max_block_size".into(), json!(rsc.max_block_size));
            rsc_map.insert("multi_information".into(), json!(rsc.multi_information));
            if !rsc.summands.is_empty() {
                rsc_map.insert("summands".into(), json!(rsc.summands));
            }
        }
        entry.insert("lz76RandomShuffleComplexity".into(), Value::Object(rsc_map));
        seq_entries.push(Value::Object(entry));
    }

    let mut out = Map::new();
    out.insert("filename".into(), json!(opt.input));
    out.insert("format".into(), json!(opt.format.magic_name()));
    out.insert("size".into(), json!(n_seq));
    out.insert("sequences".into(), Value::Array(seq_entries));

    if opt.find_distance {
        // Consecutive-sequence information distance (fmax formula). For a single
        // sequence the C++ batch returns `[0]`; otherwise n-1 pairwise values.
        let dists: Vec<f64> = if n_seq <= 1 {
            vec![0.0]
        } else {
            (0..n_seq - 1)
                .map(|i| {
                    core::metrics::lz76_information_distance(&seqs[i], &seqs[i + 1], &opt.args)
                })
                .collect()
        };
        out.insert(
            "lz76Distance".into(),
            json!({ "InformationDistance": dists }),
        );
    }

    std::fs::write(&opt.output, serde_json::to_string(&Value::Object(out))?)?;

    // Separate factors file.
    if let Some(fpath) = &opt.factors_output {
        let mut factors: Vec<Value> = Vec::new();
        for seq in seqs.iter() {
            let res = core::lz76::lz76_factors(seq, &opt.args);
            factors.push(json!(res.lzf));
        }
        // The C++ `save_factors` writes `format` as the raw enum int (unlike
        // `save_data`, which uses the MagicValues string).
        let fjson = json!({
            "filename": opt.input,
            "format": opt.format.magic_ordinal(),
            "size": n_seq,
            "factors": factors,
        });
        std::fs::write(fpath, serde_json::to_string(&fjson)?)?;
    }

    info(opt.verbose, &format!("Saved results in: {}", opt.output));
    Ok(())
}

fn main() -> ExitCode {
    let cli = Cli::parse();

    if cli.version {
        println!(
            "{}",
            term::print_msg(term::Msg::Info, &format!("v{VERSION}"))
        );
        return ExitCode::SUCCESS;
    }

    if cli.file.is_none() {
        // No positional file: with no arguments at all, print help and exit
        // success (matching the C++ tool); otherwise it is a usage error.
        if std::env::args().len() <= 1 {
            let _ = <Cli as clap::CommandFactory>::command().print_help();
            return ExitCode::SUCCESS;
        }
        eprintln!(
            "{}",
            term::print_msg(term::Msg::Error, "Input file is missing")
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

    if !std::path::Path::new(&opt.input).exists() {
        eprintln!(
            "{}",
            term::print_msg(
                term::Msg::Error,
                &format!("File doesn't exist: {}", opt.input)
            )
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
