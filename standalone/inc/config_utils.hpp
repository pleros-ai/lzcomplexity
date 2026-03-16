#pragma once
/**
 * @file utils.hpp
 * @brief Common utility functions for standalone applications
 */

#include <csv.h>
#include <lz/exceptions.h>
#include <lz/general.h>
#include <lz/pnm.h>
#include <lz/sequence.h>
#include <lz/utils.h>
#include <zlib.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "kseq.hpp"

namespace lz {

  const lz_str RED_COLOR = "\033[1;31m";
  const lz_str GREEN_COLOR = "\033[1;32m";
  const lz_str YELLOW_COLOR = "\033[1;33m";
  const lz_str BLUE_COLOR = "\033[1;34m";
  const lz_str MAGENTA_COLOR = "\033[1;35m";
  const lz_str END_COLOR = "\033[0m";
  namespace standalone {

    namespace fs = std::filesystem;

    // ============================================================================
    // Time utilities
    // ============================================================================

    using time_point_t = std::chrono::high_resolution_clock::time_point;

    [[nodiscard]] inline time_point_t now() noexcept { return std::chrono::high_resolution_clock::now(); }

    [[nodiscard]] inline double duration_sec(const std::chrono::nanoseconds& d) noexcept {
      return std::chrono::duration<double>(d).count();
    }

    // ============================================================================
    // String utilities
    // ============================================================================

    /**
     * @brief Get ANSI color code for message type
     */
    [[nodiscard]] inline const lz::lz_str& getColor(lz::utils::MSG_TYPE type) noexcept {
      switch (type) {
        case lz::utils::MSG_TYPE::ERROR: return RED_COLOR;
        case lz::utils::MSG_TYPE::WARRING: return YELLOW_COLOR;
        case lz::utils::MSG_TYPE::INFO: return GREEN_COLOR;
        default: return BLUE_COLOR;
      }
    }

    /**
     * @brief Get header string for message type
     */
    [[nodiscard]] constexpr std::string_view getHeader(lz::utils::MSG_TYPE type) noexcept {
      switch (type) {
        case lz::utils::MSG_TYPE::ERROR: return " [ Error ] ";
        case lz::utils::MSG_TYPE::INFO: return " [ Info ] ";
        case lz::utils::MSG_TYPE::WARRING: return " [ Warning ] ";
        default: return " [ Debug ] ";
      }
    }

    /**
     * @name split
     *
     * @brief Split string by delimiter - optimized version
     *
     * @param s Input string
     * @param delim Delimiter character
     * @param skip_empty Skip empty tokens (default: true)
     *
     * @return Vector of string tokens
     */
    [[nodiscard]] inline std::vector<std::string> split(std::string_view s,
                                                        char             delim,
                                                        bool             skip_empty = true) {
      std::vector<std::string> tokens;
      tokens.reserve(16);  // Pre-allocate for typical use case

      size_t start = 0;
      size_t end = s.find(delim);

      while (end != std::string_view::npos) {
        if (!skip_empty || end > start) {
          tokens.emplace_back(s.substr(start, end - start));
        }
        start = end + 1;
        end = s.find(delim, start);
      }

      // Add last token
      if (!skip_empty || start < s.size()) {
        tokens.emplace_back(s.substr(start));
      }

      return tokens;
    }

