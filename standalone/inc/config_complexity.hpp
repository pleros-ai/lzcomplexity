#pragma once

#include <lz/general.h>
#include <lz/lz.h>
#include <lz/pnm.h>

#include <cxxopts.hpp>
#include <json.hpp>

#include "lz/utils.h"
#include "config_utils.hpp"

using lz::standalone::print_msg;

// ============================================================================
// Command-line option definitions - compile-time constant
// ============================================================================

namespace detail {

inline constexpr std::string_view Header = 
   "LempelZiv-76 complexity engine. Suited for "
   "complexity analysis of time series. Send bug reports to estevez@fisica.uh.cu or "
   "efrenaragon96@gmail.com.\n";

struct CmdOpt {
   std::string_view key;
   std::string_view option_value;
   std::string_view description;
};

// Compile-time constant array
inline constexpr std::array<CmdOpt, 17> OptList{{
   {"alphabet", "a,alphabet", "Alphabet cardinality. If auto it tries to guess the alphabet size."},
   {"distance", "d,dlz", "The LZ distance is calculated between consecutive lines. Only valid for multiline files (-m option)."},
   {"shuffle", "z,zseq-shuffle", "Random shuffle complexity with Z sequence generated with the both halves of original sequence."},
   {"excess_opt", "e,excess-options", "Random shuffle complexity options for calculation. v1: maximum value for block shuffling, f: summand factors, "
     "v2: starting line for summand factors, v3: ending line for summand factors. All values are optionals"},
   {"factors", "f,factors", "Saves the factorization."},
   {"format", "F,format", "Input file format. TXT for raw text format. CSV the input file is a csv array with ',' as delimiter. TCSV the "
     "input file is a csv array with ' ' as delimiter. PBM, PGM and PNM is for the family of the graphic formats. DNA, RNA and FASTA "
     "are for the files with the biological sequences that use .fasta or .fna extensions."},
   {"help", "h,help", "Show this message."},
   {"mixed", "i,mixed-entropy", "The mixed entropy density of consecutive lines. Only valid for multiline files (-m option)."},
   {"jobs", "j,jobs", "Number of threads."},
   {"log_base", "l,log-base", "The log base value. The default is the alphabet cardinality."},
   {"multi_line", "m,multi-line", "Treat each line in the input stream as a different sequence."},
   {"entropy", "n,entropy-density", "Computes only the entropy density."},
   {"output", "o,output", "Output filename. Default appends to the end of input file a .json extension"},
   {"partitions", "p,partitions", "Number of partitions used for the parallel suffix array algorithm."},
   {"verbose", "v,verbose", "Verbose output."},
   {"version", "V,version", "Output the version number."},
   {"warn", "w,warn-out", "Hide warning messages."}
}};

// Compile-time lookup for option by key
[[nodiscard]] constexpr const CmdOpt* findOpt(std::string_view key) noexcept {
   for (const auto& opt : OptList) {
      if (opt.key == key) return &opt;
   }
   return nullptr;
}

// Helper to get option value string
[[nodiscard]] inline std::string getOptValue(std::string_view key) {
   if (const auto* opt = findOpt(key)) {
      return std::string(opt->option_value);
   }
   return {};
}

// Helper to get option description string
[[nodiscard]] inline std::string getOptDesc(std::string_view key) {
   if (const auto* opt = findOpt(key)) {
      return std::string(opt->description);
   }
   return {};
}

// ============================================================================
// Format parsing - unified function
// ============================================================================

[[nodiscard]] inline MagickNumber parseFormat(std::string_view format_str) noexcept {
   namespace utl = lz::utils;
   
   std::string lower;
   lower.reserve(format_str.size());
   for (char c : format_str) {
      lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
   }
   
   switch (utl::hash(lower)) {
      case utl::hash("pbm"):
      case utl::hash("pbmbin"):  return MagickNumber::PNM_P4;
      case utl::hash("pbmtxt"):  return MagickNumber::PNM_P1;
      case utl::hash("pgm"):
      case utl::hash("pgmbin"):  return MagickNumber::PNM_P5;
      case utl::hash("pgmtxt"):  return MagickNumber::PNM_P2;
      case utl::hash("raw"):
      case utl::hash("bin"):
      case utl::hash("rawbin"):  return MagickNumber::PNM_RAWBIN;
      case utl::hash("text"):
      case utl::hash("txt"):
      case utl::hash("rawtxt"):  return MagickNumber::PNM_RAWTXT;
      case utl::hash("csv"):     return MagickNumber::CSV;
      case utl::hash("tcsv"):    return MagickNumber::TCSV;
      case utl::hash("dna"):     return MagickNumber::DNA;
      case utl::hash("rna"):     return MagickNumber::RNA;
      case utl::hash("fasta"):   return MagickNumber::FASTA;
      default:                   return MagickNumber::AUTO;
   }
}

// ============================================================================
// Format name lookup for display
// ============================================================================

[[nodiscard]] inline std::string_view formatName(MagickNumber format) noexcept {
   switch (format) {
      case MagickNumber::PNM_P4:
      case MagickNumber::PNM_P1:     return "PBM";
      case MagickNumber::PNM_P5:
      case MagickNumber::PNM_P2:     return "PGM";
      case MagickNumber::PNM_RAWTXT:
      case MagickNumber::PNM_RAWBIN: return "TXT";
      case MagickNumber::CSV:
      case MagickNumber::TCSV:       return "CSV";
      case MagickNumber::DNA:        return "DNA";
      case MagickNumber::RNA:        return "RNA";
      case MagickNumber::FASTA:      return "FASTA";
      default:                       return "AUTO";
   }
}

inline constexpr std::string_view WarnDataSize = "Ignoring parallel processing because data source is too short";

}  // namespace detail

