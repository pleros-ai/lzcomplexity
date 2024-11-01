#include <lz/general.h>
#include <lz/lz.h>
#include <lz/pnm.h>

#include <cxxopts.hpp>
#include <json.hpp>
#include <unordered_map>

typedef struct CMD_OPT {
   std::string option_value;
   std::string description;
} CMD_OPT;

inline std::unordered_map<std::string, CMD_OPT> opt_list{
   {"adn", {"a,adn", "Calculates the distance between two sets of ADN sequences."}},
   {"binary", {"b,binary", "Calculate the distance between two sets of sequences in binary format."}},
   {"default", {"d,default", "Calculates the distance between two sets of sequences."}},
   {"factors", {"f,factors", "Saves the factorization."}},
   {"format",
    {"F,format",
     "Input file format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and PNM is for "
     "the family of the graphic formats."}},
   {"help", {"h,help", "Show this message."}},
   {"first", {"i,first", "Range of lines/files will be process from the first data source."}},
   {"jobs", {"j,jobs", "Number of threads."}},
   {"log_base", {"l,log-base", "The log base value. The default is the alphabet cardinality."}},
   {"verbose", {"L,logs", "Verbose output."}},
   {"multi_line", {"m,multi-line", "Treat each line in the input stream as a different sequence."}},
   {"reverse",
    {"r,reverse",
     "Calculate the distance between the first set of sequences and the reverse "
     "of the second set of sequences."}},
   {"output", {"o,output", "Output filename. Default appends to the end of input file a .json extension"}},
   {"partitions", {"p,partitions", "Number of partitions used for the parallel suffix array algorithm."}},
   {"second", {"s,second", "Range of lines/files will be process from the second data source."}},
   {"text", {"t,text", "Calculates the distance between two text."}},
   {"version", {"v,version", "Output the version number."}},
   {"trajectory", {"y,trajectory", "Calculates the distance between two sets of trajectories."}}};

// define print_msg function for pretty print
std::string print_msg(lz::utils::MSG_TYPE type, std::string msg);

struct lz_options {
   std::string first_input;                         //? Source input filepath.
   std::string second_input;                        //? Destination input filepath.
   std::string first_input_dir;                     //? Source input directory filepath.
   std::string second_input_dir;                    //? Destination input directory filepath.
   std::string output         = "lz_results.json";  //? Output filepath.
   std::string factors_output = "";                 //? Output filepath for factors.
   /* Extra args for LZApp functions */
   lz::utils::LZ_Args args;
   /* flags */
   lz::lz_int first_init_line = lz::utils::LZ_Args::ALL_LINES;   //?> Initial line where begin to calculate the lz
                                                                 //?> distance in the source input.
   lz::lz_int first_end_line = lz::utils::LZ_Args::ALL_LINES;    //?> Final line where ends to calculate the lz
                                                                 //?> distance in the source input.
   lz::lz_int second_init_line = lz::utils::LZ_Args::ALL_LINES;  //?> Initial line where begin to calculate
                                                                 //?> the lz distance in the destination input.
   lz::lz_int second_end_line = lz::utils::LZ_Args::ALL_LINES;   //?> Final line where ends to calculate the lz
                                                                 //?> distance in the destination input.

   lz::lz_int first_dir_init_line = lz::utils::LZ_Args::ALL_LINES;   //?> Initial file where begin to calculate
                                                                     //?> the lz distance in the source directory.
   lz::lz_int first_dir_end_line = lz::utils::LZ_Args::ALL_LINES;    //?> Final file where ends to calculate the lz
                                                                     //?> distance in the source directory.
   lz::lz_int second_dir_init_line = lz::utils::LZ_Args::ALL_LINES;  //?> Initial file where begin to calculate the
                                                                     //?> lz distance in the destination directory.
   lz::lz_int second_dir_end_line = lz::utils::LZ_Args::ALL_LINES;   //?> Final file where ends to calculate the lz
                                                                     //?> distance in the destination directory.

