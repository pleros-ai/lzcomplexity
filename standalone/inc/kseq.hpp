/**
 * @file kseq.hpp
 * @brief Modern C++ implementation of kseq for parsing FASTA/FASTQ files
 *
 * Original C version: Copyright (c) 2008 Genome Research Ltd (GRL)
 * Contact: Heng Li <lh3@sanger.ac.uk>
 *
 * C++ modernization maintains the same high-performance characteristics
 * while providing type safety and RAII semantics.
 *
 * @section Features
 * - RAII memory management (no manual malloc/free)
 * - Type-safe templates instead of macros
 * - Multiline sequence support (concatenated without newlines)
 * - Iterator support for range-based for loops
 * - Cache-line aligned buffers for optimal performance
 * - Branchless character validation for speed
 *
 * @section Usage
 * @code
 * #include "kseq.hpp"
 *
 * // Using FILE* (most common)
 * auto reader = kseq::kseq_open("sequences.fasta", "r");
 * while (reader->read() >= 0) {
 *    std::string_view name = reader->name.view();
 *    std::string_view seq = reader->seq.view();
 *    // Process sequence...
 * }
 *
 * // Using range-based for loop
 * auto reader2 = kseq::kseq_open("sequences.fastq", "r");
 * for (auto& record : *reader2) {
 *    // record.name, record.seq, record.qual available
 * }
 *
 * // Using custom read function (e.g., zlib)
 * gzFile fp = gzopen("sequences.fasta.gz", "r");
 * auto reader3 = kseq::kseq_open(fp, [](gzFile f, char* buf, std::size_t len) {
 *    return gzread(f, buf, len);
 * });
 * @endcode
 */

#pragma once

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <io.h>
#define posix_read _read
#else
#include <unistd.h>
#define posix_read ::read
#endif

namespace kseq {

  // Separator constants
  constexpr int KS_SEP_SPACE = 0;  // isspace(): \t, \n, \v, \f, \r
  constexpr int KS_SEP_TAB = 1;    // isspace() && !' '
  constexpr int KS_SEP_MAX = 1;

  // Default buffer size (4KB - optimal for most I/O operations)
  constexpr std::size_t DEFAULT_BUFSIZE = 4096;

  /**
   * @brief Round up to next power of 2
   * @param x Value to round up (modified in place)
   */
  inline constexpr std::size_t kroundup32(std::size_t x) noexcept {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
  }

  /**
   * @brief Dynamic string buffer with exponential growth
   *
   * Maintains compatibility with original kstring_t while providing
   * modern C++ interface. Uses power-of-2 growth for optimal reallocation.
   */
  class kstring_t {
public:
    std::size_t l{0};        // length
    std::size_t m{0};        // capacity
    char*       s{nullptr};  // data

    kstring_t() noexcept = default;

    ~kstring_t() noexcept { std::free(s); }

    // Non-copyable for performance (avoid accidental copies)
    kstring_t(const kstring_t&) = delete;
    kstring_t& operator=(const kstring_t&) = delete;

    // Movable
    kstring_t(kstring_t&& other) noexcept
      : l(other.l), m(other.m), s(other.s) {
      other.l = other.m = 0;
      other.s = nullptr;
    }

    kstring_t& operator=(kstring_t&& other) noexcept {
      if (this != &other) {
        std::free(s);
        l = other.l;
        m = other.m;
        s = other.s;
        other.l = other.m = 0;
        other.s = nullptr;
      }
      return *this;
    }

    /**
     * @brief Clear the string (reset length, keep capacity)
     */
    void clear() noexcept { l = 0; }

    /**
     * @brief Get string view of current content
     */
    std::string_view view() const noexcept { return {s, l}; }

    /**
     * @brief Get std::string copy of current content
     */
    std::string str() const { return s ? std::string(s, l) : std::string{}; }

    /**
     * @brief Ensure capacity for at least n characters
     */
    void reserve(std::size_t n) {
      if (n > m) {
        m = kroundup32(n);
        s = static_cast<char*>(std::realloc(s, m));
      }
    }

    /**
     * @brief Append a single character
     */
    void push_back(char c) {
      if (l + 1 >= m) {
        m = kroundup32(l + 2);
        s = static_cast<char*>(std::realloc(s, m));
      }
      s[l++] = c;
    }