struct lz_options {
   std::string input;                                    //? Input filepath.
   std::string output         = "lz_results.lz76.json";  //? Output filepath.
   std::string factors_output = "";                      //? Output filepath for factors.
   /* Extra args for lzapp functions */
   lz::utils::LZ_Args args;
   /* flags */
   lz::lz_int excess_init_line =
      lz::utils::LZ_Args::UNDEFINED_LINES;  //?> Initial line where get shuffle entropy deficit terms
                                            //?  (valid for excess of entropy by shuffling).
   lz::lz_int excess_end_line =
      lz::utils::LZ_Args::UNDEFINED_LINES;  //?> Final line where get shuffle entropy deficit
                                            //? terms (valid for excess of entropy by shuffling).
   lz::lz_uint  n_jobs             = 1;
   MagickNumber input_format       = MagickNumber::PNM_RAWTXT;
   bool         entropy_density    = false;
   bool         multiLine          = false;
   bool         find_distance      = true;
   bool         preprocess         = false;
   bool         extras             = false;
   bool         save_results       = false;  //! @deprecated --> remove in final version
   bool         verbose            = false;
   bool         warn_out           = false;
#ifdef PLEROS_INTERNAL
   bool         mixed_entropy      = false;
   bool         get_paired_shuffle = false;
#endif

   // Internal setting for Pleros
   std::string json_params;

   explicit lz_options(const cxxopts::parse_result& result) {
      const auto& unmatched = result.unmatched();
      input = !unmatched.empty() ? unmatched[0] : "";

      // Output path
      if (result.count("output")) {
         output = result["output"].as<std::string>();
         if (output.empty()) {
            output = std::filesystem::path(input).replace_extension(".lz76.json").string();
         }
      } else {
         output = std::filesystem::path(input).replace_extension(".lz76.json").string();
      }

      // Factors output
      if (result.count("factors")) {
         factors_output = result["factors"].as<std::string>();
      }

      // Boolean flags
      multiLine       = result["multi-line"].as<bool>();
      find_distance   = result["dlz"].as<bool>();
      n_jobs          = result["jobs"].as<lz::lz_uint>();
      verbose         = result["verbose"].as<bool>();
      warn_out        = result["warn-out"].as<bool>();
      entropy_density = result["entropy-density"].as<bool>();
#ifdef PLEROS_INTERNAL
      mixed_entropy      = result["mixed-entropy"].as<bool>();
      get_paired_shuffle = result["zseq-shuffle"].as<bool>();
#endif

      json_params = result["json"].as<std::string>();

      // Parse format using unified function
      input_format = detail::parseFormat(result["format"].as<std::string>());

      if (input_format == MagickNumber::AUTO) {
         auto input_extension = std::filesystem::path(input).extension();

         if (input_extension == ".fna" || input_extension == ".fasta" || input_extension == ".gz") {
            input_format = MagickNumber::FASTA;
         } else if (input_extension == ".csv") {
            input_format = MagickNumber::CSV;
         }
      }

      // Core algorithm args
      args.chunks   = result["partitions"].as<lz::lz_int>();
      args.alphabet = result.count("alphabet") ? std::stoi(result["alphabet"].as<std::string>()) : lz::NO_ALPHABET;

      if (result.count("log-base")) {
         const auto& lg = result["log-base"].as<std::string>();
         args.log_base = lg.empty() ? args.alphabet : std::stoi(lg);
      } else {
         args.log_base = args.alphabet;
      }

      // Parse excess options
      if (result.count("excess-options")) {
         parseExcessOptions(result["excess-options"].as<std::vector<std::string>>());
      }
   }

private:
   void parseExcessOptions(const std::vector<std::string>& excess_args) {
      args.get_shuffle_terms = true;
      
      if (excess_args.empty() || excess_args[0].empty()) return;
      
      const auto& first = excess_args[0];
      
      if (first != "a" && first != "f") {
         args.block_size = std::stoi(first);
      } else if (first == "a") {
         args.block_size = 0;  // Auto block size
      }

      // Parse factor lines
      size_t factor_idx = (first == "f") ? 0 : 1;
      
      if (excess_args.size() > factor_idx && excess_args[factor_idx] == "f") {
         size_t line_start_idx = factor_idx + 1;
         
         if (excess_args.size() <= line_start_idx) {
            excess_init_line = lz::utils::LZ_Args::ALL_LINES;
            excess_end_line  = lz::utils::LZ_Args::ALL_LINES;
         } else {
            excess_init_line = std::stoi(excess_args[line_start_idx]);
            if (excess_args.size() > line_start_idx + 1) {
               excess_end_line = std::stoi(excess_args[line_start_idx + 1]);
            }
         }
      }
   }

public:

