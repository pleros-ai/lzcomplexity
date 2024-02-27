#include <string>
#include <limits>
#include <vector>
#include <thread>
#if __cplusplus >= 201703L
#include <variant>
#endif

#include <tbb/task_arena.h>
#include <cxxopts.hpp>
#include <json.hpp>

#include <lz/core.h>
#include <lz/lzApp.h>

inline auto getColor(lz::utils::MSG_TYPE type) {
    switch (type) {
    case lz::utils::MSG_TYPE::ERROR:
        return lz::RED_COLOR;
    case lz::utils::MSG_TYPE::WARRING:
        return lz::YELLOW_COLOR;
    case lz::utils::MSG_TYPE::INFO:
        return lz::GREEN_COLOR;
    }
    return lz::BLUE_COLOR;
}

inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline std::string print_msg(lz::utils::MSG_TYPE type, std::string msg) {
    auto color = getColor(type);
    std::vector<std::string> allLines;
    std::va_list args;

    std::string topL = "╭──";
    std::string botL = "╰──";
    std::string topR = "──╮";
    std::string botR = "──╯";
    std::string::size_type maxLen = 0;

    std::string final_msg = "";
    std::string delimiter = "\n";
    size_t pos = 0;

    allLines = split(msg, '\n');
    for (auto tmp : allLines) {
        maxLen = tmp.length() > maxLen ? tmp.length() : maxLen;
    }

    auto header = topL + std::string(maxLen - 1, ' ') + topR + "\n";
    auto bottom = botL + std::string(maxLen - 1, ' ') + botR + "\n";

    for (auto str : allLines) {
        final_msg += "│ " + str + std::string(maxLen - str.length() + 1, ' ') + " │\n";
    }

    return color + header + final_msg + bottom + lz::END_COLOR;
}

// Read a plain text file with one line
inline std::vector<lz::sequence> read_input(const std::string& ip_path, bool multiline = false) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto file_size = fs::file_size(ip_path, ec);

    if (ec) {
        std::cerr << lz::RED_COLOR << ip_path << " : " << ec.message() << "\n" << lz::END_COLOR;
        std::exit(EXIT_FAILURE);
    }

    int num_line = 0;
    std::ifstream input(ip_path);
    lz::sequence line;
    lz::sequence final_str;
    std::vector<lz::sequence> data;

    while (input.good() && !input.eof()) {
        input >> line;
        num_line++;
        if (num_line % 35 == 0) std::cout << "\r Num of lines read: " << num_line;

        if (line.at(0) == '#' || line.at(0) == '\n' || line.length() == 0) continue;

        data.push_back(line);
        if(!multiline) break;
    }

    input.close();
    return data;
}

// Read a plain text file with multiple line
inline void read_multiInputs(const std::string& ip_path, std::vector<lz::sequence>& text_col, bool process = false) {
    namespace fs = std::filesystem;
    std::error_code ec;

    const auto file_size = fs::file_size(ip_path, ec);
    int num_line = 0;

    if (ec) {
        std::cerr << ip_path << " : " << ec.message() << "\n";
        std::exit(EXIT_FAILURE);
    }

    std::ifstream input(ip_path);
    lz::sequence line;
    lz::sequence final_str;

    while (input.good() && !input.eof()) {
        input >> line;
        num_line++;
        if (num_line % 35 == 0) std::cout << "\rprocess: " << process << " Num of lines read: " << num_line;

        if (line.at(0) == '#' || line.at(0) == '\n' || line.length() == 0) continue;

        if (process) {
            final_str += line;
        }
        else {
            text_col.push_back(line);
        }
    }

    if (process) text_col.push_back(final_str);
    input.close();
    std::cout << "End read\n";
}

// Read a csv file with multiple columns (date per column)
inline void read_csv(const std::string& ip_path, std::vector<std::string>& text_col) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto file_size = fs::file_size(ip_path, ec);
    int num_line = 0;

    if (ec) {
        std::cerr << lz::RED_COLOR << ip_path << " : " << ec.message() << "\n" << lz::END_COLOR;
        std::exit(EXIT_FAILURE);
    }

    io::LineReader input(ip_path);

    auto line = input.next_line();
    std::string str(line);
    auto rows = split(str, ',');
    text_col.reserve(rows.size());

    for (auto row : rows) {
        text_col.push_back(row);
    }

    double x, y, z;
    while (auto line = input.next_line()) {
        std::string str(line);
        auto rows = split(str, ',');
        for (size_t i = 0; i < rows.size(); i++) {
            text_col[i] += rows[i];
        }
    }
    print_msg(lz::utils::MSG_TYPE::INFO, text_col[0]);
}


struct lz_options {
    std::string input;      //? Input filepath.
    std::string output;     //? Output filepath.
    /* Extra args for LZApp functions */
    lz::utils::LZ_Args args;
    /* flags */
    bool multiLine;
    bool print_factors;
    bool find_distance;
    bool preprocess;
    bool is_csv;
    bool save_results;