   lz::lz_uint  n_jobs              = 1;
   MagickNumber input_format        = MagickNumber::PNM_RAWTXT;
   bool         is_first_directory  = false;
   bool         is_second_directory = false;
   lz::lz_bool  multiLine           = false;
   lz::lz_bool  save_results        = false;  //! @deprecated --> remove in final version
   lz::lz_bool  verbose             = false;

   lz::lz_bool revert_     = false;
   lz::lz_bool text_       = false;
   lz::lz_bool binary_     = false;
   lz::lz_bool adn_        = false;
   lz::lz_bool trajectory_ = false;

   lz_options(cxxopts::parse_result result) {

      auto first  = !result.unmatched().empty() ? result.unmatched()[0] : "";
      auto second = !result.unmatched().empty() && result.unmatched().size() > 1 ? result.unmatched()[1] : "";

      if (std::filesystem::is_directory(first)) {
         first_input_dir    = first;
         is_first_directory = true;
      } else {
         first_input        = first;
         is_first_directory = false;
      }

      if (std::filesystem::is_directory(second)) {
         second_input_dir    = second;
         is_second_directory = true;
      } else {
         second_input        = second;
         is_second_directory = false;
      }

      if (result.count("output")) {
         output = result["output"].as<std::string>();
         output = output.empty() ? (is_first_directory ? first_input_dir : first_input) + ".json" : output;
      } else {
         output = (is_first_directory ? first_input_dir : first_input) + ".json";
      }

      if (result.count("factors")) {
         factors_output = result["factors"].as<std::string>();
      } else {
         factors_output = "";
      }

      // flags
      multiLine = result["multi-line"].as<bool>();
      n_jobs    = result["jobs"].as<lz::lz_uint>();
      verbose   = result["logs"].as<bool>();

      revert_     = result["reverse"].as<bool>();
      text_       = result["text"].as<bool>();
      binary_     = result["binary"].as<bool>();
      adn_        = result["adn"].as<bool>();
      trajectory_ = result["trajectory"].as<bool>();

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
         case utl::hash("rawbin"): input_format = MagickNumber::PNM_RAWBIN; break;
         case utl::hash("rawtxt"): input_format = MagickNumber::PNM_RAWTXT; break;
         case utl::hash("csv"): input_format = MagickNumber::CSV; break;
         default: input_format = MagickNumber::AUTO; break;
      }

      // args for SA and Core functions
      args.chunks = result["partitions"].as<lz::lz_int>();

      if (result.count("log-base")) {
         auto lg       = result["log-base"].as<std::string>();
         args.log_base = lg.empty() ? args.alphabet : std::stoi(lg);
      } else {
         args.log_base = args.alphabet;
      }

      lz::lz_int init_line = lz::utils::LZ_Args::UNDEFINED_LINES, end_line = lz::utils::LZ_Args::UNDEFINED_LINES;
      std::vector<std::string> filters;

      if (result.count("first")) {
         filters = result["first"].as<std::vector<std::string>>();
      }

      if (!filters.empty()) {
         if (filters.size() == 1) {
            if (!filters[0].empty()) {
               init_line = std::stoi(filters[0]);
            }
         } else {
            if (!filters[0].empty()) {
               init_line = std::stoi(filters[0]);
            }

            if (filters[1].empty()) {
               end_line = lz::utils::LZ_Args::ALL_LINES;
            } else {
               end_line = std::stoi(filters[1]);
            }
         }
      }

      if (is_first_directory) {
         first_dir_init_line = init_line;
         first_dir_end_line  = end_line;
      } else {
         first_init_line = init_line;
         first_end_line  = end_line;
      }

      init_line = lz::utils::LZ_Args::UNDEFINED_LINES;
      end_line  = lz::utils::LZ_Args::UNDEFINED_LINES;
      if (result.count("second")) {
         filters = result["second"].as<std::vector<std::string>>();
      } else {
         filters.clear();
      }

      std::cout << filters.size() << std::endl;

