
#include <string>
#include <unordered_map>

const std::string header = "LempelZiv-76 complexity engine. Suited for "
                           "complexity analysis of time series. Send bug reports to estevez@fisica.uh.cu or "
                           "efrenaragon96@gmail.com.\n";

typedef struct CMD_OPT {
   std::string option_value;
   std::string description;
} CMD_OPT;

inline std::unordered_map<std::string, CMD_OPT> opt_list{
   {"alphabet", {"a,alphabet", "Alphabet cardinality. If auto it tries to guess the alphabet size."}},
   {"distance",
    {"d,dlz", "The LZ distance is calculated between consecutive lines. Only valid for multiline files (-m option)."}},
   {"shuffle",
    {"z,zseq-shuffle",
     "Random shuffle complexity with Z sequence generated with the both halves of original sequence."}},
   {"excess_opt",
    {"e,excess-options",
     "Random shuffle complexity options for calculation. v1: maximum value for block shuffling, f: summands output, "
     "v2: "
     "starting line for summands output, v3: ending line for summands output. All values are optionals"}},
   {"factors", {"f,factors", "Saves the factorization."}},
   {"format",
    {"F,format",
     "Input file format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and PNM is for "
     "the family of the graphic formats."}},
   {"help", {"h,help", "Show this message."}},
   {"mixed",
    {"i,mixed-entropy",
     "The mixed entropy density of consecutive lines. Only valid for multiline files (-m "
     "option)."}},
   {"jobs", {"j,jobs", "Number of threads."}},
   {"log_base", {"l,log-base", "The log base value. The default is the alphabet cardinality."}},
   {"multi_line", {"m,multi-line", "Treat each line in the input stream as a different sequence."}},
   {"entropy", {"n,entropy-density", "Computes only the entropy density."}},
   {"output", {"o,output", "Output filename. Default appends to the end of input file a .json extension"}},
   {"partitions", {"p,partitions", "Number of partitions used for the parallel suffix array algorithm."}},
   {"verbose", {"v,verbose", "Verbose output."}},
   {"version", {"V,version", "Output the version number."}},
   {"warn", {"w,warn-out", "Hide warning messages."}}};

inline const std::string warn_data_size     = "Ingoring parallel processing because data source is too sort";
inline const std::string warn_data_size_log = "[ WARN ] " + warn_data_size;
