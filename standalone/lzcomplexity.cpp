#include "inc/lzcomplexity.h"

#include <lz/lz.h>

#include <filesystem>
#include <fstream>
#include <lz/lzApp.hpp>
#include <ostream>
#include <string>

#include "inc/config.h"
#include "inc/messages.h"
#include "json.hpp"
#include "lz/exceptions.h"
#include "lz/general.h"
#include "lz/utils.h"

#define VERSION "v0.9.1"

using MSG = lz::utils::MSG_TYPE;

void save_data(lz::utils::LZ_Flags& flags, lz::utils::LZ_Output& results, lz_options& opt) {
   nlohmann::json out_data;

   out_data["filename"] = opt.input;
   out_data["format"]   = MagicValues[opt.input_format];
   out_data["size"]     = flags.input.size();

   for (std::size_t i = 0; i < flags.input.size(); i++) {
      // out_data["input"] = opt.input;
      out_data["sequences"][i]["size"] = flags.input[i].size();

      auto lz_result = results.data[i];

      out_data["sequences"][i]["lz76Complexity"]     = lz_result.getComplexity();
      out_data["sequences"][i]["lz76EntropyDensity"] = lz_result.getEntropyDensity();

      auto w_rsc = lz_result.getRandomShuffleComplexity();

      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["value"]             = w_rsc.excess_value;
      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["max_block_size"]    = w_rsc.max_block_size;
      out_data["sequences"][i]["lz76RandomShuffleComplexity"]["multi_information"] = w_rsc.multi_information;
      if (w_rsc.summands.size() > 0) {
         out_data["sequences"][i]["lz76RandomShuffleComplexity"]["summands"] = w_rsc.summands;
      }

      if (opt.args.block_size >= 0) {
         auto rsc = lz_result.getPairedShuffleComplexity();

         out_data["sequences"][i]["lz76PairedShuffleComplexity"]["value"]             = rsc.excess_value;
         out_data["sequences"][i]["lz76PairedShuffleComplexity"]["max_block_size"]    = rsc.max_block_size;
         out_data["sequences"][i]["lz76PairedShuffleComplexity"]["multi_information"] = rsc.multi_information;
         if (rsc.summands.size() > 0) {
            out_data["sequences"][i]["lz76PairedShuffleComplexity"]["summands"] = rsc.summands;
         }
      }

      if (!opt.multiLine)
         break;
   }

   if (opt.find_distance) {
      out_data["lz76Distance"]["RandomShuffleDistance"] = results.random_shuffle_distance;
      out_data["lz76Distance"]["InformationDistance"]   = results.info_distance;
   }

   if (opt.mixed_entropy) {
      out_data["lz76MixedEntropyDensity"] = results.mixed_entropy_density;
   }

   std::ofstream out(opt.output);
   if (out.is_open() && out.good()) {
      out << out_data;
      out.close();
   }

   out_data.clear();
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
   bool           notify_warm   = false;
   std::ofstream  out_log(opt.input + ".log");

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

   bool ignore_parallel = false;
   if (opt.verbose) {
      std::cout << print_msg(MSG::INFO, "Sequence to process: " + std::to_string(data.size())) << std::endl;
      std::map<lz::lz_int, lz::lz_int> map_size;
      for (auto&& x: data) {
         if (x.length() < 5e3) {
            ignore_parallel = true;
         }

         if (map_size.count(x.size()) > 0) {
            map_size[x.size()] += 1;
         } else {
            map_size[x.size()] = 1;
         }
      }

      for (auto [size, count]: map_size) {
         std::cout << print_msg(MSG::INFO,
                                std::to_string(count) + " sequence with " + std::to_string(size) + " characters")
                   << std::endl;
      }

      if (opt.warn_out)
         std::cout << std::endl;
   }

   if (!opt.warn_out) {
      std::cout << print_msg(MSG::WARRING, warn_data_size) << std::endl << std::endl;
   }

   //? Input flags
   lz::utils::LZ_Flags in_flags(data, opt.args);
   in_flags.shuffle_init_line = opt.excess_init_line;
   in_flags.shuffle_end_line  = opt.excess_end_line;
   //? Results
   lz::utils::LZ_Output lz(data.size());

   if (ignore_parallel) {
      in_flags.sa_args.chunks = 1;
      notify_warm             = true;
      out_log << warn_data_size << std::endl;
   }

   // std::string s;
   // std::string ch = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

   // auto size = 15;
   // for (int i = 0; i < 1e7; i++) {
   //    s += ch[rand() % size];
   // }
   // in_flags.input[0] = s;

   lz::utils::EnabledMT(opt.n_jobs);

   // App functions
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating lz76 factorization\n" << lz::END_COLOR;
      init_time = now();
   }
   lz::lz76Factorization(in_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << print_msg(MSG::INFO, "Complexity: ");
      for (auto x: lz.data)
         std::cout << x.getComplexity() << " ";
      std::cout << std::endl;
      std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                << std::endl
                << std::endl;
   }

   if (!opt.factors_output.empty()) {
      std::vector<lz::internal::LZ_Result> f;
      for (auto seq: in_flags.input) {
         auto flz = lz::lz76Factors(seq);
         f.push_back(flz);
         std::cout << print_msg(MSG::INFO, "Factors: [ ");
         for (auto f: flz.lzf)
            std::cout << f << " ";
         std::cout << "]" << std::endl;

         std::cout << print_msg(MSG::INFO, "epsilon: " + std::to_string(flz.epsilon)) << "\n";

         auto en = lz::lz76NormalError(seq);
         auto ep = lz::lz76PoisonError(seq);
         std::cout << print_msg(MSG::INFO, "Normal error: " + std::to_string(en)) << std::endl;
         std::cout << print_msg(MSG::INFO, "Poison error: " + std::to_string(ep)) << std::endl;
         // std::string txt = seq.toString();
         // for (int i = 1; i < flz.lzf.size(); i++) {
         //    std::cout << txt.substr(flz.lzf[i - 1], flz.lzf[i]) + ".";
         // }
         // std::cout << "]" << std::endl;
      }

      save_factors(in_flags, f, opt);
   }

   // App functions
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating Entropy density\n" << lz::END_COLOR;
      init_time = now();
   }
   lz::lz76EntropyDensity(in_flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << print_msg(MSG::INFO, "Entropy: ");
      for (auto x: lz.data)
         std::cout << x.getEntropyDensity() << " ";
      std::cout << std::endl;
      std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                << std::endl
                << std::endl;
   }

   if (opt.entropy_density) {
      auto g_end = now();
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Saving results in: " << opt.output << std::endl
                   << lz::END_COLOR;
         init_time = now();
      }
      save_data(in_flags, lz, opt);

      if (opt.verbose) {
         std::cout << print_msg(MSG::INFO, "Total time elapsed: " + std::to_string(duration(g_end - g_now)) + " s")
                   << std::endl;
      }
      lz::utils::DisabledMT();
      return EXIT_SUCCESS;
   }

   if (!opt.entropy_density) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating random shuffle complexity\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      // Excess entropy by shuffle
      lz::lz76RandomShuffleComplexity(in_flags, lz);
      if (opt.verbose) {
         const auto end_time = now();
         std::cout << print_msg(MSG::INFO, "Random shuffle complexity: ");
         for (auto x: lz.data)
            std::cout << x.getRandomShuffleComplexity().excess_value << " ";
         std::cout << std::endl;
         for (auto i = 0ul; i < lz.data.size(); i++) {
            auto x = lz.data[i];
            if (x.getRandomShuffleComplexity().summands.size() > 0) {
               std::cout << print_msg(MSG::INFO, "Shuffle entropy terms of line: " + std::to_string(i + 1) + " [ ");
               for (auto t: x.getRandomShuffleComplexity().summands)
                  std::cout << t << " ";
               std::cout << "]\n";
            }
         }
         std::cout << print_msg(MSG::INFO, "Multi information: ");
         for (auto x: lz.data)
            std::cout << x.getRandomShuffleComplexity().multi_information << " ";
         std::cout << std::endl;

         std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                   << std::endl
                   << std::endl;
      }
   }

   if (opt.args.block_size >= 0 && opt.get_paired_shuffle) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Calculating paired shuffle complexity\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      lz::lz76PairedShuffleComplexity(in_flags, lz);
      if (opt.verbose) {
         const auto end_time = now();
         std::cout << print_msg(MSG::INFO, "Paired shuffle complexity: ");
         for (auto x: lz.data)
            std::cout << x.getPairedShuffleComplexity().excess_value << " ";
         std::cout << std::endl;
         for (auto i = 0ul; i < lz.data.size(); i++) {
            auto x = lz.data[i];
            if (x.getPairedShuffleComplexity().summands.size() > 0) {
               std::cout << print_msg(MSG::INFO,
                                      "Paired shuffle entropy terms of line: " + std::to_string(i + 1) + " [ ");
               for (auto t: x.getPairedShuffleComplexity().summands)
                  std::cout << t << " ";
               std::cout << "]\n";
            }
         }
         std::cout << print_msg(MSG::INFO, "Multi information: ");
         for (auto x: lz.data)
            std::cout << x.getPairedShuffleComplexity().multi_information << " ";
         std::cout << std::endl;

         std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                   << std::endl
                   << std::endl;
      }
   }

   if (opt.mixed_entropy) {
      if (opt.verbose) {
         std::cout << lz::GREEN_COLOR << "2." << verbose_index++
                   << ". Calculating mixed entropy density of consecutive lines\n"
                   << lz::END_COLOR;
         init_time = now();
      }
      // Excess entropy by shuffling
      lz::lz76MixedEntropyDensity(in_flags, lz);
      if (opt.verbose) {
         const auto end_time = now();
         std::cout << print_msg(MSG::INFO, "Mixed entropy density: ");
         for (auto x: lz.mixed_entropy_density)
            std::cout << x << " ";
         std::cout << std::endl;
         std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                   << std::endl
                   << std::endl;
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

      lz::lz76InformationDistance(in_flags, lz);
      // lz::lz76InformationDistanceZ(in_flags, lz);
      lz::lz76RandomShuffleDistance(in_flags, lz);

      if (opt.verbose) {
         const auto end_time = now();
         std::cout << print_msg(MSG::INFO, "Info distance: ");
         for (auto x: lz.info_distance)
            std::cout << x << " ";
         std::cout << std::endl;
         std::cout << print_msg(MSG::INFO, "Random info distance: ");
         for (auto x: lz.random_shuffle_distance)
            std::cout << x << " ";
         std::cout << std::endl;
         std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                   << std::endl
                   << std::endl;
      }
   }

   auto g_end = now();
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Saving results in: " << opt.output << std::endl
                << lz::END_COLOR;
      init_time = now();
   }
   save_data(in_flags, lz, opt);

   if (notify_warm && !opt.warn_out) {
      std::cout << std::endl
                << print_msg(MSG::WARRING, "Warning issues. Check " + opt.input + ".log log file") << std::endl;
      out_log.close();
   } else {
      out_log.close();
      std::filesystem::remove(opt.input + ".log");
   }

   if (opt.verbose) {
      std::cout << print_msg(MSG::INFO, "Total time elapsed: " + std::to_string(duration(g_end - g_now)) + " s")
                << std::endl;
   }
   lz::utils::DisabledMT();
   return EXIT_SUCCESS;
}

