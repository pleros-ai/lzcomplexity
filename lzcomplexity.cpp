#include <lz/lzApp.h>

#include "main/config.h"
#include "main/main.h"

void save_data(lz::utils::LZ_Flags& flags, lz::utils::LZ_Output& results, lz_options& opt) {
   nlohmann::json out_data;

   out_data["filename"] = opt.input;
   out_data["format"]   = opt.input_format;
   out_data["size"]     = flags.input.size();

   for (std::size_t i = 0; i < flags.input.size(); i++) {
      // out_data["input"] = opt.input;
      out_data["sequences"][i]["size"] = flags.input[i].size();

      auto lz_result = results.data[i];

      out_data["sequences"][i]["lz76Complexity"]     = lz_result.getComplexity();
      out_data["sequences"][i]["lz76EntropyDensity"] = lz_result.getEntropyDensity();

      auto rsc = lz_result.getRandomShuffleComplexity();

      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["value"]             = rsc.excess_value;
      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["max_block_size"]    = rsc.max_block_size;
      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["multi_information"] = rsc.multi_information;
      if (rsc.summands.size() > 0) {
         out_data["sequences"][i]["lz76RandomShuffleComplexity"]["summands"] = rsc.summands;
      }

      out_data["sequences"][i]["lz76Extra"]["lz_rajski_distance"]     = lz_result.getExtras().lz_rajski_distance;
      out_data["sequences"][i]["lz76Extra"]["fh_uncertainty"]         = lz_result.getExtras().fh_uncertainty;
      out_data["sequences"][i]["lz76Extra"]["lh_uncertainty"]         = lz_result.getExtras().lh_uncertainty;
      out_data["sequences"][i]["lz76Extra"]["redundancy"]             = lz_result.getExtras().redundancy;
      out_data["sequences"][i]["lz76Extra"]["lz_pearson_coefficient"] = lz_result.getExtras().lz_pearson_coefficient;

      if (opt.args.block_size >= 0) {
         auto w_rsc = lz_result.getAllRandomShuffleComplexity();

         out_data["sequences"][i]["lz76WholeRandomShuffleComplexity"]["value"]             = w_rsc.excess_value;
         out_data["sequences"][i]["lz76WholeRandomShuffleComplexity"]["max_block_size"]    = w_rsc.max_block_size;
         out_data["sequences"][i]["lz76WholeRandomShuffleComplexity"]["multi_information"] = w_rsc.multi_information;
         if (w_rsc.summands.size() > 0) {
            out_data["sequences"][i]["lz76WholeRandomShuffleComplexity"]["summands"] = w_rsc.summands;
         }
      }

      if (!opt.multiLine)
         break;
   }

   if (opt.find_distance) {
      out_data["lz76Distance"]["RandomShuffleDistance"] = results.random_shuffle_distance;
      out_data["lz76Distance"]["InformationDistance"]   = results.info_distance;
   }

   std::ofstream out(opt.output);
   if (out.is_open() && out.good()) {
      out << out_data;
      out_data.clear();
      out.close();
   }
}

void save_factors(lz::utils::LZ_Flags& flags, std::vector<lz::internal::LZ_Result> data, lz_options& opt) {
   nlohmann::json out_data;

   out_data["filename"] = opt.input;
   out_data["format"]   = opt.input_format;
   out_data["size"]     = flags.input.size();

   for (auto i = 0ul; i < data.size(); i++) {
      out_data["factors"][i] = data[i].lzf;
   }

   std::ofstream out(opt.factors_output);
   if (out.is_open() && out.good()) {
      out << out_data;
      out_data.clear();
      out.close();
   }
}

