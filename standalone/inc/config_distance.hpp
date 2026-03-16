#pragma once

#include <lz/general.h>
#include <lz/lz.h>
#include <lz/pnm.h>

#include <cxxopts.hpp>
#include <json.hpp>
#include <limits>

#include "config_utils.hpp"

using lz::standalone::print_msg;

// ============================================================================
// Command-line option definitions - compile-time constant
// ============================================================================

namespace detail {

  struct CmdOpt {
    std::string_view key;
    std::string_view option_value;
    std::string_view description;
  };

  // Compile-time constant array instead of runtime unordered_map
  inline constexpr std::array<CmdOpt, 19> OptList{{
    {"adn", "a,adn", "Calculates the distance between two sets of ADN sequences."},
    {"binary", "b,binary", "Calculate the distance between two sets of sequences in binary format."},
    {"default", "d,default", "Calculates the distance between two sets of sequences."},
    {"factors", "f,factors", "Saves the factorization."},
    {"direction", "g,get-direction", "Calculates the lz76 direct graph between sequences."},
    {"first_format",
     "I,first-format",
     "First data source format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and PNM "
     "is for "
     "the family of the graphic formats."},
    {"second_format",
     "S,second-format",
     "Second data source format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and "
     "PNM is for "
     "the family of the graphic formats."},
    {"help", "h,help", "Show this message."},
    {"first", "i,first", "Range of lines/files will be process from the first data source."},
    {"jobs", "j,jobs", "Number of threads."},
    {"log_base", "l,log-base", "The log base value. The default is the alphabet cardinality."},
    {"verbose", "L,logs", "Verbose output."},
    {"reverse",
     "r,reverse",
     "Calculate the distance between the first set of sequences and the reverse of the second set of "
     "sequences."},
    {"output", "o,output", "Output filename. Default appends to the end of input file a .json extension"},
    {"partitions", "p,partitions", "Number of partitions used for the parallel suffix array algorithm."},
    {"second", "s,second", "Range of lines/files will be process from the second data source."},
    {"text", "t,text", "Calculates the distance between two text."},
    {"trajectory", "y,trajectory", "Calculates the distance between two sets of trajectories."},
    {"version", "v,version", "Output the version number."},
  }};

  // Compile-time lookup for option by key
  inline const CmdOpt* findOpt(std::string_view key) noexcept {
    for (const auto& opt: OptList) {
      if (opt.key == key) return &opt;
    }
    return nullptr;
  }

  // Helper to get option value string
  inline const std::string getOptValue(std::string_view key) {
    if (const auto* opt = findOpt(key)) {
      return std::string(opt->option_value);
    }
    return {};
  }

  // Helper to get option description string
  inline const std::string getOptDesc(std::string_view key) {
    if (const auto* opt = findOpt(key)) {
      return std::string(opt->description);
    }
    return {};
  }

  // ============================================================================
  // Format parsing - unified function to avoid code duplication
  // ============================================================================

  inline MagickNumber parseFormat(std::string_view format_str) noexcept {
    namespace utl = lz::utils;

    // Create lowercase copy for comparison
    std::string lower;
    lower.reserve(format_str.size());
    for (char c: format_str) {
      lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    switch (utl::hash(lower)) {
      case utl::hash("pbm"):
      case utl::hash("pbmbin"): return MagickNumber::PNM_P4;
      case utl::hash("pbmtxt"): return MagickNumber::PNM_P1;
      case utl::hash("pgm"):
      case utl::hash("pgmbin"): return MagickNumber::PNM_P5;
      case utl::hash("pgmtxt"): return MagickNumber::PNM_P2;
      case utl::hash("raw"):
      case utl::hash("bin"):
      case utl::hash("rawbin"): return MagickNumber::PNM_RAWBIN;
      case utl::hash("text"):
      case utl::hash("txt"):
      case utl::hash("rawtxt"): return MagickNumber::PNM_RAWTXT;
      case utl::hash("csv"): return MagickNumber::CSV;
      case utl::hash("tcsv"): return MagickNumber::TSV;
      case utl::hash("dna"): return MagickNumber::DNA;
      case utl::hash("rna"): return MagickNumber::RNA;
      case utl::hash("fasta"): return MagickNumber::FASTA;
      default: return MagickNumber::AUTO;
    }
  }

  // ============================================================================
  // Line range parsing - unified function
  // ============================================================================

  inline lz::details::LineRange parseLineRange(const std::vector<std::string>& filters) noexcept {
    lz::details::LineRange range;

    if (filters.empty()) return range;

    if (!filters[0].empty()) {
      try {
        range.init = std::stoi(filters[0]);
      } catch (...) {
        // Keep default on parse error
      }
    }

    if (filters.size() > 1) {
      if (filters[1].empty()) {
        range.end = lz::details::ALL_LINES;
      } else {
        try {
          range.end = std::stoi(filters[1]);
        } catch (...) {
          range.end = lz::details::ALL_LINES;
        }
      }
    }

    return range;
  }

  // ============================================================================
  // Format name lookup for display
  // ============================================================================

  inline std::string_view formatName(MagickNumber format) noexcept {
    switch (format) {
      case MagickNumber::PNM_P4:
      case MagickNumber::PNM_P1: return "PBM";
      case MagickNumber::PNM_P5:
      case MagickNumber::PNM_P2: return "PGM";
      case MagickNumber::PNM_RAWTXT:
      case MagickNumber::PNM_RAWBIN: return "TXT";
      case MagickNumber::CSV:
      case MagickNumber::TSV: return "CSV";
      case MagickNumber::DNA: return "DNA";
      case MagickNumber::RNA: return "RNA";
      case MagickNumber::FASTA: return "FASTA";
      default: return "AUTO";
    }
  }

}  // namespace detail

struct lz_options {
  // Data source paths
  std::string first_input;
  std::string second_input;
  std::string first_input_dir;
  std::string second_input_dir;
  std::string output = "lz_results.lzdist.json";
  std::string factors_output;

