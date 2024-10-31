#include <csv.h>

#include "../../utils/src/lz_tbb_arena.h"
#ifdef __cpp_lib_format
#include <format>
#endif
#include <lz/parallel_utils.h>
#include <lz/pnm.h>

typedef std::chrono::high_resolution_clock::time_point time_point_t;
constexpr inline auto                                  now      = std::chrono::high_resolution_clock::now;
constexpr inline auto                                  duration = [](const std::chrono::nanoseconds& d) {
   return std::chrono::duration_cast<std::chrono::duration<double>>(d).count();
};

constexpr std::initializer_list<std::string> avoid_extensions = {".json", ".log"};

namespace lz {
   namespace utils {
      namespace fs = std::filesystem;

      struct FileInfo {
         double      size;
         std::string name;
         fs::path    path;

         bool operator==(const FileInfo& other) const { return name == other.name && size == other.size; }
         bool operator!=(const FileInfo& other) const { return !operator==(other); }
      };

      class DirectoryReader {
     private:
         std::string           dirPath;
         std::vector<FileInfo> files;

     public:
         DirectoryReader(const std::string& path)
           : dirPath(path) {
            if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
               throw FileNameError("Directory does not exist");
            }

            for (const auto& entry: fs::directory_iterator(dirPath)) {
               if (entry.is_regular_file()) {
                  FileInfo fileInfo;
                  fileInfo.name = entry.path().filename().string();
                  fileInfo.path = entry.path();
                  fileInfo.size = fs::file_size(entry.path());
                  files.push_back(fileInfo);
               }
            }
         }
         DirectoryReader(const fs::path& path)
           : DirectoryReader(path.string()) {}
         DirectoryReader(const DirectoryReader& other)
           : dirPath(other.dirPath), files(other.files){};
         DirectoryReader(DirectoryReader&& other)
           : dirPath(std::move(other.dirPath)), files(std::move(other.files)){};

         auto setDirectory(std::string path) -> void {
            dirPath = path;

            for (const auto& entry: fs::directory_iterator(dirPath)) {
               if (entry.is_regular_file()) {
                  FileInfo fileInfo;
                  fileInfo.name = entry.path().filename().string();
                  fileInfo.size = entry.file_size();
                  files.push_back(fileInfo);
               }
            }
         }

         auto getDirectory() const -> std::string { return dirPath; }

         auto getFiles() const -> std::vector<FileInfo> { return files; }

         bool isEmpty() const { return files.empty(); }

         DirectoryReader& operator=(const DirectoryReader& other) {
            if (this != &other) {
               this->~DirectoryReader();
               new (this) DirectoryReader(other);
            }
            return *this;
         };
         DirectoryReader& operator=(DirectoryReader&& other) {
            if (this != &other) {
               this->~DirectoryReader();
               new (this) DirectoryReader(std::move(other));
            }
            return *this;
         };

         bool operator==(const DirectoryReader& other) { return dirPath == other.dirPath && files == other.files; }
         bool operator!=(const DirectoryReader& other) { return !operator==(other); }

         void displayFiles() const {
            std::cout << "Number of files: " << files.size() << std::endl;
            for (const auto& file: files) {
               std::cout << file.name << " size: " << file.size << std::endl;
            }
         }
      };
   }  // namespace utils
}  // namespace lz

inline auto getColor(lz::utils::MSG_TYPE type) {
   switch (type) {
      case lz::utils::MSG_TYPE::ERROR: return lz::RED_COLOR;
      case lz::utils::MSG_TYPE::WARRING: return lz::YELLOW_COLOR;
      case lz::utils::MSG_TYPE::INFO: return lz::GREEN_COLOR;
   }
   return lz::BLUE_COLOR;
}

inline std::vector<std::string> split(const std::string& s, char delim) {
   std::vector<std::string> tokens;
   std::string              token;
   std::istringstream       tokenStream(s);
   while (std::getline(tokenStream, token, delim)) {
      tokens.push_back(token);
   }
   return tokens;
}

inline std::string print_msg(lz::utils::MSG_TYPE type, std::string msg) {
   auto                     color = getColor(type);
   std::vector<std::string> allLines;
   std::va_list             args;
   std::string::size_type   maxLen = 0;

   std::string final_msg = "";
   std::string delimiter = "\n";
   size_t      pos       = 0;

   allLines = split(msg, '\n');
   for (auto tmp: allLines) {
      maxLen = tmp.length() > maxLen ? tmp.length() : maxLen;
   }

   std::string header = type == lz::utils::MSG_TYPE::ERROR  ? " [ Error ] "
                        : type == lz::utils::MSG_TYPE::INFO ? " [ Info ] "
                                                            : " [ Warning ] ";

   for (auto str: allLines) {
      auto offset = final_msg.size() > 0 ? std::string(header.size(), ' ') : "";
      final_msg += offset + str + "\n";
   }

   return color + header + lz::END_COLOR + final_msg;
}