    /**
     * @brief Split string by whitespace (space, tab, newline)
     */
    [[nodiscard]] inline std::vector<std::string> split_whitespace(std::string_view s) {
      std::vector<std::string> tokens;
      tokens.reserve(16);

      size_t       i = 0;
      const size_t len = s.size();

      while (i < len) {
        // Skip whitespace
        while (i < len && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        if (i >= len) break;

        // Find end of token
        size_t start = i;
        while (i < len && !std::isspace(static_cast<unsigned char>(s[i]))) ++i;

        tokens.emplace_back(s.substr(start, i - start));
      }

      return tokens;
    }

    /**
     * @brief Format a message with color and header - optimized version
     *
     * @param type    message type (ERROR, WARN or INFO)
     * @param msg     the string with the message
     *
     * @return The message formatted according to the type
     */
    [[nodiscard]] inline std::string print_msg(lz::utils::MSG_TYPE type, std::string_view msg) {
      const auto color = getColor(type);
      const auto header = getHeader(type);

      // Count newlines to estimate size
      size_t newline_count = 0;
      for (char c: msg) {
        if (c == '\n') ++newline_count;
      }

      std::string result;
      result.reserve(msg.size() + (newline_count + 1) * header.size() + 32);

      result += color;
      result += header;
      result += END_COLOR;

      size_t line_start = 0;
      size_t pos = 0;
      bool   first_line = true;

      while (pos <= msg.size()) {
        if (pos == msg.size() || msg[pos] == '\n') {
          if (!first_line) {
            result.append(header.size(), ' ');
          }
          result.append(msg.data() + line_start, pos - line_start);
          if (pos < msg.size()) {
            result += '\n';
          }
          line_start = pos + 1;
          first_line = false;
        }
        ++pos;
      }

      return result;
    }

    // ============================================================================
    // File validation utilities
    // ============================================================================

    /**
     * @brief Check if file exists and is readable
     *
     * @param path    The file path to validate
     *
     * @throws FileFormatError if file doesn't exist or can't be read
     */
    inline void validate_file(const fs::path& path) {
      std::error_code ec;
      if (!fs::exists(path, ec) || ec) {
        throw FileFormatError(path.string() + " : file does not exist");
      }
      if (!fs::is_regular_file(path, ec) || ec) {
        throw FileFormatError(path.string() + " : not a regular file");
      }
    }

    /**
     * @brief Check if directory exists
     *
     * @param path    The directory path to validate
     *
     * @throws FileNameError if directory doesn't exist
     */
    inline void validate_directory(const fs::path& path) {
      std::error_code ec;
      if (!fs::exists(path, ec) || !fs::is_directory(path, ec)) {
        throw FileNameError("Directory does not exist: " + path.string());
      }
    }

    // ============================================================================
    // File reading utilities
    // ============================================================================
    /**
     * @brief Read CSV file with multiple columns
     * @param path File path
     * @param data Output vector of sequences (one per column)
     * @param multiline If true, read all columns; if false, read only first column
     * @param delimiter Column delimiter character
     */
    inline void read_csv(const fs::path&            path,
                         std::vector<lz::sequence>& data,
                         bool                       multiline = true,
                         char                       delimiter = ',') {
      validate_file(path);

      io::LineReader input(path.string());

      // Read header/first line
      auto* line = input.next_line();
      if (!line) return;

      auto         rows = split(line, delimiter, false);
      const size_t num_cols = multiline ? rows.size() : std::min<size_t>(1, rows.size());

      data.clear();
      data.reserve(num_cols);

      for (size_t i = 0; i < num_cols; ++i) {
        data.emplace_back(std::string{rows[i]});
      }

      // Read remaining lines
      while ((line = input.next_line()) != nullptr) {
        const auto   cols = split(line, delimiter, false);
        const size_t n = std::min(num_cols, cols.size());
        for (size_t i = 0; i < n; ++i) {
          data[i] += cols[i];
        }
      }

      // Set alphabet sizes
      for (auto& seq: data) {
        seq.setAlphabetSize();
      }
    }

    /**
     * @brief Read CSV file into column name/values pairs
     */
    [[nodiscard]] inline std::vector<std::pair<std::string, std::vector<int>>> read_csv_with_header(
      const fs::path& path) {
      std::vector<std::pair<std::string, std::vector<int>>> result;

      std::ifstream file(path);
      if (!file) {
        throw std::runtime_error("Could not open file: " + path.string());
      }

      std::string line;

      // Read column names
      if (std::getline(file, line)) {
        auto cols = split(line, ',');
        result.reserve(cols.size());
        for (auto& col: cols) {
          result.emplace_back(std::move(col), std::vector<int>{});
        }
      }

      // Read data rows
      while (std::getline(file, line)) {
        std::istringstream ss(line);
        int                val;
        size_t             col_idx = 0;

        while (ss >> val && col_idx < result.size()) {
          result[col_idx].second.push_back(val);
          if (ss.peek() == ',') ss.ignore();
          ++col_idx;
        }
      }

      return result;
    }

    /**
     * @brief Read DNA sequences from .fna files (process FASTA format)
     */
    inline void read_dna(const fs::path& path, std::vector<lz::sequence>& data, bool multiline = false) {
      validate_file(path);

      gzFile fp = gzopen(path.string().c_str(), "r");
      if (!fp) {
        throw FileFormatError("Cannot open file: " + path.string());
      }

      auto reader
        = kseq::kseq_open(fp, [](auto& f, char* buf, std::size_t size) { return gzread(f, buf, size); });

      data.clear();

      while (reader->read() >= 0) {
        // std::string_view name = reader->name.view();
        // std::string_view comment = reader->comment.view();
        std::string_view seq = reader->seq.view();

        data.emplace_back(seq);

        if (!multiline) break;
      }

      gzclose(fp);
    }

    namespace detail {

      /**
       * @brief Handle PNM parsing errors uniformly
       */
      inline void handle_parse_error(const std::string& msg, const std::string& error_detail) {
        std::cerr << print_msg(lz::utils::MSG_TYPE::ERROR, msg + "\n" + error_detail) << std::endl;
      }

      /**
       * @brief Parse file based on format and multiline flag
       */
      inline void parse_by_format(const fs::path&            path,
                                  std::vector<lz::sequence>& seqs,
                                  MagickNumber               format,
                                  bool                       multiline = false) {
        lz::utils::pnm parser;

        std::ifstream in(path);
        if (!in) {
          throw FileFormatError("Cannot open file: " + path.string());
        }

        switch (format) {
          case PNM_P1:
            {
              if (multiline) {
                parser.ReadPBM(in, seqs, false);
              } else {
                parser.ReadPBM(in, seqs[0], false);
              }
              break;
            }
          case PNM_P4:
            {
              if (multiline) {
                parser.ReadPBM(in, seqs, true);
              } else {
                parser.ReadPBM(in, seqs[0], true);
              }
              break;
            }
          case PNM_P2:
            {
              if (multiline) {
                parser.ReadPGM(in, seqs, false);
              } else {
                parser.ReadPGM(in, seqs[0], false);
              }
              break;
            }
          case PNM_P5:
            {
              if (multiline) {
                parser.ReadPGM(in, seqs, true);
              } else {
                parser.ReadPGM(in, seqs[0], true);
              }
              break;
            }
          case PNM_RAWTXT:
            {
              if (multiline) {
                parser.ReadRAW(in, seqs, false);
              } else {
                parser.ReadRAW(in, seqs[0], false);
              }
              break;
            }
          case PNM_RAWBIN:
            {
              if (multiline) {
                parser.ReadRAW(in, seqs, true);
              } else {
                parser.ReadRAW(in, seqs[0], true);
              }
              break;
            }
          case CSV: read_csv(path, seqs, multiline, ','); break;
          case TSV: read_csv(path, seqs, multiline, ' '); break;
          case DNA:
          case RNA:
          case FASTA: read_dna(path, seqs, multiline); break;
          default:
            {
              if (multiline) {
                parser.ReadPNM(in, seqs);
              } else {
                parser.ReadPNM(in, seqs[0]);
              }
            }
        }
      }

    }  // namespace detail

    /**
     * @brief Read single line/sequence from file
     */
    inline void process_file(const fs::path&            path,
                             std::vector<lz::sequence>& seq,
                             MagickNumber               format,
                             bool                       multiline = false) {
      if (seq.empty()) {
        seq.emplace_back();
      }

      try {
        detail::parse_by_format(path, seq, format, multiline);
      } catch (const BadAlloc& err) {
        detail::handle_parse_error("Memory allocation failed", err.msg);
      } catch (const lz::utils::PNMBadFileFormat& err) {
        detail::handle_parse_error("PNM incorrect format", err.msg);
      } catch (const lz::utils::PNMUnknownError& err) {
        detail::handle_parse_error("Unknown PNM format", err.msg);
      } catch (const Errors& err) {
        detail::handle_parse_error("Error reading file", err.msg);
      }
    }

    /**
     * @brief Main input reading function - dispatches based on format
     */
    [[nodiscard]] inline std::vector<lz::sequence> read_input(const fs::path& path,
                                                              bool            multiline = false,
                                                              MagickNumber    format
                                                              = MagickNumber::PNM_RAWTXT) {
      validate_file(path);

      std::vector<lz::sequence> data;

      process_file(path, data, format, multiline);

      return data;
    }

    /**
     * @brief Read multi-line file and concatenate into single sequence
     */
    inline void multiLineToOneLine(const fs::path&            path,
                                   std::vector<lz::sequence>& output,
                                   bool                       concatenate = false,
                                   bool                       verbose = false) {
      validate_file(path);

      std::ifstream input(path);
      lz::sequence  final_seq;
      int           line_count = 0;

      std::string line;
      while (std::getline(input, line)) {
        ++line_count;

        if (verbose && line_count % 35 == 0) {
          std::cout << "\rLines read: " << line_count << std::flush;
        }

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == '>' || line[0] == '\n') {
          continue;
        }

        if (concatenate) {
          // Remove trailing newline if present
          if (!line.empty() && line.back() == '\n') {
            line.pop_back();
          }
          final_seq += line;
        } else {
          output.emplace_back(line);
        }
      }

      if (concatenate) {
        output.emplace_back(std::move(final_seq));
      }

      if (verbose) {
        std::cout << "\nEnd read: size --> " << (output.empty() ? 0 : output.back().size()) << "\n";
      }
    }

