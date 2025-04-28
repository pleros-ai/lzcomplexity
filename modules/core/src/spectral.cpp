#include <lz/parallel_utils.h>

#include <lz/spectral.hpp>
#include <map>
#include <numeric>

namespace lz {

   /**
    * @brief Creates a linearly spaced vector of values
    *
    * @param start The starting value of the sequence
    * @param end The ending value of the sequence
    * @param numPoints Number of points to generate
    * @return std::vector<double> Vector containing linearly spaced values
    */
   std::vector<double> linespace(double start, double end, int numPoints) {
      std::vector<double> result(numPoints);
      double              step = (end - start) / (numPoints - 1);

      for (int i = 0; i < numPoints; ++i) {
         result[i] = start + i * step;
      }

      return result;
   }

   /**
    * @brief Generates a generalized cosine window
    *
    * @param M Length of the window
    * @param a Vector of coefficients for the cosine terms
    * @return std::vector<double> The generated window function values
    */
   auto general_cosine(int M, std::vector<double> a) -> std::vector<double> {
      if (M <= 1)
         return std::vector<double>(M, 1.0);

      auto fac = linespace(-M_PI, M_PI, M);
      auto w   = std::vector<double>(M, 0.0);
      for (auto k = 0ul; k < a.size(); ++k) {
         for (int i = 0; i < M; ++i) {
            w[i] += a[k] * std::cos(k * fac[i]);
         }
      }

      return w;
   }

   /**
    * @brief Generates a generalized Hamming window
    *
    * @param M Length of the window
    * @param alpha Parameter controlling the shape of the window
    * @return std::vector<double> The generated Hamming window values
    */
   auto general_hamming(int M, double alpha) -> std::vector<double> {
      return general_cosine(M, {alpha, 1. - alpha});
   }

   /**
    * @brief Generates a standard Hamming window with alpha=0.54
    *
    * @param M Length of the window
    * @return std::vector<double> The generated Hamming window values
    */
   auto hamming(int M) -> std::vector<double> {
      return general_hamming(M, 0.54);
   }

   /**
    * @brief Generates a Hann window (also known as Hanning window) with alpha=0.5
    *
    * @param M Length of the window
    * @return std::vector<double> The generated Hann window values
    */
   auto hann(int M) -> std::vector<double> {
      return general_hamming(M, 0.5);
   }

   /**
    * @brief Computes the Fast Fourier Transform of a signal using pocketfft
    *
    * @param signal Input signal as a vector of doubles
    * @param useComplex Whether to use complex FFT (c2c) or real FFT (r2c)
    * @return std::vector<Cpx> The FFT result as a vector of complex numbers
    */
   auto get_fft(std::vector<double> signal, bool useComplex) -> std::vector<Cpx> {
      pocketfft::shape_t  shape{signal.size()};
      pocketfft::stride_t stride(shape.size());
      size_t              tmp = sizeof(Cpx);

      for (int i = shape.size() - 1; i >= 0; --i) {
         stride[i] = tmp;
         tmp *= shape[i];
      }

      pocketfft::shape_t axes;
      for (size_t i = 0; i < shape.size(); ++i)
         axes.push_back(i);

      std::vector<Cpx> in;
      std::vector<Cpx> out(signal.size(), Cpx{});

      for (auto& val: signal) {
         in.emplace_back(val, 0);
      }

      if (useComplex) {
         pocketfft::c2c(shape, stride, stride, axes, pocketfft::FORWARD, in.data(), out.data(), 1.);
         // out = fft(signal);
      } else {
         pocketfft::r2c(shape, stride, stride, axes, pocketfft::FORWARD, signal.data(), out.data(), 1.);
      }

      return out;
   }

   /**
    * @brief Recursive implementation of the Fast Fourier Transform using the Cooley-Tukey algorithm
    *
    * @param signal Input signal as a vector of complex numbers
    * @return std::vector<Cpx> The FFT result as a vector of complex numbers
    */
   std::vector<Cpx> fft_recursive(const std::vector<Cpx>& signal) {
      size_t n = signal.size();
      if (n == 1) {
         return signal;
      }

      if (n % 2 != 0) {
         // throw std::runtime_error("Input length must be a power of 2");
         n = next_power_of_two(n);
      }

      auto s = signal;
      for (auto i = 0ul; i < n - signal.size(); i++)
         s.emplace_back(0., 0.);

      std::vector<Cpx> even_signal, odd_signal;
      for (size_t i = 0; i < n; i += 2) {
         even_signal.push_back(s[i]);
         odd_signal.push_back(s[i + 1]);
      }

      auto even_fft = fft_recursive(even_signal);
      auto odd_fft  = fft_recursive(odd_signal);

      std::vector<Cpx> result(n);
      double           angle = -2.0 * M_PI / static_cast<double>(n);

      for (size_t k = 0; k < n / 2; ++k) {
         Cpx w = std::polar(1.0, angle * static_cast<double>(k));
         Cpx t = w * odd_fft[k];

         result[k]         = even_fft[k] + t;
         result[k + n / 2] = even_fft[k] - t;
      }

      return result;
   }