  // Core algorithm args
  lz::utils::LZ_Args args;

  // Line/file range for first source
  lz::details::LineRange first_range;
  lz::details::LineRange first_dir_range;

  // Line/file range for second source
  lz::details::LineRange second_range;
  lz::details::LineRange second_dir_range;

  // Runtime settings
  lz::lz_uint  n_jobs = 1;
  MagickNumber first_input_format = MagickNumber::PNM_RAWTXT;
  MagickNumber second_input_format = MagickNumber::PNM_RAWTXT;

  lz::lz_int matrix_threshold = std::numeric_limits<lz::lz_int>::max();

  // Flags - grouped for cache efficiency
  bool is_first_directory  : 1 = false;
  bool is_second_directory : 1 = false;
  bool verbose             : 1 = false;
  bool revert_             : 1 = false;
  bool text_               : 1 = false;
  bool binary_             : 1 = false;
  bool adn_                : 1 = false;
  bool trajectory_         : 1 = false;

  // Backward compatibility accessors
  lz::lz_int first_init_line() const noexcept { return first_range.init; }
  lz::lz_int first_end_line() const noexcept { return first_range.end; }
  lz::lz_int second_init_line() const noexcept { return second_range.init; }
  lz::lz_int second_end_line() const noexcept { return second_range.end; }
  lz::lz_int first_dir_init_line() const noexcept { return first_dir_range.init; }
  lz::lz_int first_dir_end_line() const noexcept { return first_dir_range.end; }
  lz::lz_int second_dir_init_line() const noexcept { return second_dir_range.init; }
  lz::lz_int second_dir_end_line() const noexcept { return second_dir_range.end; }