   friend std::ostream& operator<<(std::ostream& out, const lz_options& opt) {
      out << "Summary of options:\n";
      out << "Input data file: " << opt.input << '\n';
      out << "Format: " << detail::formatName(opt.input_format) << '\n';
      out << "Output: " << opt.output << '\n';
      out << "Process multiline: " << (opt.multiLine ? "ON" : "OFF") << '\n';
      out << "Number of jobs: " << opt.n_jobs << '\n';
      out << "Partitions use in parallel suffix array: " << opt.args.chunks << '\n';
      
      if (!opt.factors_output.empty()) {
         out << "Save factors in: " << opt.factors_output << '\n';
      }
      
      out << "Find distance: " << (opt.find_distance ? "ON" : "OFF") << '\n';
      
      if (opt.args.block_size > 0) {
         out << "Max block size for shuffling: " << opt.args.block_size << '\n';
      } else if (opt.args.block_size == 0) {
         out << "Max block size for shuffling: auto\n";
      } else {
         out << "Shuffle entropy density: OFF\n";
      }

      constexpr auto ALL = lz::utils::LZ_Args::ALL_LINES;
      if (opt.excess_init_line >= 0 && opt.excess_end_line >= 0) {
         out << "Find shuffle entropy deficit from line: " << opt.excess_init_line << '\n';
         out << "Find shuffle entropy deficit to line: " << opt.excess_end_line << '\n';
      } else if (opt.excess_init_line >= 0) {
         out << "Find shuffle entropy deficit for line: " << opt.excess_init_line << '\n';
      } else if (opt.excess_init_line == ALL) {
         out << "Find shuffle entropy deficit for all lines\n";
      }

      return out;
   }
};

inline void process_json_params(lz_options& opt, const std::string& file) {
   namespace fs = std::filesystem;
   const fs::path json_path = file;

   if (!fs::exists(json_path) || !fs::is_regular_file(json_path)) {
      throw FileNameError();
   }

   if (json_path.has_extension() && json_path.extension() != ".json") {
      throw FileFormatError();
   }

   std::ifstream dat(json_path);
   if (!dat.is_open()) {
      throw Errors("Unable to open the json file.");
   }

   nlohmann::json json_opt;
   dat >> json_opt;

   // Apply JSON parameters
   if (json_opt.contains("jobs"))         opt.n_jobs = json_opt["jobs"];
   if (json_opt.contains("multi_line"))   opt.multiLine = json_opt["multi_line"];
   if (json_opt.contains("get_distance")) opt.find_distance = json_opt["get_distance"];
   if (json_opt.contains("output"))       opt.output = json_opt["output"];
   
   if (json_opt.contains("input_format")) {
      opt.input_format = detail::parseFormat(json_opt["input_format"].get<std::string>());
   }

#ifdef PLEROS_INTERNAL
   if (json_opt.contains("get_paired_shuffle"))
      opt.get_paired_shuffle = json_opt["get_paired_shuffle"];
#endif
   // Arguments for LZ76 algoeithms
   if (json_opt.contains("partitions"))
      opt.args.chunks = json_opt["partitions"];
   if (json_opt.contains("block_size"))
      opt.args.block_size = json_opt["block_size"];
   if (json_opt.contains("initial_line"))
      opt.excess_init_line = json_opt["initial_line"];
   if (json_opt.contains("final_line"))
      opt.excess_end_line = json_opt["final_line"];
   if (json_opt.contains("get_shuffle_terms"))
      opt.args.get_shuffle_terms = json_opt["get_shuffle_terms"];

   if (json_opt.contains("logs"))
      opt.verbose = json_opt["logs"];
   else
      opt.verbose = false;
}