    /**
     * @brief Append data from buffer
     */
    void append(const char* data, std::size_t len) {
      if (m - l < len + 1) {
        m = kroundup32(l + len + 1);
        s = static_cast<char*>(std::realloc(s, m));
      }
      std::memcpy(s + l, data, len);
      l += len;
    }

    /**
     * @brief Null-terminate the string
     */
    void terminate() noexcept {
      if (l == 0 && m == 0) {
        m = 1;
        s = static_cast<char*>(std::calloc(1, 1));
      } else if (s) {
        s[l] = '\0';
      }
    }
  };

  /**
   * @brief Buffered stream reader
   *
   * Template parameter ReadFunc should be a callable with signature:
   *   ssize_t(FileType, char*, size_t)
   *
   * @tparam FileType The file handle type (e.g., int for POSIX, gzFile for zlib)
   * @tparam ReadFunc The read function type
   * @tparam BufSize Buffer size (default 4KB)
   */
  template<typename FileType, typename ReadFunc, std::size_t BufSize = DEFAULT_BUFSIZE> class kstream_t {
public:
    using file_type = FileType;
    static constexpr std::size_t buffer_size = BufSize;

private:
    alignas(64) char buf_[BufSize];  // Cache-line aligned buffer
    int      begin_{0};
    int      end_{0};
    int      is_eof_{0};
    FileType f_;
    ReadFunc read_func_;

public:
    explicit kstream_t(FileType f, ReadFunc read_func = ReadFunc{})
      : f_(f), read_func_(read_func) {}

    // Non-copyable, non-movable (owns file handle state)
    kstream_t(const kstream_t&) = delete;
    kstream_t& operator=(const kstream_t&) = delete;
    kstream_t(kstream_t&&) = delete;
    kstream_t& operator=(kstream_t&&) = delete;

    /**
     * @brief Check if at end of file
     */
    bool eof() const noexcept { return is_eof_ && begin_ >= end_; }

    /**
     * @brief Rewind stream to beginning
     */
    void rewind() noexcept { is_eof_ = begin_ = end_ = 0; }

    /**
     * @brief Get a single character from stream
     * @return Character value (0-255) or -1 on EOF
     */
    int getc() noexcept {
      if (is_eof_ && begin_ >= end_) return -1;
      if (begin_ >= end_) {
        begin_ = 0;
        end_ = static_cast<int>(read_func_(f_, buf_, BufSize));
        if (end_ < static_cast<int>(BufSize)) is_eof_ = 1;
        if (end_ == 0) return -1;
      }
      return static_cast<unsigned char>(buf_[begin_++]);
    }

    /**
     * @brief Read until delimiter
     * @param delimiter Delimiter character or KS_SEP_SPACE/KS_SEP_TAB
     * @param str Output string buffer
     * @param dret If non-null, receives the delimiter character found
     * @return Length of string read, or -1 on EOF
     */
    int getuntil(int delimiter, kstring_t& str, int* dret = nullptr) noexcept {
      if (dret) *dret = 0;
      str.clear();

      if (begin_ >= end_ && is_eof_) return -1;

      for (;;) {
        if (begin_ >= end_) {
          if (!is_eof_) {
            begin_ = 0;
            end_ = static_cast<int>(read_func_(f_, buf_, BufSize));
            if (end_ < static_cast<int>(BufSize)) is_eof_ = 1;
            if (end_ == 0) break;
          } else {
            break;
          }
        }

        int i;
        if (delimiter > KS_SEP_MAX) {
          // Single character delimiter
          for (i = begin_; i < end_; ++i) {
            if (buf_[i] == delimiter) break;
          }
        } else if (delimiter == KS_SEP_SPACE) {
          // Any whitespace
          for (i = begin_; i < end_; ++i) {
            if (std::isspace(static_cast<unsigned char>(buf_[i]))) break;
          }
        } else if (delimiter == KS_SEP_TAB) {
          // Whitespace except space
          for (i = begin_; i < end_; ++i) {
            const unsigned char c = static_cast<unsigned char>(buf_[i]);
            if (std::isspace(c) && c != ' ') break;
          }
        } else {
          i = begin_;  // Should never reach here
        }

        // Append to string
        str.append(buf_ + begin_, static_cast<std::size_t>(i - begin_));
        begin_ = i + 1;

        if (i < end_) {
          if (dret) *dret = static_cast<unsigned char>(buf_[i]);
          break;
        }
      }

      str.terminate();
      return static_cast<int>(str.l);
    }
  };