  explicit lz_options(const cxxopts::parse_result& result) {
    const auto& unmatched = result.unmatched();

    // Parse first source
    if (!unmatched.empty()) {
      const std::string& first = unmatched[0];
      is_first_directory = std::filesystem::is_directory(first);
      (is_first_directory ? first_input_dir : first_input) = first;
    }

    // Parse second source
    if (unmatched.size() > 1) {
      const auto& second = unmatched[1];
      is_second_directory = std::filesystem::is_directory(second);
      (is_second_directory ? second_input_dir : second_input) = second;
    }

    // Output path
    const auto& primary_source = is_first_directory ? first_input_dir : first_input;
    if (result.count("output")) {
      output = result["output"].as<std::string>();
      if (output.empty()) output = primary_source + ".lzdist.json";
    } else {
      output = primary_source + ".lzdist.json";
    }

    // Factors output
    if (result.count("factors")) {
      factors_output = result["factors"].as<std::string>();
    }

    // Runtime settings
    n_jobs = result["jobs"].as<lz::lz_uint>();
    verbose = result["logs"].as<bool>();

    // Mode flags
    revert_ = result["reverse"].as<bool>();
    text_ = result["text"].as<bool>();
    binary_ = result["binary"].as<bool>();
    adn_ = result["adn"].as<bool>();
    trajectory_ = result["trajectory"].as<bool>();

    if (result.count("get-direction")) {
      matrix_threshold = result["get-direction"].as<lz::lz_int>();
    }

    // Parse formats using unified function
    first_input_format = detail::parseFormat(result["first-format"].as<std::string>());
    second_input_format = detail::parseFormat(result["second-format"].as<std::string>());

    if (first_input_format == MagickNumber::AUTO) {
      auto input_extension = std::filesystem::path(first_input).extension();

      if (input_extension == ".fna" || input_extension == ".fasta" || input_extension == ".gz") {
        first_input_format = MagickNumber::FASTA;
      } else if (input_extension == ".csv") {
        first_input_format = MagickNumber::CSV;
      }
    }
    if (second_input_format == MagickNumber::AUTO) {
      auto input_extension = std::filesystem::path(second_input).extension();

      if (input_extension == ".fna" || input_extension == ".fasta" || input_extension == ".gz") {
        second_input_format = MagickNumber::FASTA;
      } else if (input_extension == ".csv") {
        second_input_format = MagickNumber::CSV;
      }
    }

    // Core algorithm args
    args.chunks = result["partitions"].as<lz::lz_int>();
    if (result.count("log-base")) {
      const auto& lg = result["log-base"].as<std::string>();
      args.log_base = lg.empty() ? args.alphabet : std::stoi(lg);
    } else {
      args.log_base = args.alphabet;
    }

    // Parse line ranges using unified function
    if (result.count("first")) {
      auto range = detail::parseLineRange(result["first"].as<std::vector<std::string>>());
      (is_first_directory ? first_dir_range : first_range) = range;
    }

    if (result.count("second")) {
      auto range = detail::parseLineRange(result["second"].as<std::vector<std::string>>());
      (is_second_directory ? second_dir_range : second_range) = range;
    }
  }