   /**
    * @brief Calculates the mean of each column in a matrix
    *
    * @param matrix Input matrix as a vector of vectors
    * @return std::vector<double> Vector containing the mean of each column
    */
   std::vector<double> calculateColumnMeans(const std::vector<std::vector<double>>& matrix) {
      if (matrix.empty())
         return {};  // Handle empty input

      int                 numRows = matrix.size();
      int                 numCols = matrix[0].size();
      std::vector<double> columnMeans(numCols, 0.0);

      // Sum up each column
      for (int i = 0; i < numRows; ++i) {
         for (int j = 0; j < numCols; ++j) {
            columnMeans[j] += matrix[i][j];
         }
      }

      // Divide by the number of rows to get the mean
      for (int j = 0; j < numCols; ++j) {
         columnMeans[j] /= numRows;
      }

      return columnMeans;
   }

   /**
    * @brief Map of window function names to their corresponding function pointers
    */
   std::map<std::string, std::function<std::vector<double>(int)>> windows = {{"hamming", hamming},
                                                                             {"hamm", hamming},
                                                                             {"ham", hamming},
                                                                             {"hann", hann},
                                                                             {"han", hann}};

   void apply_window(std::vector<double>& signal, const std::string& window_type, int sample_rate) {
      auto win_fun = windows.find(window_type) != windows.end() ? windows[window_type] : hann;
      auto window  = win_fun(sample_rate);
      for (size_t i = 0; i < signal.size(); ++i) {
         signal[i] *= window[i];
      }
   }

   std::vector<double> compute_power_spectral_density(const std::vector<Cpx>& fft_result, double scale) {
      std::vector<double> psd;
      for (const auto& x: fft_result) {
         auto power = std::norm(x);  // equivalent to std::conj(x) * x
         psd.push_back(power * scale);
      }
      return psd;
   }

   /**
    * @brief Processes a signal by applying windowing and FFT, then computing power spectral density
    *
    * @param signal The input signal structure containing the signal data and processing parameters
    * @param step Step size for processing when cutting the signal into segments
    * @return std::vector<double> The power spectral density of the signal
    */
   auto process_signal(Signal signal, int step) -> std::vector<double> {
      std::vector<std::vector<double>> segments;
      double                           scale = 1.0 / (signal.sample_rate * signal.sample_rate);

      auto process_segment = [&](const std::vector<double>& segment) {
         std::vector<double> processed_segment = segment;
         if (signal.useWindow) {
            apply_window(processed_segment, signal.window, signal.sample_rate);
         }
         auto fft_result = get_fft(processed_segment, signal.useComplex);
         return compute_power_spectral_density(fft_result, scale);
      };

      if (signal.cut) {
         for (size_t i = 0; i < signal.signal.size() - signal.sample_rate; i += step) {
            auto segment = std::vector<double>(signal.signal.begin() + i * signal.sample_rate,
                                               signal.signal.begin() + (i + 1) * signal.sample_rate);
            segments.push_back(process_segment(segment));
         }
         return calculateColumnMeans(segments);
      } else {
         return process_segment(signal.signal);
      }
   }

   /**
    * @brief Computes the base-2 logarithm of a value, returning 0 for input of 0
    *
    * @tparam T Type of the input value
    * @param x Input value
    * @return double The base-2 logarithm of x, or 0 if x is 0
    */
   auto log(auto x) {
      if (x == 0)
         return 0.0;
      return std::log2(x);
   }

   /**
    * @brief Calculates the spectral entropy of a signal
    *
    * @param signal The input signal structure
    * @param step Step size for processing when cutting the signal into segments
    * @return double The spectral entropy value
    */
   auto spectral_entropy(Signal& signal, int step) -> double {
      auto ss = process_signal(signal, step);

      return spectral_entropy(ss);
   }