    lz_options() {
        output = "lz_results.json";
        multiLine = false;
        print_factors = false;
        find_distance = false;
        preprocess = false;
        is_csv = false;
        save_results = false;
    }

    lz_options(cxxopts::ParseResult result) {
        input = result.unmatched()[0];
        output = result["output"].as<std::string>();
        multiLine = result["multi-line"].as<bool>();
        print_factors = result["factors"].as<bool>();
        find_distance = result["dlz"].as<bool>();
        preprocess = result["process"].as<bool>();
        is_csv = result["csv"].as<bool>();
        save_results = result["save"].as<bool>();

        args.chunks = result["chunks"].as<lz::lz_int>();
        args.max_context = result["max-context"].as<lz::lz_int>();
        args.block_size = result["max-excess"].as<lz::lz_int>();
        args.excess_line = result["excess"].as<lz::lz_int>();
    }
};

lz_options process_args(cxxopts::ParseResult& result) {
    lz_options options(result);

    namespace fs = std::filesystem;
    namespace utl = lz::utils;
    if (!fs::is_regular_file(options.input) &&
        !fs::is_character_file(options.input)) {
        throw FileNameError("File doesn't exist: " + options.input);
    }

    return options;
}

void save_data(lz::utils::LZ_Flags& flags, lz::utils::LZ_Output& results, lz_options& opt) {
    nlohmann::json out_data;

    for (std::size_t i = 0; i < flags.input.size(); i++) {
        // out_data["input"] = opt.input;
        out_data["sequences"][i]["sequence_size"] = flags.input[i].size();

        if (results.complexity.size()) {
            out_data["sequences"][i]["complexity"] = results.complexity[i];
        }

        if (results.entropy_density.size()) {
            out_data["sequences"][i]["entropy_density"] = results.entropy_density[i];
        }

        if (results.excess_entropy_mi.size()) {
            out_data["sequences"][i]["excess_entropy_mi"] = results.excess_entropy_mi[i];
        }

        if (results.excess_entropy_dist.size()) {
            out_data["sequences"][i]["excess_entropy_by_distance"] = results.excess_entropy_dist[i];
        }

        if (results.excess_entropy_shuffle.size()) {
            out_data["sequences"][i]["excess_entropy_by_shuffle"] = results.excess_entropy_shuffle[i];
        }

        if (opt.find_distance && i < flags.input.size() - 1) {
            out_data["sequences"][i]["info_distance"] = results.info_distance[i];
        }

        if (!opt.multiLine) break;
    }

    std::ofstream out(opt.output);
    if (out.is_open() && out.good()) {
        out << out_data;
        out.close();
    }
}

lz::lz_int process(lz_options& opt) {
    std::vector<std::string> data;
    std::vector<lz::sequence> data2;
    std::string text;
    lz::sequence text2;

    if (opt.is_csv) {
        read_csv(opt.input, data);
        exit(0);
    }
    else {
        data2 = read_input(opt.input, opt.multiLine);
    }
    // read_multiInputs(opt.input, data2, opt.preprocess);

    if (data.size()) std::cout << data[0].length() << " str: " << data[0].substr(0, 10) << std::endl;
    if (data2.size()) std::cout << " seq: " << text2.Take(10) << std::endl;

    // lz::utils::sa_type<>::type alg = lz::suffixarray::SAIS();
    // lz::utils::sa_type<>::type alg = lz::suffixarray::CaPS_SA(opt.args.chunks, opt.args.max_context);

    lz::utils::LZ_Flags test_flags(data2, opt.args);
    lz::utils::LZ_Output lz;
    lz::utils::LZ_Output lz2;

    std::cout << data2.size() << std::endl;


    // auto maxConcurrency = 11;
    // if (maxConcurrency > tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)) {
    //     lz::utils::print_msg(lz::utils::MSG_TYPE::WARRING, "tbb::global_control is active, limiting the number of parallel workers\n from this task arena available for execution.");
    // }

    // auto arena = tbb::task_arena();
    // arena.initialize(maxConcurrency);

    // tbb::task_arena no_ht_arena(std::thread::hardware_concurrency());
    
    // Core functions
    // auto c = lz::LempelZivFactorization(data2[0]);
    // auto d = lz::EntropyDensity(data2[0], lz::utils::LZ_Args(8));
    // auto f = lz::InformationDistance(data2[0], data2[0]);
    // auto es = lz::ExcessEntropyShuffle(data2[0], lz::utils::LZ_Args(8));
    // auto em = lz::ExcessEntropyMi(data2[0]);
    // auto ed = lz::ExcessEntropyDistance(data2[0]);
    // std::cout  << c << "\n";
    // std::cout  << d << "\n";
    // std::cout  << f << "\n";
    // std::cout  << em << "\n";
    // std::cout  << es << "\n";
    // std::cout  << ed << "\n";

    // App functions
    lz::LempelZivFactorization(test_flags, lz);
    std::cout << std::endl;
    std::cout << "Complexity: ";
    for (auto x : lz.complexity) std::cout << x << " ";
    std::cout << std::endl;

    lz::EntropyDensity(test_flags, lz);
    std::cout << "Entropy: ";
    for (auto x : lz.entropy_density) std::cout << x << " ";
    std::cout << std::endl;

    lz::ExcessEntropyMi(test_flags, lz);
    std::cout << "Excess entropy as MI: ";
    for (auto x : lz.excess_entropy_mi) std::cout << x << " ";
    std::cout << std::endl;

    lz::ExcessEntropyDistance(test_flags, lz);
    std::cout << "Excess entropy by distance: ";
    for (auto x : lz.excess_entropy_dist) std::cout << x << " ";
    std::cout << std::endl;

    lz::ExcessEntropyShuffle(test_flags, lz);
    std::cout << "Excess entropy by shuffle: ";
    for (auto x : lz.excess_entropy_shuffle) std::cout << x << " ";
    // lz::ExcessEntropyShuffleSequential(test_flags, lz2);
    // std::cout << "\nExcess entropy by shuffle sequential: ";
    // for (auto x : lz2.excess_entropy_shuffle) std::cout << x << " ";
    std::cout << std::endl;

    if(opt.args.excess_line > 0) {
        // TODO: Implement excess of entropy for a sequence and get struct with info
    }

    lz::InformationDistanceBySequence(test_flags);
    std::cout << "Info distance in sequences: ";
    for (auto x : test_flags.sequence_info_distance) std::cout << x << " ";
    std::cout << std::endl;

    if (opt.find_distance) {
        lz::InformationDistance(test_flags, lz);

        std::cout << "Info distance: ";
        for (auto x : lz.info_distance) std::cout << x << " ";
        std::cout << std::endl;
    }

    if (opt.save_results) save_data(test_flags, lz, opt);

    return EXIT_SUCCESS;
}

