#include <csv.h>
#include <lz/lzApp.h>

#ifdef __cpp_lib_format
#include <format>
#endif
#include <limits>

#include "config.h"

typedef std::chrono::high_resolution_clock::time_point time_point_t;
constexpr inline auto now = std::chrono::high_resolution_clock::now;
constexpr inline auto duration = [](const std::chrono::nanoseconds& d) {
   return std::chrono::duration_cast<std::chrono::duration<double>>(d).count();
};

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
   for (auto tmp: allLines) {
      maxLen = tmp.length() > maxLen ? tmp.length() : maxLen;
   }

   auto header = topL + std::string(maxLen - 1, ' ') + topR + "\n";
   auto bottom = botL + std::string(maxLen - 1, ' ') + botR + "\n";

   for (auto str: allLines) {
      final_msg += "│ " + str + std::string(maxLen - str.length() + 1, ' ') + " │\n";
   }

   return color + header + final_msg + bottom + lz::END_COLOR;
}

inline void read_multi_line(std::ifstream& in, std::vector<lz::sequence>& seq_vec, MagickNumber format) {
   lz::utils::pnm parser;

   try {
      switch (format) {
         case AUTO:
            parser.ReadPNM(in, seq_vec);
            break;
         case PNM_P1:
            parser.ReadPBM(in, seq_vec, false);
            break;
         case PNM_P4:
            parser.ReadPBM(in, seq_vec, true);
            break;
         case PNM_P2:
            parser.ReadPGM(in, seq_vec, false);
            break;
         case PNM_P5:
            parser.ReadPGM(in, seq_vec, true);
            break;
         case PNM_RAWTXT:
            parser.ReadRAW(in, seq_vec, false);
            break;
         case PNM_RAWBIN:
            parser.ReadRAW(in, seq_vec, true);
            break;
         default:
            parser.ReadPNM(in, seq_vec);
      }
   } catch (BadAlloc& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (lz::utils::PNMBadFileFormat& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (lz::utils::PNMUnknownError& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (Errors& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   }
}

inline void read_one_line(std::ifstream& in, lz::sequence& seq, MagickNumber format) {
   lz::utils::pnm parser;

   try {
      switch (format) {
         case AUTO:
            parser.ReadPNM(in, seq);
            break;
         case PNM_P1:
            parser.ReadPBM(in, seq, false);
            break;
         case PNM_P4:
            parser.ReadPBM(in, seq, true);
            break;
         case PNM_P2:
            parser.ReadPGM(in, seq, false);
            break;
         case PNM_P5:
            parser.ReadPGM(in, seq, true);
            break;
         case PNM_RAWTXT:
            parser.ReadRAW(in, seq, false);
            break;
         case PNM_RAWBIN:
            parser.ReadRAW(in, seq, true);
            break;
         default:
            parser.ReadPNM(in, seq);
      }
   } catch (BadAlloc& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (lz::utils::PNMBadFileFormat& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (lz::utils::PNMUnknownError& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   } catch (Errors& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + err.msg) << std::endl;
   }
}
// Read a plain text file with one line
inline std::vector<lz::sequence> read_input(const std::string& ip_path, bool multiline = false,
                                            MagickNumber format = MagickNumber::PNM_RAWTXT) {
   namespace fs = std::filesystem;
   std::error_code ec;
   const auto file_size = fs::file_size(ip_path, ec);

   if (ec) {
      std::cerr << lz::RED_COLOR << ip_path << " : " << ec.message() << "\n" << lz::END_COLOR;
      std::exit(EXIT_FAILURE);
   }

   int num_line = 0;
   std::ifstream input(ip_path);

   lz::utils::pnm parser;
   lz::sequence oneLine;
   std::vector<lz::sequence> data{};

   if (multiline)
      read_multi_line(input, data, format);
   else {
      read_one_line(input, oneLine, format);
      data.push_back(oneLine);
   }

   input.close();
   return data;
}

// Read a plain text file with multiple line
inline void multiLineToOneLine(const std::string& ip_path, std::vector<lz::sequence>& text_col, bool process = false) {
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

      if (line.at(0) == '#' || line.at(0) == '>' || line.at(0) == '\n' || line.length() == 0) continue;

      if (process) {
         final_str += line.Take(line.size() - 1);
      } else {
         text_col.push_back(line);
      }
   }

   if (process) text_col.push_back(final_str);
   input.close();
   std::cout << "End read: size --> " << final_str.size() << "\n";
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

   for (auto row: rows) {
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