      if (!filters.empty()) {
         if (filters.size() == 1) {
            if (!filters[0].empty()) {
               init_line = std::stoi(filters[0]);
            }
         } else {
            if (!filters[0].empty()) {
               init_line = std::stoi(filters[0]);
            }

            if (filters[1].empty()) {
               end_line = lz::utils::LZ_Args::ALL_LINES;
            } else {
               end_line = std::stoi(filters[1]);
            }
         }
      }

      if (is_second_directory) {
         second_dir_init_line = init_line;
         second_dir_end_line  = end_line;
      } else {
         second_init_line = init_line;
         second_end_line  = end_line;
      }
   }

   friend std::ostream& operator<<(std::ostream& out, const lz_options& opt) {
      out << "Summary of options: " << std::endl;
      out << "First data source: " << (opt.is_first_directory ? " [directory] " : " [file] ")
          << (opt.is_first_directory ? opt.first_input_dir : opt.first_input) << std::endl;
      out << "Second data source: " << (opt.is_second_directory ? " [directory] " : " [file] ")
          << (opt.is_second_directory ? opt.second_input_dir : opt.second_input) << std::endl;

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

      out << "Data used for the distance calculation: ";
      if (opt.first_init_line >= 0 && opt.first_end_line >= 0) {
         out << "First file from line: " << opt.first_init_line << std::endl;
         out << "First file to line: " << opt.first_end_line << std::endl;
      } else if (opt.first_init_line >= 0 && opt.first_end_line == lz::utils::LZ_Args::ALL_LINES) {
         out << "First file from line: " << opt.first_init_line << std::endl;
         out << "First file to final line" << std::endl;
      } else if (opt.first_init_line >= 0) {
         out << "First file for line: " << opt.first_init_line << std::endl;
      }
      if (opt.first_dir_init_line >= 0 && opt.first_dir_end_line >= 0) {
         out << "First directory from file: " << opt.first_dir_init_line << std::endl;
         out << "First directory to file: " << opt.first_dir_end_line << std::endl;
      } else if (opt.first_dir_init_line >= 0 && opt.first_dir_end_line == lz::utils::LZ_Args::ALL_LINES) {
         out << "First directory from file: " << opt.first_dir_init_line << std::endl;
         out << "First directory to final file" << std::endl;
      } else if (opt.first_dir_init_line >= 0) {
         out << "First directory for file: " << opt.first_dir_init_line << std::endl;
      }
      if (opt.second_init_line >= 0 && opt.second_end_line >= 0) {
         out << "Second file from line: " << opt.second_init_line << std::endl;
         out << "Second file to line: " << opt.second_end_line << std::endl;
      } else if (opt.second_init_line >= 0 && opt.second_end_line == lz::utils::LZ_Args::ALL_LINES) {
         out << "Second file from line: " << opt.second_init_line << std::endl;
         out << "Second file to final line" << std::endl;
      } else if (opt.second_init_line >= 0) {
         out << "Second file for line: " << opt.second_init_line << std::endl;
      }
      if (opt.first_dir_init_line >= 0 && opt.second_dir_end_line >= 0) {
         out << "Second directory from file: " << opt.first_dir_init_line << std::endl;
         out << "Second directory to file: " << opt.second_dir_end_line << std::endl;
      } else if (opt.first_dir_init_line >= 0 && opt.second_dir_end_line == lz::utils::LZ_Args::ALL_LINES) {
         out << "Second directory from file: " << opt.first_dir_init_line << std::endl;
         out << "Second directory to final file" << std::endl;
      } else if (opt.first_dir_init_line >= 0) {
         out << "Second directory for file: " << opt.first_dir_init_line << std::endl;
      }

      return out;
   }
};