  friend std::ostream& operator<<(std::ostream& out, const lz_options& opt) {
    out << "Summary of options:\n";
    out << "First data source:" << (opt.is_first_directory ? " [directory] " : " [file] ")
        << (opt.is_first_directory ? opt.first_input_dir : opt.first_input) << '\n';

    if (!opt.second_input.empty() || !opt.second_input_dir.empty()) {
      out << "Second data source:" << (opt.is_second_directory ? " [directory] " : " [file] ")
          << (opt.is_second_directory ? opt.second_input_dir : opt.second_input) << '\n';
    }

    out << "Format: " << detail::formatName(opt.first_input_format) << '\n';
    out << "Output: " << opt.output << '\n';
    out << "Number of jobs: " << opt.n_jobs << '\n';
    out << "Partitions use in parallel suffix array: " << opt.args.chunks << '\n';

    if (!opt.factors_output.empty()) {
      out << "Save factors in: " << opt.factors_output << '\n';
    }

    // Helper lambda to print range info
    auto printRange = [&out](std::string_view prefix, const lz::details::LineRange& range) {
      constexpr auto ALL = lz::details::ALL_LINES;
      if (range.init >= 0 && range.end >= 0) {
        out << prefix << " from: " << range.init << '\n';
        out << prefix << " to: " << range.end << '\n';
      } else if (range.init >= 0 && range.end == ALL) {
        out << prefix << " from: " << range.init << '\n';
        out << prefix << " to final\n";
      } else if (range.init >= 0) {
        out << prefix << " at: " << range.init << '\n';
      }
    };

    out << "Data used for the distance calculation:\n";
    printRange("First file line", opt.first_range);
    printRange("First directory file", opt.first_dir_range);
    printRange("Second file line", opt.second_range);
    printRange("Second directory file", opt.second_dir_range);

    return out;
  }
};

inline auto process_args(cxxopts::parse_result& result) -> lz_options {
  namespace fs = std::filesystem;
  namespace utl = lz::utils;

  if (result.unmatched().empty() || result.unmatched().size() < 1) {
    throw BadCmdOptions("Input data source file or directory is missing");
  }
  lz_options options(result);

  if (options.first_input.empty() && options.first_input_dir.empty()) {
    throw FileNameError("The data source is missing.");
  }

  if (!options.is_first_directory && !fs::is_regular_file(options.first_input)
      && !fs::is_character_file(options.first_input)) {
    throw FileNameError("First data source doesn't exist: " + options.first_input);
  }
  if (!options.second_input.empty() && !options.is_second_directory
      && !fs::is_regular_file(options.second_input) && !fs::is_character_file(options.second_input)) {
    throw FileNameError("Second data source doesn't exist: " + options.second_input);
  }
  if (options.is_first_directory && !fs::is_directory(options.first_input_dir)) {
    throw FileNameError("First data source directory doesn't exist: " + options.first_input_dir);
  }
  if (!options.second_input_dir.empty() && options.is_second_directory
      && !fs::is_directory(options.second_input_dir)) {
    throw FileNameError("Second data source directory doesn't exist: " + options.second_input_dir);
  }

  if (options.verbose) {
    std::cout << "Verbose active!!!\n";
    std::cout << lz::GREEN_COLOR << "1. " << lz::END_COLOR << "Processing input arguments\n";

    using msg = utl::MSG_TYPE;
    std::ostringstream iss;
    iss << options;

    iss << std::endl;
    iss << "Summary of results:\n";
    if (!options.first_input_dir.empty() && !options.second_input_dir.empty()) {
      iss << " - Information distance between the files of two data source directories\n";
      iss << " - Random shuffle distance between the files of two data source directories\n";
    } else if ((!options.first_input_dir.empty() && !options.second_input.empty())
               || (!options.first_input.empty() && !options.second_input_dir.empty())) {
      iss << " - Information distance between files into a directory and a file\n";
      iss << " - Random shuffle distance between files into a directory and a file\n";
    } else if (!options.first_input.empty() && !options.second_input.empty()) {
      iss << " - Information distance between sequences in input data source files\n";
      iss << " - Random shuffle distance between sequences in input data source files\n";
    } else {
      iss << " - Information distance between sequences of the input data source file\n";
      iss << " - Random shuffle distance between sequences of the input data source file\n";
    }

    if (options.matrix_threshold != std::numeric_limits<lz::lz_int>::max()) {
      iss << " - Direct graph between sequences of the input data source file\n";
    }

    std::cout << print_msg(msg::INFO, iss.str()) << std::endl;
  }

  return options;
}

inline cxxopts::options generateOptions() {
  using detail::getOptDesc;
  using detail::getOptValue;

  cxxopts::options options(
    "LempelZiv-76 Information Distance engine. Suited for "
    "information distance analysis of time series. Send bug reports to estevez@fisica.uh.cu or "
    "efrenaragon96@gmail.com.\n");

  options.custom_help("[OPTIONS] <first source> [<second source>]").set_width(120).set_tab_expansion(true);
  options.allow_unrecognised_options();

  auto g = options.add_options("OPTIONS: ");

  // Boolean flags with defaults
  g(getOptValue("adn"), getOptDesc("adn"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("binary"), getOptDesc("binary"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("default"), getOptDesc("default"), cxxopts::value<bool>()->default_value("true"));
  g(getOptValue("reverse"), getOptDesc("reverse"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("direction"),
    getOptDesc("direction"),
    cxxopts::value<lz::lz_int>()->implicit_value("0"),
    "[threshold]");
  g(getOptValue("text"), getOptDesc("text"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("trajectory"), getOptDesc("trajectory"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("version"), getOptDesc("version"), cxxopts::value<bool>()->default_value("false"));
  g(getOptValue("verbose"), getOptDesc("verbose"), cxxopts::value<bool>()->default_value("false"));

  // String options
  g(getOptValue("factors"), getOptDesc("factors"), cxxopts::value<std::string>(), "file_name");
  g(getOptValue("output"), getOptDesc("output"), cxxopts::value<std::string>(), "file_name");
  g(getOptValue("log_base"), getOptDesc("log_base"), cxxopts::value<std::string>(), "value");

  // Format options
  g(getOptValue("first_format"),
    getOptDesc("first_format"),
    cxxopts::value<std::string>()->default_value("AUTO"),
    "value");
  g(getOptValue("second_format"),
    getOptDesc("second_format"),
    cxxopts::value<std::string>()->default_value("AUTO"),
    "value");

  // Range options
  g(getOptValue("first"),
    getOptDesc("first"),
    cxxopts::value<std::vector<std::string>>()->delimiter(':'),
    "#:#");
  g(getOptValue("second"),
    getOptDesc("second"),
    cxxopts::value<std::vector<std::string>>()->delimiter(':'),
    "#:#");

  // Numeric options
  g(getOptValue("jobs"),
    getOptDesc("jobs"),
    cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())),
    "value");
  g(getOptValue("partitions"),
    getOptDesc("partitions"),
    cxxopts::value<lz::lz_int>()->default_value("2"),
    "value");

  // Help
  g(getOptValue("help"), getOptDesc("help"));

  return options;
}