lz::lz_int process(lz_options& opt) {
   std::vector<lz::sequence> data;

   auto           g_now         = now();
   unsigned short verbose_index = 1;

   time_point_t init_time;
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2. Processing the data...\n" << lz::END_COLOR;
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Reading input data from: " << lz::END_COLOR
                << opt.input << std::endl;
   }
   if (opt.preprocess) {
      multiLineToOneLine(opt.input, data, true);
   } else {
      data = read_input(opt.input, opt.multiLine, opt.input_format);
   }

   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "Sequence to process: " << data.size() << std::endl << std::endl << lz::END_COLOR;
   }

   //? Input flags
   lz::utils::LZ_Flags test_flags(data, opt.args);
   test_flags.shuffle_init_line = opt.excess_init_line;
   test_flags.shuffle_end_line  = opt.excess_end_line;
   //? Results
   lz::utils::LZ_Output lz(data.size());

   lz::utils::EnabledMT(opt.n_jobs);

   // App functions
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating lz76 factorization\n" << lz::END_COLOR;
      init_time = now();
   }
   lz::lz76Factorization(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Complexity: ";
      for (auto x: lz.data)
         std::cout << x.getComplexity() << " ";
      std::cout << std::endl;
      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }

   if (!opt.factors_output.empty()) {
      std::vector<lz::internal::LZ_Result> f;
      for (auto seq: test_flags.input) {
         auto flz = lz::lz76Factors(seq);
         f.push_back(flz);
         std::cout << "Factors: [ ";
         for (auto f: flz.lzf)
            std::cout << f << " ";
         std::cout << "]" << std::endl;

         std::cout << "epsilon: " << flz.epsilon << "\n";

         auto en = lz::lz76NormalError(seq);
         auto ep = lz::lz76PoisonError(seq);
         std::cout << "Errors: " << en << " " << ep << std::endl;
         // std::string txt = seq.toString();
         // for (int i = 1; i < flz.lzf.size(); i++) {
         //    std::cout << txt.substr(flz.lzf[i - 1], flz.lzf[i]) + ".";
         // }
         // std::cout << "]" << std::endl;
      }

      save_factors(test_flags, f, opt);
   }

   // App functions
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating Entropy density\n" << lz::END_COLOR;
      init_time = now();
   }
   lz::lz76EntropyDensity(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Entropy: ";
      for (auto x: lz.data)
         std::cout << x.getEntropyDensity() << " ";
      std::cout << std::endl;
      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }

   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating extra measures\n" << lz::END_COLOR;
      init_time = now();
   }
   // Excess entropy by distance
   lz::lz76ExtraMeasures(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "LZ Rajski distance: ";
      for (auto x: lz.data)
         std::cout << x.getExtras().lz_rajski_distance << " ";
      std::cout << std::endl;
      std::cout << "FH Uncertainly: ";
      for (auto x: lz.data)
         std::cout << x.getExtras().fh_uncertainty << " ";
      std::cout << std::endl;
      std::cout << "LH Uncertainly: ";
      for (auto x: lz.data)
         std::cout << x.getExtras().lh_uncertainty << " ";
      std::cout << std::endl;
      std::cout << "LZ Redundancy: ";
      for (auto x: lz.data)
         std::cout << x.getExtras().redundancy << " ";
      std::cout << std::endl;
      std::cout << "LZ Pearson coefficient: ";
      for (auto x: lz.data)
         std::cout << x.getExtras().lz_pearson_coefficient << " ";
      std::cout << std::endl;
      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++
                << ". Calculating random shuffle complexity using Z sequence\n"
                << lz::END_COLOR;
      init_time = now();
   }
   lz::lz76RandomShuffleComplexity(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Random shuffle complexity using Z sequence: ";
      for (auto x: lz.data)
         std::cout << x.getRandomShuffleComplexity().excess_value << " ";
      std::cout << std::endl;
      for (auto i = 0ul; i < lz.data.size(); i++) {
         auto x = lz.data[i];
         if (x.getRandomShuffleComplexity().summands.size() > 0) {
            std::cout << "Shuffle entropy terms of line: " << i + 1 << " [ ";
            for (auto t: x.getRandomShuffleComplexity().summands)
               std::cout << t << " ";
            std::cout << "]\n";
         }
      }
      std::cout << "Multi information: ";
      for (auto x: lz.data)
         std::cout << x.getRandomShuffleComplexity().multi_information << " ";
      std::cout << std::endl;

      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }

   if (opt.args.block_size >= 0) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++
                   << ". Calculating random shuffle complexity using whole sequence\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      // Excess entropy by shuffle
      lz::lz76WholeRandomShuffleComplexity(test_flags, lz);
      if (opt.verbose) {
         const auto end_time = now();
         std::cout << "Random shuffle complexity using whole sequence: ";
         for (auto x: lz.data)
            std::cout << x.getAllRandomShuffleComplexity().excess_value << " ";
         std::cout << std::endl;
         for (auto i = 0ul; i < lz.data.size(); i++) {
            auto x = lz.data[i];
            if (x.getAllRandomShuffleComplexity().summands.size() > 0) {
               std::cout << "Shuffle entropy terms of line: " << i + 1 << " [ ";
               for (auto t: x.getAllRandomShuffleComplexity().summands)
                  std::cout << t << " ";
               std::cout << "]\n";
            }
         }
         std::cout << "Multi information: ";
         for (auto x: lz.data)
            std::cout << x.getAllRandomShuffleComplexity().multi_information << " ";
         std::cout << std::endl;

         std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
      }
   }

   if (opt.find_distance) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++
                   << ". Calculating information distance between consecutive sequences\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      std::vector<double> rand_dist;

      // lz::InformationDistance(test_flags, lz);
      lz::lz76InformationDistance(test_flags, lz);
      lz::lz76RandomShuffleDistance(test_flags, lz);

      // for (int i = 10; i <= 60; i += 5) {
      //    test_flags.sa_args.block_size = i;

      //    lz::RandomShuffleDistance(test_flags, lz);
      //    rand_dist.push_back(lz.random_shuffle_distance[0]);
      // }

      // for (auto x: rand_dist)
      //    std::cout << x << " ";
      // std::cout << std::endl;

      if (opt.verbose) {
         const auto end_time = now();
         std::cout << "Info distance: ";
         for (auto x: lz.info_distance)
            std::cout << x << " ";
         std::cout << std::endl;
         std::cout << "Random info distance: ";
         for (auto x: lz.random_shuffle_distance)
            std::cout << x << " ";
         std::cout << std::endl;
         std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
      }
   }

   auto g_end = now();
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Saving results in: " << opt.output << std::endl
                << lz::END_COLOR;
      init_time = now();
   }
   save_data(test_flags, lz, opt);

   if (opt.verbose) {
      std::cout << "Total time elapsed: " << duration(g_end - g_now) << " s" << std::endl;
   }
   lz::utils::DisabledMT();
   return EXIT_SUCCESS;
}

