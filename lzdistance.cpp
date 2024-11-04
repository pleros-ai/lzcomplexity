#include "main/lzdistance.hpp"

#include <lz/lz.h>

#include <lzDistance/lzDistanceApp.hpp>

#include "main/config_distance.hpp"

#define VERSION "0.8.8"

using MSG = lz::utils::MSG_TYPE;

void save_data(lz::dist::LZ_Flags& flags, lz::dist::LZ_Output& results, lz_options& opt) {
   nlohmann::json out_data;

   out_data["first_data_source"] = opt.is_first_directory ? opt.first_input_dir : opt.first_input;
   out_data["first_dim"]         = flags.first_input.size();

   if (!opt.first_input.empty() || !opt.first_input_dir.empty()) {
      out_data["second_data_source"] = opt.is_second_directory ? opt.second_input_dir : opt.second_input;
      out_data["second_dim"]         = flags.second_input.size();
   }

   out_data["first_data_source_format"]  = opt.first_input_format;
   out_data["second_data_source_format"] = opt.second_input_format;

   out_data["information_distance"]         = results.info_distance;
   out_data["shuffle_information_distance"] = results.shuffle_distance;

   std::ofstream out(opt.output);
   if (out.is_open() && out.good()) {
      out << out_data;
      out.close();
   }

   out_data.clear();
}

lz::lz_int process(lz_options& opt) {
   std::vector<lz::sequence> first_data;
   std::vector<lz::sequence> second_data;

   auto           g_now         = now();
   unsigned short verbose_index = 1;

   time_point_t init_time;
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2. " << lz::END_COLOR << "Processing the data...\n";
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << lz::END_COLOR
                << ". Reading data sources: " << std::endl
                << lz::GREEN_COLOR << " • " << lz::BLUE_COLOR
                << (opt.is_first_directory ? "[ directory ] " : "[ file ] ") << lz::END_COLOR << lz::END_COLOR
                << (opt.is_first_directory ? opt.first_input_dir : opt.first_input) << std::endl;
      if (!opt.second_input.empty() || !opt.second_input_dir.empty())
         std::cout << lz::GREEN_COLOR << " • " << lz::BLUE_COLOR
                   << (opt.is_second_directory ? "[ directory ] " : "[ file ] ") << lz::END_COLOR << lz::END_COLOR
                   << (opt.is_second_directory ? opt.second_input_dir : opt.second_input) << std::endl
                   << std::endl;
   }

   auto load_first_data = [&first_data, &opt]() {
      if (opt.is_first_directory) {
         first_data = read_dir(opt.first_input_dir, opt.first_input_format);
      } else if (!opt.first_input.empty()) {
         first_data = read_input(opt.first_input, true, opt.first_input_format);
      }
   };

   auto load_second_data = [&second_data, &opt]() {
      if (opt.second_input.empty() && opt.second_input_dir.empty())
         return;

      if (opt.is_second_directory) {
         second_data = read_dir(opt.second_input_dir, opt.second_input_format);
      } else if (!opt.second_input.empty()) {
         second_data = read_input(opt.second_input, true, opt.second_input_format);
      }
   };

   lz::utils::par_do(load_first_data, load_second_data);

   lz::dist::LZ_Flags flags(first_data, second_data, opt.args);
   flags.first_dist_init = opt.is_second_directory ? opt.first_dir_init_line : opt.first_init_line;
   flags.first_dist_end  = opt.is_second_directory ? opt.first_dir_end_line : opt.first_end_line;

   flags.second_dist_init = opt.is_second_directory ? opt.second_dir_init_line : opt.second_init_line;
   flags.second_dist_end  = opt.is_second_directory ? opt.second_dir_end_line : opt.second_end_line;

   flags.revert_     = opt.revert_;
   flags.text_       = opt.text_;
   flags.binary_     = opt.binary_;
   flags.adn_        = opt.adn_;
   flags.trajectory_ = opt.trajectory_;

   const auto first_dim  = first_data.size();
   const auto second_dim = second_data.empty() ? first_data.size() : second_data.size();

   lz::dist::LZ_Output lz(first_dim, second_dim);

   lz::utils::EnabledMT(opt.n_jobs);

   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << lz::END_COLOR
                << ". Calculating information distance matrix\n";
      init_time = now();
   }
   lz::lz76DistanceMatrix(flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << print_msg(MSG::INFO, "Information distance matrix: ");
      for (auto i = 0ul; i < first_dim; i++) {
         if (i > 0)
            std::cout << std::string(39, ' ');
         for (auto j = 0ul; j < second_dim; j++) {
            std::cout << lz.info_distance[i][j] << " ";
         }
         std::cout << std::endl;
      }
      std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                << std::endl
                << std::endl;
   }

   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << lz::END_COLOR
                << ". Calculating shuffle distance matrix\n";
      init_time = now();
   }
   lz::lz76ShuffleDistanceMatrix(flags, lz);
   if (opt.verbose) {
      const auto end_time = now();
      std::cout << print_msg(MSG::INFO, "Shuffle distance matrix: ");
      for (auto i = 0ul; i < first_dim; i++) {
         if (i > 0)
            std::cout << std::string(35, ' ');
         for (auto j = 0ul; j < second_dim; j++) {
            std::cout << lz.shuffle_distance[i][j] << " ";
         }
         std::cout << std::endl;
      }
      std::cout << print_msg(MSG::INFO, "Finished in: " + std::to_string(duration(end_time - init_time)) + " s")
                << std::endl
                << std::endl;
   }

   auto g_end = now();
   if (opt.verbose) {
      std::cout << lz::GREEN_COLOR << "2." << verbose_index++ << ". Saving results in: " << opt.output << std::endl
                << lz::END_COLOR;
      init_time = now();
   }
   save_data(flags, lz, opt);

   if (opt.verbose) {
      std::cout << print_msg(MSG::INFO, "Total time elapsed: " + std::to_string(duration(g_end - g_now)) + " s")
                << std::endl;
   }

   lz::utils::DisabledMT();

   return EXIT_SUCCESS;
};

auto main(int argc, char const* argv[]) -> int {
   auto options = generateOptions();

   try {
      auto result = options.parse(argc, argv);

      if (result["version"].count() || result["v"].count()) {
         std::cout << print_msg(MSG::INFO, "Version of lzdistance: v") << VERSION << std::endl;
         return EXIT_SUCCESS;
      }

      if (result["h"].count() || result["help"].count() ||
          (result.arguments().empty() && result.unmatched().size() == 0)) {
         std::cout << lz::GREEN_COLOR << options.help() << lz::END_COLOR;
         return EXIT_SUCCESS;
      }

      auto opt = process_args(result);

      if (opt.first_input.empty() && opt.first_input_dir.empty()) {
         std::cerr << print_msg(MSG::ERROR, "Input data source is missing") << std::endl;
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
      std::cerr << std::endl << print_msg(MSG::ERROR, "BOOM... " + msg) << std::endl;
      return EXIT_FAILURE;
   } catch (...) {
      std::cerr << std::endl << print_msg(MSG::ERROR, "Fatal error BOOM!!!") << std::endl;
      return EXIT_FAILURE;
   }
}
