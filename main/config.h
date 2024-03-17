#include <lz/core.h>
#include <lz/general.h>
#include <lz/pnm.h>

#include <cxxopts.hpp>
#include <json.hpp>

// define print_msg function for pretty print
std::string print_msg(lz::utils::MSG_TYPE type, std::string msg);

struct lz_options {
   std::string input;                       //? Input filepath.
   std::string output = "lz_results.json";  //? Output filepath.
   std::string factors_output = "";         //? Output filepath for factors.
   /* Extra args for LZApp functions */
   lz::utils::LZ_Args args;
   /* flags */
   lz::lz_int excess_init_line =
       lz::utils::LZ_Args::UNDEFINED_LINES;  //?> Initial line where get shuffle entropy deficit terms
                                             //?  (valid for excess of entropy by shuffling).
   lz::lz_int excess_end_line =
       lz::utils::LZ_Args::UNDEFINED_LINES;  //?> Final line where get shuffle entropy deficit
                                             //? terms (valid for excess of entropy by shuffling).
   lz::lz_uint n_jobs = 1;
   lz::lz_int input_format = MagickNumber::PNM_RAWTXT;
   bool entropy_density = false;
   bool multiLine = false;
   bool find_distance = true;
   bool preprocess = false;
   bool is_csv = false;
   bool save_results = false;  //! @deprecated --> remove in final version
   bool verbose = false;

   lz_options(cxxopts::parse_result result) {
      input = result.unmatched()[0];
      output = result["output"].as<std::string>();
      output = output.empty() ? input + ".json" : output;
      factors_output = result["factors"].as<std::string>();

      // flags
      multiLine = result["multi-line"].as<bool>();
      find_distance = result["dlz"].as<bool>();
      n_jobs = result["jobs"].as<lz::lz_uint>();
      is_csv = result["csv"].as<bool>();
      verbose = result["verbose"].as<bool>();
      entropy_density = result["entropy-density"].as<bool>();

      auto opt_format = result["format"].as<std::string>();
      lz::utils::to_lowercase(opt_format);

      if (opt_format == "pbm")
         input_format = MagickNumber::PNM_P4;
      else if (opt_format == "pgm")
         input_format = MagickNumber::PNM_P5;
      else if (opt_format == "pbmtxt")
         input_format = MagickNumber::PNM_P1;
      else if (opt_format == "pbmbin")
         input_format = MagickNumber::PNM_P4;
      else if (opt_format == "pgmtxt")
         input_format = MagickNumber::PNM_P2;
      else if (opt_format == "pgmbin")
         input_format = MagickNumber::PNM_P5;
      else if (opt_format == "raw")
         input_format = MagickNumber::PNM_RAWBIN;
      else if (opt_format == "rawbin")
         input_format = MagickNumber::PNM_RAWBIN;
      else if (opt_format == "rawtxt")
         input_format = MagickNumber::PNM_RAWTXT;
      else if (opt_format == "csv")
         input_format = MagickNumber::CSV;
      else
         input_format = MagickNumber::AUTO;

      // args for SA and Core functions
      args.chunks = result["partitions"].as<lz::lz_int>();
      args.alphabet = std::stoi(result["alphabet"].as<std::string>());
      auto lg = result["log-base"].as<std::string>();
      args.log_base = lg.empty() ? args.alphabet : std::stoi(lg);

      auto excess_args = result["entropy-shuffle"].as<std::vector<std::string>>();
      if (!excess_args.empty() && !excess_args[0].empty()) {
         if (excess_args[0] != "a" && excess_args[0] != "f") {
            args.block_size = std::stoi(excess_args[0]);
         } else if (excess_args[0] == "a") {
            args.block_size = 0;  //? Find a default block size in shuffling
         }

         if (excess_args.size() > 1 && excess_args[1] == "f") {
            if (excess_args.size() > 2) {
               excess_init_line = std::stoi(excess_args[2]);
            } else {
               excess_init_line = lz::utils::LZ_Args::ALL_LINES;
               excess_end_line = lz::utils::LZ_Args::ALL_LINES;
            }
            if (excess_args.size() > 3) {
               excess_end_line = std::stoi(excess_args[3]);
            }
         } else if (excess_args[0] == "f") {
            if (excess_args.size() > 1) {
               excess_init_line = std::stoi(excess_args[1]);
            } else {
               excess_init_line = lz::utils::LZ_Args::ALL_LINES;
               excess_end_line = lz::utils::LZ_Args::ALL_LINES;
            }
            if (excess_args.size() > 2) {
               excess_end_line = std::stoi(excess_args[2]);
            }
         }
      }
   }

   friend std::ostream& operator<<(std::ostream& out, const lz_options& opt) {
      out << "Summary of options: " << std::endl;
      out << "Input data file: " << opt.input << std::endl;

      out << "Format: ";
      if (opt.input_format == MagickNumber::PNM_P4 || opt.input_format == MagickNumber::PNM_P1) {
         out << "PBM" << std::endl;
      } else if (opt.input_format == MagickNumber::PNM_P5 || opt.input_format == MagickNumber::PNM_P2) {
         out << "PGM" << std::endl;
      } else if (opt.input_format == MagickNumber::PNM_RAWTXT || opt.input_format == MagickNumber::PNM_RAWBIN) {
         out << "TXT" << std::endl;
      } else if (opt.input_format == MagickNumber::CSV) {
         out << "CSV" << std::endl;
      } else {
         out << "AUTO" << std::endl;
      }

      out << "Output: " << opt.output << std::endl;
      out << "Process multiline: " << (opt.multiLine ? "ON" : "OFF") << std::endl;
      out << "Number of jobs: " << opt.n_jobs << std::endl;
      out << "Partitions use in parallel suffix array: " << opt.args.chunks << std::endl;
      if (!opt.factors_output.empty()) {
         out << "Save factors in: " << opt.factors_output << std::endl;
      }
      out << "Find distance: " << (opt.find_distance ? "ON" : "OFF") << std::endl;
      if (opt.args.block_size > 0) {
         out << "Max block size for shuffling: " << opt.args.block_size << std::endl;
      } else if (opt.args.block_size == 0) {
         out << "Max block size for shuffling: "
             << "auto" << std::endl;
      } else {
         out << "Shuffle entropy density: OFF" << std::endl;
      }

      if (opt.excess_init_line >= 0 && opt.excess_end_line >= 0) {
         out << "Find shuffle entropy deficit from line: " << opt.excess_init_line << std::endl;
         out << "Find shuffle entropy deficit to line: " << opt.excess_end_line << std::endl;
      } else if (opt.excess_init_line >= 0) {
         out << "Find shuffle entropy deficit for line: " << opt.excess_init_line << std::endl;
      } else if (opt.excess_init_line == lz::utils::LZ_Args::ALL_LINES) {
         out << "Find shuffle entropy deficit for all lines" << std::endl;
      }

      return out;
   }
};

lz_options process_args(cxxopts::parse_result& result) {
   lz_options options(result);

   namespace fs = std::filesystem;
   namespace utl = lz::utils;
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
      iss << " - LZ effective complexity\n";
      iss << " - Excess of entropy by distance\n";  // possible remove (same as LZ effective complexity)
      if (options.args.block_size >= 0) iss << " - Shuffle entropy deficit\n";
      iss << " - Information distance inside the sequence\n";
      if (options.find_distance) iss << " - Information distance between consecutive sequences\n";

      std::cout << print_msg(msg::INFO, iss.str()) << std::endl;
   }

   return options;
}
