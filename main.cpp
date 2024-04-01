#include "main/main.h"

void save_data(lz::utils::LZ_Flags& flags, lz::utils::LZ_Output& results, lz_options& opt) {
   nlohmann::json out_data;

   for (std::size_t i = 0; i < flags.input.size(); i++) {
      // out_data["input"] = opt.input;
      out_data["sequences"][i]["sequence_size"] = flags.input[i].size();

      out_data["sequences"][i]["complexity"]      = results.data[i].getComplexity();
      out_data["sequences"][i]["entropy_density"] = results.data[i].getEntropyDensity();

      auto rsc = results.data[i].getRandomShuffleComplexity();

      out_data["sequences"][i]["random_shuffle_complexity"]["value"]      = rsc.excess_value;
      out_data["sequences"][i]["random_shuffle_complexity"]["block_size"] = rsc.max_block_size;
      out_data["sequences"][i]["multi_information"]                       = rsc.multi_information;
      if (rsc.excess_by_terms.size() > 0) {
         out_data["sequences"][i]["random_shuffle_complexity"]["terms"] = rsc.excess_by_terms;
      }

      out_data["sequences"][i]["extra"]["lz_rajski_distance"]     = results.data[i].getExtras().lz_rajski_distance;
      out_data["sequences"][i]["extra"]["fh_uncertainty"]         = results.data[i].getExtras().fh_uncertainty;
      out_data["sequences"][i]["extra"]["lh_uncertainty"]         = results.data[i].getExtras().lh_uncertainty;
      out_data["sequences"][i]["extra"]["redundancy"]             = results.data[i].getExtras().redundancy;
      out_data["sequences"][i]["extra"]["lz_pearson_coefficient"] = results.data[i].getExtras().lz_pearson_coefficient;

      // if (results.complexity.size()) {
      //    out_data["sequences"][i]["complexity"] = results.complexity[i];
      // }

      // if (results.entropy_density.size()) {
      //    out_data["sequences"][i]["entropy_density"] = results.entropy_density[i];
      // }

      // if (results.lz_effective_complexity.size()) {
      //    out_data["sequences"][i]["lz_effective_complexity"] = results.lz_effective_complexity[i];
      // }

      // if (results.excess_entropy_dist.size()) {
      //    out_data["sequences"][i]["excess_entropy_dist"] = results.excess_entropy_dist[i];
      // }
      // if (results.extra.size()) {
      //    out_data["sequences"][i]["extra"]["lz_rajski_distance"]     = results.extra[i].lz_rajski_distance;
      //    out_data["sequences"][i]["extra"]["fh_uncertainty"]         = results.extra[i].fh_uncertainty;
      //    out_data["sequences"][i]["extra"]["lh_uncertainty"]         = results.extra[i].lh_uncertainty;
      //    out_data["sequences"][i]["extra"]["redundancy"]             = results.extra[i].redundancy;
      //    out_data["sequences"][i]["extra"]["lz_pearson_coefficient"] = results.extra[i].lz_pearson_coefficient;
      // }

      // if (results.sequence_info_distance.size()) {
      //    out_data["sequences"][i]["info_distance_sequence"] = results.sequence_info_distance[i];
      // }

      // if (results.mutual_information.size()) {
      //    out_data["sequences"][i]["mutual_information"] = results.mutual_information[i];
      // }

      // if (results.random_shuffle_complexity.size()) {
      //    out_data["sequences"][i]["random_shuffle_complexity"]["value"] = results.random_shuffle_complexity[i].value;
      //    out_data["sequences"][i]["random_shuffle_complexity"]["block_size"] =
      //       results.random_shuffle_complexity[i].block_size;

      //    if (results.random_shuffle_complexity[i].terms.line == i + 1) {
      //       out_data["sequences"][i]["random_shuffle_complexity"]["terms"] =
      //          results.random_shuffle_complexity[i].terms.terms;
      //    }
      // }

      if (flags.sa_args.block_size >= 0) {
         // out_data["sequences"][i]["whole_random_shuffle_complexity"]["value"] =
         //    results.whole_random_shuffle_complexity[i].value;
         // out_data["sequences"][i]["whole_random_shuffle_complexity"]["block_size"] =
         //    results.whole_random_shuffle_complexity[i].block_size;

         // if (results.whole_random_shuffle_complexity[i].terms.line == i + 1) {
         //    out_data["sequences"][i]["whole_random_shuffle_complexity"]["terms"] =
         //       results.whole_random_shuffle_complexity[i].terms.terms;
         // }

         auto w_rsc = results.data[i].getWholeRandomShuffleComplexity();

         out_data["sequences"][i]["random_shuffle_complexity"]["value"]      = w_rsc.excess_value;
         out_data["sequences"][i]["random_shuffle_complexity"]["block_size"] = w_rsc.max_block_size;
         if (w_rsc.excess_by_terms.size() > 0) {
            out_data["sequences"][i]["random_shuffle_complexity"]["terms"] = w_rsc.excess_by_terms;
         }
      }

      // if (results.multi_information.size()) {
      //    out_data["sequences"][i]["multi_information"] = results.multi_information[i];
      // }

      if (opt.find_distance && i < flags.input.size() - 1) {
         out_data["sequences"][i]["info_distance"] = results.info_distance[i];
      }

      if (!opt.multiLine)
         break;
   }

   std::ofstream out(opt.output);
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
                << opt.input << std::endl
                << std::endl;
   }
   if (opt.is_csv) {
      // read_csv(opt.input, data);
      exit(0);
   } else if (opt.preprocess) {
      multiLineToOneLine(opt.input, data, true);
   } else {
      data = read_input(opt.input, opt.multiLine, opt.input_format);
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
   lz::LempelZivFactorization(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Complexity: ";
      for (auto x: lz.data)
         std::cout << x.getComplexity() << " ";
      std::cout << std::endl;
      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }

   if (!opt.factors_output.empty()) {
      for (auto seq: test_flags.input) {
         auto flz = lz::LempelZivFactors(seq);
         std::cout << "Factors: [ ";
         for (auto f: flz.lzf)
            std::cout << f << " ";
         std::cout << "]" << std::endl;

         std::cout << "epsilon: " << flz.epsilon << "\n";

         auto en = lz::LZNormalError(seq);
         auto ep = lz::LZPoisonError(seq);
         std::cout << "Errors: " << en << " " << ep << std::endl;
         // std::string txt = seq.toString();
         // for (int i = 1; i < flz.lzf.size(); i++) {
         //    std::cout << txt.substr(flz.lzf[i - 1], flz.lzf[i]) + ".";
         // }
         // std::cout << "]" << std::endl;
      }
   }

   // App functions
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating Entropy density\n" << lz::END_COLOR;
      init_time = now();
   }
   lz::EntropyDensity(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Entropy: ";
      for (auto x: lz.data)
         std::cout << x.getEntropyDensity() << " ";
      std::cout << std::endl;
      std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   }

   // if (opt.verbose) {
   //    std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating LZ effective complexity\n"
   //              << lz::END_COLOR;
   //    init_time = now();
   // }
   // // Excess entropy by distance (mutual information)
   // lz::LZEffectiveComplexity(test_flags, lz);
   // // lz::LZEffectiveComplexityNormalized(test_flags, lz);
   // if (opt.verbose) {
   //    const auto end_time = now();
   //    std::cout << "Excess entropy as MI: ";
   //    for (auto x: lz.lz_effective_complexity)
   //       std::cout << x << " ";
   //    std::cout << std::endl;
   //    std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   // }

   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating extra measures\n" << lz::END_COLOR;
      init_time = now();
   }
   // Excess entropy by distance
   // lz::ExcessEntropyDistance(test_flags, lz);
   lz::ExtraMeasures(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      // std::cout << "Excess entropy by distance: ";
      // for (auto x: lz.excess_entropy_dist) std::cout << x << " ";
      // std::cout << std::endl;
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
   lz::RandomShuffleComplexity(test_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << "Random shuffle complexity using Z sequence: ";
      for (auto x: lz.data)
         std::cout << x.getRandomShuffleComplexity().excess_value << " ";
      std::cout << std::endl;
      for (auto i = 0ul; i < lz.data.size(); i++) {
         auto x = lz.data[i];
         if (x.getRandomShuffleComplexity().excess_by_terms.size() > 0) {
            std::cout << "Shuffle entropy terms of line: " << i + 1 << " [ ";
            for (auto t: x.getRandomShuffleComplexity().excess_by_terms)
               std::cout << t << " ";
            std::cout << "]\n";
         }
      }
      std::cout << "Multi information: ";
      for (auto x: lz.multi_information)
         std::cout << x << " ";
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
      lz::WholeRandomShuffleComplexity(test_flags, lz);
      if (opt.verbose) {
         const auto end_time = now();
         std::cout << "Random shuffle complexity using whole sequence: ";
         for (auto x: lz.data)
            std::cout << x.getWholeRandomShuffleComplexity().excess_value << " ";
         std::cout << std::endl;
         for (auto i = 0ul; i < lz.data.size(); i++) {
            auto x = lz.data[i];
            if (x.getWholeRandomShuffleComplexity().excess_by_terms.size() > 0) {
               std::cout << "Shuffle entropy terms of line: " << i + 1 << " [ ";
               for (auto t: x.getWholeRandomShuffleComplexity().excess_by_terms)
                  std::cout << t << " ";
               std::cout << "]\n";
            }
         }
         std::cout << "Multi information: ";
         for (auto x: lz.multi_information)
            std::cout << x << " ";
         std::cout << std::endl;

         std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
      }
   }

   // if (opt.verbose) {
   //    std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating information distance in
   //    sequences\n"
   //              << lz::END_COLOR;
   //    init_time = now();
   // }
   // // lz::InformationDistanceBySequence(test_flags, lz);
   // // lz::MutualInformationBySequence(test_flags, lz);
   // lz::RandomShuffleDistanceBySequence(test_flags, lz);
   // if (opt.verbose) {
   //    const auto end_time = now();
   //    std::cout << "Info distance in sequences: ";
   //    for (auto x: lz.sequence_info_distance) std::cout << x << " ";
   //    std::cout << std::endl;
   //    std::cout << "Finished in: " << duration(end_time - init_time) << " s" << std::endl << std::endl;
   // }

   if (opt.find_distance) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++
                   << ". Calculating information distance between consecutive sequences\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      // lz::InformationDistance(test_flags, lz);
      lz::RandomShuffleDistance(test_flags, lz);

      if (opt.verbose) {
         const auto end_time = now();
         std::cout << "Info distance: ";
         for (auto x: lz.info_distance)
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
                            "lz76 analysis engine v0.8 2024 by EAP.\nSend bug reports to estevez@fisica.uh.cu or "
                            "efrenaragon96@gmail.com.\n");

   // clang-format off
   options.custom_help("[OPTIONS...] input_data")
          .set_width(120)
          .set_tab_expansion(true);
   options.allow_unrecognised_options();
   // clang-format on
   auto opt_group = options.add_options("lzcomplexity");
   opt_group("a,alphabet",
             "Alphabet cardinality. If auto it tries to guess the alphabet size. Default is 2",
             cxxopts::value<std::string>()->default_value("2"),
             "value");
   opt_group("C,csv", "Input file has csv format.");
   opt_group("d,dlz",
             "The LZ distance is calculated between consecutive lines. Only valid for multiline files (-m "
             "option).");
   opt_group("e,entropy-shuffle",
             "Computes shuffle entropy deficit and may return the list of shuffle terms for specific lines. format: "
             "[#|a]:f:#:#, where the first section says the max size of the block for shuffling (a for automatic "
             "size), second says if save the terms and the last two numbers are the range of lines in the file for it. "
             "In case of missing  numbers for the range the terms will be save for every line",
             cxxopts::value<std::vector<std::string>>()->delimiter(':')->default_value(""),
             "value");
   opt_group("f,factors", "Save the factorization.", cxxopts::value<std::string>()->default_value(""), "file_name");
   opt_group("F,format",
             "Input file format. TXT for raw text format. CSV the input file is a csv array. PBM, PGM and PNM is for "
             "the family of the graphic formats.",
             cxxopts::value<std::string>()->default_value("TXT"),
             "value");
   opt_group("h,help", "Show the help of the program.");
   opt_group("j,jobs",
             "Configure number of threads.",
             cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())),
             "value");
   opt_group("l,log-base",
             "Configure the log base value. The default is the alphabet cardinality.",
             cxxopts::value<std::string>()->default_value(""),
             "value");
   opt_group("m,multi-line", "Treat each line in the input stream as a different sequence.");
   opt_group("n,entropy-density", "Computes only the entropy density.");
   opt_group("o,output",
             "Output filepath for results. Default appends to the end of input file a .json extension",
             cxxopts::value<std::string>()->default_value(""),
             "file_name");
   opt_group("p,partitions",
             "Number of partitions used for the parallel suffix array algorithm.",
             cxxopts::value<lz::lz_int>()->default_value("20"),
             "value");
   opt_group("v,verbose", "Verbose output.", cxxopts::value<bool>()->default_value("false"));

   opt_group("r,process", "Clear input data.");
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