inline auto process_args(cxxopts::parse_result& result) -> lz_options {
   namespace fs  = std::filesystem;
   namespace utl = lz::utils;

   if (result.unmatched().empty() || result.unmatched().size() < 1) {
      throw BadCmdOptions("Input data source file or directory is missing");
   }
   lz_options options(result);

   if (options.first_input.empty() && options.first_input_dir.empty()) {
      throw FileNameError("The data source is missing.");
   }

   if (!options.is_first_directory && !fs::is_regular_file(options.first_input) &&
       !fs::is_character_file(options.first_input)) {
      throw FileNameError("First data source doesn't exist: " + options.first_input);
   }
   if (!options.second_input.empty() && !options.is_second_directory && !fs::is_regular_file(options.second_input) &&
       !fs::is_character_file(options.second_input)) {
      throw FileNameError("Second data source doesn't exist: " + options.second_input);
   }
   if (options.is_first_directory && !fs::is_directory(options.first_input_dir)) {
      throw FileNameError("First data source directory doesn't exist: " + options.first_input_dir);
   }
   if (!options.second_input_dir.empty() && options.is_second_directory &&
       !fs::is_directory(options.second_input_dir)) {
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
      } else if ((!options.first_input_dir.empty() && !options.second_input.empty()) ||
                 (!options.first_input.empty() && !options.second_input_dir.empty())) {
         iss << " - Information distance between files into a directory and a file\n";
         iss << " - Random shuffle distance between files into a directory and a file\n";
      } else if (!options.first_input.empty() && !options.second_input.empty()) {
         iss << " - Information distance between sequences in input data source files\n";
         iss << " - Random shuffle distance between sequences in input data source files\n";
      } else {
         iss << " - Information distance between sequences of the input data source file\n";
         iss << " - Random shuffle distance between sequences of the input data source file\n";
      }

      std::cout << print_msg(msg::INFO, iss.str()) << std::endl;
   }

   return options;
}

inline auto generateOptions() -> cxxopts::options {
   cxxopts::options options("LempelZiv-76 Information Distance engine. Suited for "
                            "information distance analysis of time series. Send bug reports to estevez@fisica.uh.cu or "
                            "efrenaragon96@gmail.com.\n");

   // clang-format off
   options.custom_help("[OPTIONS] <first source> [<second source>]")
          .set_width(120)
          .set_tab_expansion(true);
   options.allow_unrecognised_options();
   // clang-format on
   auto opt_group = options.add_options("OPTIONS: ");
   opt_group(opt_list["adn"].option_value, opt_list["adn"].description, cxxopts::value<bool>()->default_value("false"));
   opt_group(
      opt_list["binary"].option_value, opt_list["binary"].description, cxxopts::value<bool>()->default_value("false"));
   opt_group(
      opt_list["default"].option_value, opt_list["default"].description, cxxopts::value<bool>()->default_value("true"));
   opt_group(
      opt_list["factors"].option_value, opt_list["factors"].description, cxxopts::value<std::string>(), "file_name");
   opt_group(opt_list["format"].option_value,
             opt_list["format"].description,
             cxxopts::value<std::string>()->default_value("AUTO"),
             "value");
   opt_group(opt_list["help"].option_value, opt_list["help"].description);
   opt_group(opt_list["first"].option_value,
             opt_list["first"].description,
             cxxopts::value<std::vector<std::string>>()->delimiter(':'),
             "#:#");
   opt_group(opt_list["jobs"].option_value,
             opt_list["jobs"].description,
             cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())),
             "value");
   opt_group(
      opt_list["log_base"].option_value, opt_list["log_base"].description, cxxopts::value<std::string>(), "value");
   opt_group(opt_list["verbose"].option_value,
             opt_list["verbose"].description,
             cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["multi_line"].option_value, opt_list["multi_line"].description);
   opt_group(
      opt_list["output"].option_value, opt_list["output"].description, cxxopts::value<std::string>(), "file_name");
   opt_group(opt_list["partitions"].option_value,
             opt_list["partitions"].description,
             cxxopts::value<lz::lz_int>()->default_value("2"),
             "value");
   opt_group(opt_list["reverse"].option_value,
             opt_list["reverse"].description,
             cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["second"].option_value,
             opt_list["second"].description,
             cxxopts::value<std::vector<std::string>>()->delimiter(':'),
             "#:#");
   opt_group(
      opt_list["text"].option_value, opt_list["text"].description, cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["version"].option_value,
             opt_list["version"].description,
             cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["trajectory"].option_value,
             opt_list["trajectory"].description,
             cxxopts::value<bool>()->default_value("false"));

   return options;
}