inline lz_options process_args(cxxopts::parse_result& result) {
   lz_options options(result);

   if (!options.json_params.empty()) {
      process_json_params(options, options.json_params);
   }

   namespace fs = std::filesystem;

   if (options.input.empty()) {
      throw FileNameError("Input file is missing.");
   }

   if (!fs::is_regular_file(options.input) && !fs::is_character_file(options.input)) {
      throw FileNameError("File doesn't exist: " + options.input);
   }

   if (options.verbose) {
      std::cout << "Verbose active!!!\n";
      std::cout << lz::GREEN_COLOR << "1. Processing input arguments\n" << lz::END_COLOR;

      using msg = lz::utils::MSG_TYPE;
      std::ostringstream iss;
      iss << options;

      iss << std::endl;
      iss << "Summary of results:\n";
      iss << " - lz76 factorization\n";
      iss << " - Entropy density\n";
      iss << " - Random shuffle complexity\n";
      if (options.args.block_size >= 0)
         iss << " - Paired Shuffle Complexity\n";
      if (options.find_distance) {
         iss << " - Information distance between consecutive sequences\n";
         iss << " - Random shuffle distance between consecutive sequences\n";
      }

      std::cout << print_msg(msg::INFO, iss.str()) << std::endl;
   }

   return options;
}

// ============================================================================
// Generate command-line options
// ============================================================================

[[nodiscard]] inline cxxopts::options generateOptions() {
   using detail::getOptValue;
   using detail::getOptDesc;
   
   cxxopts::options options("lzcomplexity", std::string(detail::Header));

   options.custom_help("[OPTIONS] <file>")
          .set_width(120)
          .set_tab_expansion(true);
   options.allow_unrecognised_options();
   
   auto g = options.add_options("OPTIONS: ");
   
   // String options with values
   g(getOptValue("alphabet"), getOptDesc("alphabet"), cxxopts::value<std::string>(), "value");
   g(getOptValue("factors"), getOptDesc("factors"), cxxopts::value<std::string>(), "file_name");
   g(getOptValue("format"), getOptDesc("format"), 
     cxxopts::value<std::string>()->default_value("AUTO"), "value");
   g(getOptValue("log_base"), getOptDesc("log_base"), cxxopts::value<std::string>(), "value");
   g(getOptValue("output"), getOptDesc("output"), cxxopts::value<std::string>(), "file_name");
   
   // Numeric options
   g(getOptValue("jobs"), getOptDesc("jobs"), 
     cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())), "value");
   g(getOptValue("partitions"), getOptDesc("partitions"), 
     cxxopts::value<lz::lz_int>()->default_value("0"), "value");
   
   // Complex options
   g(getOptValue("excess_opt"), getOptDesc("excess_opt"),
     cxxopts::value<std::vector<std::string>>()->delimiter(':')->implicit_value("a"), "[v1]:[f]:[v2]:[v3]");
   
   // Boolean flags
   g(getOptValue("distance"), getOptDesc("distance"));
   g(getOptValue("multi_line"), getOptDesc("multi_line"));
   g(getOptValue("entropy"), getOptDesc("entropy"));
   g(getOptValue("verbose"), getOptDesc("verbose"), cxxopts::value<bool>()->default_value("false"));
   g(getOptValue("version"), getOptDesc("version"), cxxopts::value<bool>()->default_value("false"));
   g(getOptValue("warn"), getOptDesc("warn"), cxxopts::value<bool>()->default_value("false"));
   
   // Help
   g(getOptValue("help"), getOptDesc("help"));
   
   // Hidden JSON option for internal use
   g("J,json", "Input parameters as json format (ignore all other flags)",
     cxxopts::value<std::string>()->default_value("")->hide(), "value");

#ifdef PLEROS_INTERNAL
   g(getOptValue("mixed"), getOptDesc("mixed"), cxxopts::value<bool>()->default_value("false"));
   g(getOptValue("shuffle"), getOptDesc("shuffle"));
#endif

   return options;
}