  /**
   * @brief FASTA/FASTQ sequence record
   *
   * @tparam FileType The file handle type
   * @tparam ReadFunc The read function type
   * @tparam BufSize Buffer size
   */
  template<typename FileType, typename ReadFunc, std::size_t BufSize = DEFAULT_BUFSIZE> class kseq_t {
public:
    using stream_type = kstream_t<FileType, ReadFunc, BufSize>;

    kstring_t name;          // Sequence name
    kstring_t comment;       // Comment (after first space in header)
    kstring_t seq;           // Sequence data
    kstring_t qual;          // Quality scores (FASTQ only)
    int       last_char{0};  // Last character read (for state tracking)

private:
    std::unique_ptr<stream_type> f_;

public:
    explicit kseq_t(FileType fd, ReadFunc read_func = ReadFunc{})
      : f_(std::make_unique<stream_type>(fd, read_func)) {}

    // Non-copyable
    kseq_t(const kseq_t&) = delete;
    kseq_t& operator=(const kseq_t&) = delete;

    // Movable
    kseq_t(kseq_t&&) = default;
    kseq_t& operator=(kseq_t&&) = default;

    /**
     * @brief Get underlying stream
     */
    stream_type* stream() noexcept { return f_.get(); }

    /**
     * @brief Read next sequence record
     *
     * Reads FASTA/FASTQ sequences, handling multiline sequences by
     * concatenating all lines into a single sequence without line breaks.
     *
     * @return >=0: length of sequence (success)
     *         -1: end of file
     *         -2: truncated quality string
     */
    int read() noexcept {
      int          c;
      stream_type* ks = f_.get();

      // Jump to next header line if needed
      if (last_char == 0) {
        while ((c = ks->getc()) != -1 && c != '>' && c != '@');
        if (c == -1) return -1;  // EOF
        last_char = c;
      }

      // Reset lengths (keep capacity for reuse)
      comment.clear();
      seq.clear();
      qual.clear();

      // Read name (until first whitespace)
      if (ks->getuntil(KS_SEP_SPACE, name, &c) < 0) return -1;

      // Read comment if present (rest of header line)
      if (c != '\n') {
        ks->getuntil('\n', comment, nullptr);
      }

      // Read sequence data - handles multiline FASTA
      // All lines are concatenated into single sequence without newlines
      while ((c = ks->getc()) != -1 && c != '>' && c != '+' && c != '@') {
        // Accept only printable non-space characters (ASCII 33-126)
        // This automatically skips newlines, spaces, tabs, etc.
        if (static_cast<unsigned>(c - 33) < 94u) {  // Branchless: c >= 33 && c <= 126
          seq.push_back(static_cast<char>(c));
        }
      }

      if (c == '>' || c == '@') {
        last_char = c;  // Save header char for next read
      }

      seq.terminate();

      // FASTA format - no quality scores
      if (c != '+') {
        return static_cast<int>(seq.l);
      }

      // FASTQ format - allocate quality buffer to match sequence length
      if (qual.m < seq.m) {
        qual.reserve(seq.m);
      }

      // Skip rest of '+' line (optional repeat of name)
      while ((c = ks->getc()) != -1 && c != '\n');
      if (c == -1) return -2;  // Truncated file

      // Read quality string - handles multiline quality scores
      while ((c = ks->getc()) != -1 && qual.l < seq.l) {
        // Accept valid Phred quality characters (ASCII 33-127)
        if (static_cast<unsigned>(c - 33) < 95u) {  // Branchless: c >= 33 && c <= 127
          qual.s[qual.l++] = static_cast<char>(c);
        }
      }

      qual.terminate();
      last_char = 0;  // Ready for next record

      // Validate quality length matches sequence length
      if (seq.l != qual.l) return -2;

      return static_cast<int>(seq.l);
    }

    /**
     * @brief Reset reader state for re-reading
     */
    void rewind() noexcept {
      last_char = 0;
      if (f_) f_->rewind();
    }

    /**
     * @brief Check if stream has reached end of file
     */
    bool eof() const noexcept { return f_ && f_->eof(); }

    // =========================================================================
    // Iterator support for range-based for loops
    // =========================================================================

    class iterator {
      kseq_t* seq_{nullptr};
      int     status_{0};

  public:
      using iterator_category = std::input_iterator_tag;
      using value_type = kseq_t;
      using difference_type = std::ptrdiff_t;
      using pointer = kseq_t*;
      using reference = kseq_t&;

      iterator() noexcept = default;
      explicit iterator(kseq_t* seq) noexcept
        : seq_(seq) {
        if (seq_) status_ = seq_->read();
      }

      reference operator*() noexcept { return *seq_; }
      pointer   operator->() noexcept { return seq_; }

      iterator& operator++() noexcept {
        if (seq_) status_ = seq_->read();
        return *this;
      }

      iterator operator++(int) noexcept {
        iterator tmp = *this;
        ++(*this);
        return tmp;
      }

      bool operator==(const iterator& other) const noexcept {
        // End iterator: seq_ is null or status_ < 0
        const bool this_end = !seq_ || status_ < 0;
        const bool other_end = !other.seq_ || other.status_ < 0;
        return this_end == other_end;
      }

      bool operator!=(const iterator& other) const noexcept { return !(*this == other); }

      int status() const noexcept { return status_; }
    };

    iterator begin() noexcept { return iterator(this); }
    iterator end() noexcept { return iterator(); }
  };