auto main(int argc, char const* argv[]) -> int {
    cxxopts::Options options("lz", "lz76 analysis engine v0.6 2023 by EAP.\nSend bug reports to estevez@fisica.uh.cu or efrenaragon96@gmail.com.\n");

    options.custom_help("[OPTIONS...] input_data");
    options.allow_unrecognised_options();
    options.add_options("lz")
        ("h,help", "Show the help of the program.") // a bool parameter
        // ("a,suffix-algorithm", "Suffix array algorithm.", cxxopts::value<std::string>()->default_value("caps"), "caps | sais")
        ("c,chunks", "Number of partitions for data array (only for caps algorithm).", cxxopts::value<lz::lz_int>()->default_value("20"), "num")
        ("m,max-context", "Max context for suffix comparisons (only for caps algorithm).", cxxopts::value<lz::lz_int>()->default_value("0"), "num")
        ("o,output", "Output file path for results (json format).", cxxopts::value<std::string>()->default_value("result.json"), "file_name")
        ("M,multi-line", "Treat each line in the input stream as a different sequence.")
        // ("t,text", "Treat 'input_data' as text.")
        ("f,factors", "Print the factors and save them in the output file.")
        ("e,excess", "Outputs the excess entropy list of a given line.", cxxopts::value<lz::lz_int>()->default_value("-1"), "value")
        ("E,max-excess", "Maximum iteration value for the excess entropy calculation by shuffling.", cxxopts::value<lz::lz_int>()->default_value("0"), "value")
        ("p,process", "Clear input data.")
        ("d,dlz", "The LZ distance is calculated between successive sequences. Only valid for multisequence file (-M option).")
        ("C,csv", "Input file has csv format.")
        ("S,save", "Save results in an output file.")
        ("v,verbose", "Verbose output.", cxxopts::value<bool>()->default_value("false"));

    try {
        auto result = options.parse(argc, argv);

        if (result["h"].count() || result["help"].count()) {
            std::cout << lz::GREEN_COLOR << options.help() << lz::END_COLOR;
            return EXIT_SUCCESS;
        }

        // lz::utils::read_input("/Users/efren_aragon/Documents/Work/Investigacion/Softwares/library/data/ecoli.fa", text);
        // lz::utils::read_input("/Users/efren_aragon/Documents/Work/Investigacion/Softwares/library/data/sequence1000.bin", text);
        // lz::utils::read_input("/Users/efren_aragon/Documents/Work/Investigacion/Softwares/library/data/sequence1000000.bin", text);
        // lz::utils::read_input("/Users/efren_aragon/Documents/Work/Investigacion/Softwares/library/data/simpletest2", text);

        auto opt = process_args(result);

        if (process(opt)) { return EXIT_SUCCESS; }
        else { return EXIT_FAILURE; }
    }
    catch (Errors er) {
        std::string msg = "Error: " + std::to_string(er.type);
        msg += "\nMessage => " + er.msg;
        std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, msg) << std::endl;
        // std::cerr << lz::GREEN_COLOR << options.help() << lz::END_COLOR << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception err) {
        std::string msg{ err.what() };
        std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, "Fatal error BOOM!!!" + msg) << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        // std::string msg{ err.what() };
        std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, "Fatal error BOOM!!!") << std::endl;
        return EXIT_FAILURE;
    }
}
