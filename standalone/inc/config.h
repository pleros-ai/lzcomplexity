#include <lz/general.h>
#include <lz/lz.h>
#include <lz/pnm.h>

#include <cxxopts.hpp>
#include <json.hpp>

// define print_msg function for pretty print
std::string print_msg(lz::utils::MSG_TYPE type, std::string msg);

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

   lz_options(cxxopts::parse_result result) {
      input = !result.unmatched().empty() ? result.unmatched()[0] : "";

      if (result.count("output")) {
         output = result["output"].as<std::string>();
         output = output.empty() ? std::filesystem::path(input).replace_extension(".lz76.json").string() : output;
      } else {
         output = std::filesystem::path(input).replace_extension(".lz76.json").string();
      }

      if (result.count("factors")) {
         factors_output = result["factors"].as<std::string>();
      } else {
         factors_output = "";
      }

      // flags
      multiLine          = result["multi-line"].as<bool>();
      find_distance      = result["dlz"].as<bool>();
      n_jobs             = result["jobs"].as<lz::lz_uint>();
      verbose            = result["verbose"].as<bool>();
      warn_out           = result["warn-out"].as<bool>();
      entropy_density    = result["entropy-density"].as<bool>();
#ifdef PLEROS_INTERNAL
      mixed_entropy      = result["mixed-entropy"].as<bool>();
      get_paired_shuffle = result["zseq-shuffle"].as<bool>();
#endif
      // preprocess      = result["process"].as<bool>();

      json_params = result["json"].as<std::string>();

      auto opt_format = result["format"].as<std::string>();
      lz::utils::to_lowercase(opt_format);

      namespace utl = lz::utils;
      switch (utl::hash(opt_format)) {
         case utl::hash("pbm"):
         case utl::hash("pbmbin"): input_format = MagickNumber::PNM_P4; break;
         case utl::hash("pbmtxt"): input_format = MagickNumber::PNM_P1; break;
         case utl::hash("pgm"):
         case utl::hash("pgmbin"): input_format = MagickNumber::PNM_P5; break;
         case utl::hash("pgmtxt"): input_format = MagickNumber::PNM_P2; break;
         case utl::hash("raw"):
         case utl::hash("bin"):
         case utl::hash("rawbin"): input_format = MagickNumber::PNM_RAWBIN; break;
         case utl::hash("text"):
         case utl::hash("txt"):
         case utl::hash("rawtxt"): input_format = MagickNumber::PNM_RAWTXT; break;
         case utl::hash("csv"): input_format = MagickNumber::CSV; break;
         case utl::hash("tcsv"): input_format = MagickNumber::TCSV; break;
         default: input_format = MagickNumber::AUTO; break;
      }

      // args for SA and Core functions
      args.chunks   = result["partitions"].as<lz::lz_int>();
      args.alphabet = result.count("alphabet") ? std::stoi(result["alphabet"].as<std::string>()) : lz::NO_ALPHABET;

      if (result.count("log-base")) {
         auto lg       = result["log-base"].as<std::string>();
         args.log_base = lg.empty() ? args.alphabet : std::stoi(lg);
      } else {
         args.log_base = args.alphabet;
      }

      std::vector<std::string> excess_args;
      if (result.count("excess-options")) {
         excess_args            = result["excess-options"].as<std::vector<std::string>>();
         args.get_shuffle_terms = true;
      }

      if (!excess_args.empty() && !excess_args[0].empty()) {
         if (excess_args[0] != "a" && excess_args[0] != "f") {
            args.block_size = std::stoi(excess_args[0]);
         } else if (excess_args[0] == "a") {
            args.block_size = 0;  //? Find a default block size in shuffling
         }

         if (excess_args.size() > 1 && excess_args[1] == "f") {
            if (excess_args.size() == 2) {
               excess_init_line = lz::utils::LZ_Args::ALL_LINES;
               excess_end_line  = lz::utils::LZ_Args::ALL_LINES;
            } else if (excess_args.size() > 2) {
               excess_init_line = std::stoi(excess_args[2]);
            }
            if (excess_args.size() > 3) {
               excess_end_line = std::stoi(excess_args[3]);
            }
         } else if (excess_args[0] == "f") {
            if (excess_args.size() > 1) {
               excess_init_line = std::stoi(excess_args[1]);
            } else {
               excess_init_line = lz::utils::LZ_Args::ALL_LINES;
               excess_end_line  = lz::utils::LZ_Args::ALL_LINES;
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
      } else if (opt.input_format == MagickNumber::CSV || opt.input_format == MagickNumber::TCSV) {
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

inline void process_json_params(lz_options& opt, std::string file) {

   namespace fs             = std::filesystem;
   const fs::path json_path = file;

   if (!fs::exists(json_path) || !fs::is_regular_file(json_path)) {
      throw FileNameError();
   }

   if (json_path.has_extension() && json_path.extension() != ".json") {
      throw FileFormatError();
   }

   nlohmann::json json_opt;
   std::ifstream  dat(json_path);

   if (!dat.is_open()) {
      throw Errors("Unable to open the json file.");
   }

   dat >> json_opt;

   if (json_opt.contains("jobs"))
      opt.n_jobs = json_opt["jobs"];

   if (json_opt.contains("multi_line"))
      opt.multiLine = json_opt["multi_line"];

   if (json_opt.contains("get_distance"))
      opt.find_distance = json_opt["get_distance"];

   if (json_opt.contains("output"))
      opt.output = json_opt["output"];

   if (json_opt.contains("input_format")) {
      namespace utl = lz::utils;
      switch (utl::hash(json_opt["input_format"].get<std::string>())) {
         case utl::hash("pbm"):
         case utl::hash("pbmbin"): opt.input_format = MagickNumber::PNM_P4; break;
         case utl::hash("pbmtxt"): opt.input_format = MagickNumber::PNM_P1; break;
         case utl::hash("pgm"):
         case utl::hash("pgmbin"): opt.input_format = MagickNumber::PNM_P5; break;
         case utl::hash("pgmtxt"): opt.input_format = MagickNumber::PNM_P2; break;
         case utl::hash("raw"):
         case utl::hash("rawbin"): opt.input_format = MagickNumber::PNM_RAWBIN; break;
         case utl::hash("rawtxt"): opt.input_format = MagickNumber::PNM_RAWTXT; break;
         case utl::hash("csv"): opt.input_format = MagickNumber::CSV; break;
         default: opt.input_format = MagickNumber::AUTO; break;
      }
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