    // ============================================================================
    // Directory utilities
    // ============================================================================

    struct FileInfo {
      double      size;
      std::string name;
      fs::path    path;

      bool operator==(const FileInfo& other) const noexcept {
        return name == other.name && size == other.size;
      }
      bool operator!=(const FileInfo& other) const noexcept { return !(*this == other); }
    };

    /**
     * @brief Directory reader with file filtering
     */
    class DirectoryReader {
  public:
      explicit DirectoryReader(const fs::path& path, std::vector<std::string> exclude_ext = {".json", ".log"})
        : dir_path_(path.string()), exclude_extensions_(std::move(exclude_ext)) {
        validate_directory(path);
        scan_directory();
      }

      [[nodiscard]] const std::string&           directory() const noexcept { return dir_path_; }
      [[nodiscard]] const std::vector<FileInfo>& files() const noexcept { return files_; }
      [[nodiscard]] bool                         empty() const noexcept { return files_.empty(); }
      [[nodiscard]] size_t                       size() const noexcept { return files_.size(); }

      void display() const {
        std::cout << "Directory: " << dir_path_ << "\n";
        std::cout << "Files: " << files_.size() << "\n";
        for (const auto& f: files_) {
          std::cout << "  " << f.name << " (" << f.size << " bytes)\n";
        }
      }

  private:
      void scan_directory() {
        for (const auto& entry: fs::directory_iterator(dir_path_)) {
          if (!entry.is_regular_file()) continue;

          const auto ext = entry.path().extension().string();
          bool       excluded = std::any_of(exclude_extensions_.begin(),
                                            exclude_extensions_.end(),
                                            [&ext](const auto& e) { return ext == e; });

          if (!excluded) {
            files_.push_back({static_cast<double>(fs::file_size(entry.path())),
                              entry.path().filename().string(),
                              entry.path()});
          }
        }
      }

      std::string              dir_path_;
      std::vector<std::string> exclude_extensions_;
      std::vector<FileInfo>    files_;
    };

  }  // namespace standalone
}  // namespace lz