   auto spectral_entropy(const Signal& signal, int step) -> double {
      auto ss = process_signal(signal, step);

      return spectral_entropy(ss);
   }

   /**
    * @brief Calculates the spectral entropy from a power spectral density vector
    *
    * @param signal Vector containing the power spectral density values
    * @return double The spectral entropy value
    */
   auto spectral_entropy(const std::vector<double>& signal) -> double {
      double res = 0.0;

      auto acc = std::accumulate(signal.begin(), signal.begin() + signal.size() / 2, 0.0);

      for (auto i = 0ul; i < signal.size() / 2; i++) {
         auto p = signal[i] / acc;
         res += p * log(p);
      }
      // for (auto& x: signal) {
      //    auto p = x / acc;
      //    res += p * log(p);
      // }

      return -res;
   }

   inline void swap(std::vector<double_t>& s, lz_size start1, lz_size start2, lz_size length) {
#ifdef __cpp_lib_ranges
      std::ranges::swap_ranges(
         s.begin() + start1, s.begin() + start1 + length, s.begin() + start2, s.begin() + start2 + length);
#else
      std::swap_ranges(s.begin() + start1, s.begin() + start1 + length, s.begin() + start2);
#endif
   }

   void Shuffle(std::vector<double>& s, lz_uint block_size) {
      static std::random_device rd_seed;
      static std::mt19937       random_engine(rd_seed());

      std::uniform_int_distribution<> dis(0, (s.size() - block_size - 0x01) / block_size);
      lz_uint                         op1 = s.size() + 0x03, op2 = s.size() + 0x03;

      while (op1 > s.size() - block_size - 0x01)  // this goes on until we get a valid index
         op1 = block_size * dis(random_engine);   // the index for the first block

      op2 = block_size * dis(random_engine);
      while (true && s.size() > 10) {            // this goes on until we get a valid index
         op2 = block_size * dis(random_engine);  // the index for the second block
         if ((op2 < op1 || op2 > op1 + block_size) &&
             op2 < s.size() - block_size - 0x01)  // it does not overlap with the previous block and
                                                  // is not to large the block size can not be feed
            break;
      }

      swap(s, op1, op2, block_size);
   }

   std::vector<double> Shuffle(const std::vector<double>& s, lz_uint block_size, lz_uint times) {
      auto seq(s);

      for (lz_uint i = 0; i < times; i++)
         Shuffle(seq, block_size);

      return seq;
   }

   std::pair<std::vector<double>, lz_size> ShuffleSpectralSignal(const Signal& signal, lz_int block_size = 0) {
      lz_int mm = block_size;
      if (mm <= 0) {
         mm = utils::max_block_size(signal.signal.size());  // the maximum number for the sum in the entropy estimation
         mm += signal.signal.size() > 50 ? 10 : 0;          // begin aggressive
      }

      std::vector<double> res(mm + 3);  // Reserve the number of blocks for shuffling + 3

      auto fun = [&res, &signal](lz_size idx) {
         auto rand_seq =
            Shuffle(signal.signal, idx, signal.signal.size() / 2);  // Shuffling is made for half the
                                                                    // size of the sequence, hope that is enough
         auto sig               = signal;
         sig.signal             = rand_seq;
         auto spectral_random_h = spectral_entropy(sig);
         res[idx]               = spectral_random_h;
      };

      utils::parallel_for(1, mm + 1, fun);

      return {res, mm};
   }

   auto effective_spectral_complexity(const Signal& signal, lz_int block_size, lz_int step) -> double {
      std::pair<std::vector<lz_double>, lz_size> random_spectral_entropy;
      lz_int                                     spectral_entropy_h;

      auto factor_fun = [&]() { spectral_entropy_h = spectral_entropy(signal, step); };
      auto rand_fun   = [&]() { random_spectral_entropy = ShuffleSpectralSignal(signal, block_size); };

      utils::par_do(factor_fun, rand_fun);

      // auto [H_rand, mm] = random_spectral_entropy;
      auto H_rand = random_spectral_entropy.first;

      auto eff_spectral_cpx = 0.0;

      std::for_each(
         H_rand.begin(), H_rand.end(), [&](auto&& idx) { eff_spectral_cpx += H_rand[idx] - spectral_entropy_h; });

      return eff_spectral_cpx;
   }
}  // namespace lz