auto main(int argc, char const* argv[]) -> int {
   cxxopts::options options("lz",
                            "LempelZiv-76 complexity utilities as a library and also a standalone software. Suited for "
                            "complexity analysis of time series.\nSend bug reports to estevez@fisica.uh.cu or "
                            "efrenaragon96@gmail.com.\n");

   // clang-format off
   options.custom_help("[OPTIONS...] input_data")
          .set_width(120)
          .set_tab_expansion(true);
   options.allow_unrecognised_options();
   // clang-format on
   auto opt_group = options.add_options("lzcomplexity");
   opt_group("a,alphabet",
             "Alphabet cardinality. If auto it tries to guess the alphabet size.",
             cxxopts::value<std::string>()->default_value("2"),
             "value");
   opt_group("d,dlz",
             "The LZ distance is calculated between consecutive lines. Only valid for multiline files (-m "
             "option).");
   opt_group(
      "e,entropy-shuffle",
      "Random shuffle complexity with whole sequence. v1: maximum value for block shuffling, f: summands output, v2: "
      "starting line for summands output, v3: ending line for summands output. All values are optionals",
      cxxopts::value<std::vector<std::string>>()->delimiter(':')->implicit_value("a"),
      "[v1]:[f]:[v2]:[v3]");
   opt_group("f,factors", "Saves the factorization.", cxxopts::value<std::string>(), "file_name");
   opt_group("F,format",
             "Input file format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and PNM is for "
             "the family of the graphic formats.",
             cxxopts::value<std::string>()->default_value("AUTO"),
             "value");
   opt_group("h,help", "Show this message.");
   opt_group("j,jobs",
             "Number of threads.",
             cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())),
             "value");
   opt_group("l,log-base",
             "The log base value. The default is the alphabet cardinality.",
             cxxopts::value<std::string>(),
             "value");
   opt_group("m,multi-line", "Treat each line in the input stream as a different sequence.");
   opt_group("n,entropy-density", "Computes only the entropy density.");
   opt_group("o,output",
             "Output filename. Default appends to the end of input file a .json extension",
             cxxopts::value<std::string>(),
             "file_name");
   opt_group("p,partitions",
             "Number of partitions used for the parallel suffix array algorithm.",
             cxxopts::value<lz::lz_int>()->default_value("20"),
             "value");
   opt_group("v,verbose", "Verbose output.", cxxopts::value<bool>()->default_value("false"));

   // opt_group("r,process", "Clear input data.");
   //    opt_group("m,max-context", "Max context for suffix comparisons (only for caps algorithm).",
   //              cxxopts::value<lz::lz_int>()->default_value("0"), "num");
   try {
      auto result = options.parse(argc, argv);

      if (result["h"].count() || result["help"].count() || result.arguments().size() == 0) {
         std::cout << lz::GREEN_COLOR << options.help() << lz::END_COLOR;
         return EXIT_SUCCESS;
      }

      auto opt = process_args(result);

      if (process(opt)) {
         return EXIT_SUCCESS;
      } else {
         return EXIT_FAILURE;
      }
   } catch (Errors er) {
      std::string msg = "Error: " + std::to_string(er.type);
      msg += "\nMessage => " + er.msg;
      std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, msg) << std::endl;
      // std::cerr << lz::GREEN_COLOR << options.help() << lz::END_COLOR << std::endl;
      return EXIT_FAILURE;
   } catch (std::exception err) {
      std::string msg{err.what()};
      std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, "Fatal error BOOM!!!" + msg) << std::endl;
      return EXIT_FAILURE;
   } catch (...) {
      // std::string msg{ err.what() };
      std::cerr << std::endl << print_msg(lz::utils::MSG_TYPE::ERROR, "Fatal error BOOM!!!") << std::endl;
      return EXIT_FAILURE;
   }
}