  /**
   * @brief Factory function to create kseq reader
   *
   * @tparam FileType File handle type
   * @tparam ReadFunc Read function type
   * @tparam BufSize Buffer size
   *
   * @param fd File handle
   * @param read_func Read function
   *
   * @return Unique pointer to kseq_t
   */
  template<typename FileType, typename ReadFunc, std::size_t BufSize = DEFAULT_BUFSIZE>
  inline auto make_kseq(FileType fd, ReadFunc read_func) {
    return std::make_unique<kseq_t<FileType, ReadFunc, BufSize>>(fd, read_func);
  }

  // ============================================================================
  // Predefined read function wrappers for common file types
  // ============================================================================

  /**
   * @brief POSIX file descriptor reader
   */
  struct PosixReader {
    auto operator()(int fd, char* buf, std::size_t len) const noexcept { return posix_read(fd, buf, len); }
  };

  /**
   * @brief FILE* stream reader
   */
  struct FileReader {
    auto operator()(std::FILE* fp, char* buf, std::size_t len) const noexcept {
      return std::fread(buf, 1, len, fp);
    }
  };

  /**
   * @brief Generic lambda/functor reader wrapper
   * @tparam Func Callable type with signature: ssize_t(FileType, char*, size_t)
   */
  template<typename Func> struct FuncReader {
    Func func;

    explicit FuncReader(Func f)
      : func(std::move(f)) {}

    template<typename FileType> auto operator()(FileType fd, char* buf, std::size_t len) const noexcept {
      return func(fd, buf, len);
    }
  };

  // ============================================================================
  // Type aliases for common use cases
  // ============================================================================

  /// Sequence reader for POSIX file descriptors
  using kseq_fd = kseq_t<int, PosixReader>;

  /// Sequence reader for FILE* streams
  using kseq_file = kseq_t<std::FILE*, FileReader>;

  // ============================================================================
  // Factory functions
  // ============================================================================

  /**
   * @brief Create sequence reader from POSIX file descriptor
   * @param fd File descriptor
   * @return Unique pointer to kseq reader
   */
  inline auto kseq_open(int fd) { return std::make_unique<kseq_fd>(fd, PosixReader{}); }

  /**
   * @brief Create sequence reader from FILE* stream
   * @param fp File pointer
   * @return Unique pointer to kseq reader
   */
  inline auto kseq_open(std::FILE* fp) { return std::make_unique<kseq_file>(fp, FileReader{}); }

  /**
   * @brief Create sequence reader with custom read function
   *
   * @tparam FileType File handle type
   * @tparam ReadFunc Read function type
   *
   * @param fd File handle
   * @param read_func Read function callable
   *
   * @return Unique pointer to kseq reader
   */
  template<typename FileType, typename ReadFunc> inline auto kseq_open(FileType fd, ReadFunc read_func) {
    return std::make_unique<kseq_t<FileType, ReadFunc>>(fd, std::move(read_func));
  }

  /**
   * @brief Open file by path and create sequence reader
   *
   * @param path File path
   * @param mode Open mode (default "r")
   *
   * @return Unique pointer to kseq reader, nullptr on failure
   */
  inline auto kseq_open(const char* path, const char* mode = "r") {
    std::FILE* fp = std::fopen(path, mode);
    if (!fp) return std::unique_ptr<kseq_file>(nullptr);
    return std::make_unique<kseq_file>(fp, FileReader{});
  }

}  // namespace kseq