inline void read_multi_line(std::ifstream& in, std::vector<lz::sequence>& seq_vec, MagickNumber format) {
   lz::utils::pnm parser;

   try {
      switch (format) {
         case PNM_P1: parser.ReadPBM(in, seq_vec, false); break;
         case PNM_P4: parser.ReadPBM(in, seq_vec, true); break;
         case PNM_P2: parser.ReadPGM(in, seq_vec, false); break;
         case PNM_P5: parser.ReadPGM(in, seq_vec, true); break;
         case PNM_RAWTXT: parser.ReadRAW(in, seq_vec, false); break;
         case PNM_RAWBIN: parser.ReadRAW(in, seq_vec, true); break;
         default: parser.ReadPNM(in, seq_vec);
      }
   } catch (BadAlloc& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (lz::utils::PNMBadFileFormat& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (lz::utils::PNMUnknownError& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (Errors& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   }
}

inline void read_one_line(std::ifstream& in, lz::sequence& seq, MagickNumber format) {
   lz::utils::pnm parser;

   try {
      switch (format) {
         case PNM_P1: parser.ReadPBM(in, seq, false); break;
         case PNM_P4: parser.ReadPBM(in, seq, true); break;
         case PNM_P2: parser.ReadPGM(in, seq, false); break;
         case PNM_P5: parser.ReadPGM(in, seq, true); break;
         case PNM_RAWTXT: parser.ReadRAW(in, seq, false); break;
         case PNM_RAWBIN: parser.ReadRAW(in, seq, true); break;
         default: parser.ReadPNM(in, seq);
      }
   } catch (BadAlloc& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (lz::utils::PNMBadFileFormat& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (lz::utils::PNMUnknownError& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   } catch (Errors& err) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Bad allow while reading file ...\n" + std::string(err.msg))
                << std::endl;
   }
}

// Read a csv file with multiple columns (date per column)
inline void read_csv(const std::string& ip_path, std::vector<lz::sequence>& text_col) {
   namespace fs = std::filesystem;
   std::error_code ec;
   const auto      file_size = fs::file_size(ip_path, ec);
   int             num_line  = 0;

   if (ec) {
      std::cerr << lz::RED_COLOR << ip_path << " : " << ec.message() << "\n" << lz::END_COLOR;
      std::exit(EXIT_FAILURE);
   }

   io::LineReader input(ip_path);

   auto        line = input.next_line();
   std::string str(line);
   auto        rows = split(str, ',');
   text_col.reserve(rows.size());

   for (auto row: rows) {
      text_col.push_back(row);
   }

   // std::vector<std::vector<std::string>> data_frame(rows.size());

   while (auto line = input.next_line()) {
      std::string str(line);
      auto        rows = split(str, ',');
      for (size_t i = 0; i < rows.size(); i++) {
         text_col[i] += rows[i];
      }
   }
   // std::cout.setf(std::ios::left, std::ios::adjustfield);
   // std::cout << text_col[0];
}

// Read a plain text file with one line
inline std::vector<lz::sequence>
   read_input(const std::string& ip_path, bool multiline = false, MagickNumber format = MagickNumber::PNM_RAWTXT) {
   namespace fs = std::filesystem;
   std::error_code ec;
   fs::is_regular_file(ip_path, ec);
   fs::is_character_file(ip_path, ec);

   if (ec) {
      // std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, ip_path + " : " + ec.message()) << lz::END_COLOR;
      throw FileFormatError(ip_path + " : " + ec.message());
   }

   int           num_line = 0;
   std::ifstream input(ip_path);

   lz::utils::pnm            parser;
   lz::sequence              oneLine;
   std::vector<lz::sequence> data{};

   if (format == CSV) {
      read_csv(ip_path, data);
   } else if (multiline)
      read_multi_line(input, data, format);
   else {
      read_one_line(input, oneLine, format);
      data.push_back(oneLine);
   }

   input.close();
   return data;
}

inline std::vector<lz::sequence> read_dir(const std::string& ip_path, MagickNumber format = MagickNumber::PNM_RAWTXT) {
   namespace fs = std::filesystem;

   fs::path path(ip_path);

   // Check if the provided path is a directory
   if (!fs::is_directory(path)) {
      std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, "Provided path is not a directory.") << std::endl;
      throw FileFormatError("Provided path is not a directory.");
   }

   lz::utils::DirectoryReader reader(path);

   std::cout << reader.getDirectory() << std::endl;
   reader.displayFiles();

   const auto&               files = reader.getFiles();
   std::vector<lz::sequence> data(files.size());

   lz::utils::parallel_for(0, files.size(), [&](lz::lz_size idx) {
      auto seq  = read_input(files[idx].path, false, format);
      data[idx] = seq[0];
   });

   std::cout << "Data size: " << data.size() << std::endl;
   std::cout << data[0] << std::endl;

   return data;
}