auto main(int argc, char const* argv[]) -> int {
   cxxopts::options options("lzcomplexity", header);

   // clang-format off
   options.custom_help("[OPTIONS] <file>")
          .set_width(120)
          .set_tab_expansion(true);
   options.allow_unrecognised_options();
   // clang-format on
   auto opt_group = options.add_options("OPTIONS: ");
   opt_group(opt_list["alphabet"].option_value,
             opt_list["alphabet"].description,
             cxxopts::value<std::string>()->default_value("2"),
             "value");
   opt_group(opt_list["distance"].option_value, opt_list["distance"].description);
   opt_group(opt_list["shuffle"].option_value,
             opt_list["shuffle"].description,
             cxxopts::value<std::vector<std::string>>()->delimiter(':')->implicit_value("a"),
             "[v1]:[f]:[v2]:[v3]");
   opt_group(
      opt_list["factors"].option_value, opt_list["factors"].description, cxxopts::value<std::string>(), "file_name");
   opt_group(opt_list["format"].option_value,
             opt_list["format"].description,
             cxxopts::value<std::string>()->default_value("AUTO"),
             "value");
   opt_group(opt_list["help"].option_value, opt_list["help"].description);
   opt_group(
      opt_list["mixed"].option_value, opt_list["mixed"].description, cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["jobs"].option_value,
             opt_list["jobs"].description,
             cxxopts::value<lz::lz_uint>()->default_value(std::to_string(std::thread::hardware_concurrency())),
             "value");
   opt_group("J,json",
             "Input parameters as json format (ignore all other flags)",
             cxxopts::value<std::string>()->default_value("")->hide(),
             "value");
   opt_group(
      opt_list["log_base"].option_value, opt_list["log_base"].description, cxxopts::value<std::string>(), "value");
   opt_group(opt_list["multi_line"].option_value, opt_list["multi_line"].description);
   opt_group(opt_list["entropy"].option_value, opt_list["entropy"].description);
   opt_group(
      opt_list["output"].option_value, opt_list["output"].description, cxxopts::value<std::string>(), "file_name");
   opt_group(opt_list["partitions"].option_value,
             opt_list["partitions"].description,
             cxxopts::value<lz::lz_int>()->default_value("2"),
             "value");
   opt_group(opt_list["verbose"].option_value,
             opt_list["verbose"].description,
             cxxopts::value<bool>()->default_value("false"));
   opt_group(opt_list["version"].option_value,
             opt_list["version"].description,
             cxxopts::value<bool>()->default_value("false"));
   opt_group(
      opt_list["warn"].option_value, opt_list["warn"].description, cxxopts::value<bool>()->default_value("false"));

   // opt_group("x,extras",
   //           "Computes additional measures based on lz76 (rajski distance, the uncertainty of both halves, pearson "
   //           "coefficient and redundancy).",
   //           cxxopts::value<bool>()->default_value("false"));

   // opt_group("r,process", "Clear input data.");
   //    opt_group("m,max-context", "Max context for suffix comparisons (only for caps algorithm).",
   //              cxxopts::value<lz::lz_int>()->default_value("0"), "num");
   try {
      auto result = options.parse(argc, argv);

      if (result["version"].count() || result["V"].count()) {
         std::cout << print_msg(MSG::INFO, VERSION) << std::endl;
         return EXIT_SUCCESS;
      }

      if (result["h"].count() || result["help"].count() ||
          (result.arguments().empty() && result.unmatched().size() == 0)) {
         std::cout << lz::GREEN_COLOR << options.help() << lz::END_COLOR;
         return EXIT_SUCCESS;
      }

      auto opt = process_args(result);

      if (opt.input.empty()) {
         std::cerr << print_msg(MSG::ERROR, "Input file is missing") << std::endl;
         return EXIT_FAILURE;
      }

      if (process(opt)) {
         return EXIT_SUCCESS;
      } else {
         return EXIT_FAILURE;
      }
   } catch (Errors er) {
      std::cerr << std::endl << print_msg(MSG::ERROR, std::string(er.msg)) << std::endl;
      return EXIT_FAILURE;
   } catch (std::exception err) {
      std::string msg(err.what());
      std::cerr << std::endl << print_msg(MSG::ERROR, "Fatal error BOOM!!! " + msg) << std::endl;
      return EXIT_FAILURE;
   } catch (...) {
      // std::string msg{ err.what() };
      std::cerr << std::endl << print_msg(MSG::ERROR, "Fatal error BOOM!!!") << std::endl;
      return EXIT_FAILURE;
   }
}
