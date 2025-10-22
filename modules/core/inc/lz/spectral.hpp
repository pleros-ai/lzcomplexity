#include <pocketfft.h>

namespace lz {

   using Cpx = std::complex<double>;

   struct Signal {
      std::vector<double> signal;
      int                 sample_rate;          // samples per seconds
      bool                useComplex = true;    // process the signal as complex
      bool                cut        = false;   // cut the signal by each second
      bool                useWindow  = false;   // use window function
      std::string         window     = "hann";  // window function
   };

   inline int next_power_of_two(int n) {
      int x = n;
      x -= 1;
      x |= x >> 1;
      x |= x >> 2;
      x |= x >> 4;
      x |= x >> 8;
      x |= x >> 16;

      if (sizeof(n) > 4) {
         // int with 64 bits
         x |= x >> 32;
      }

      return x + 1;
   }

   auto general_cosine(int M, std::vector<double> a) -> std::vector<double>;
   auto general_hamming(int M, double alpha) -> std::vector<double>;

   auto hamming(int M) -> std::vector<double>;
   auto hann(int M) -> std::vector<double>;

   auto get_fft(std::vector<double> signal, bool useComplex = false) -> std::vector<Cpx>;

   auto fft_recursive(const std::vector<Cpx>& signal) -> std::vector<Cpx>;

   inline std::vector<Cpx> fft(const std::vector<double>& signal) {
      std::vector<Cpx> complex_signal;
      for (double x: signal) {
         complex_signal.emplace_back(x, 0.0);
      }
      return fft_recursive(complex_signal);
   }

   auto process_signal(Signal signal, int step = 10) -> std::vector<double>;

   auto spectral_entropy(const std::vector<double>& signal) -> double;
   auto spectral_entropy(Signal& signal, int step = 10) -> double;
   auto spectral_entropy(const Signal& signal, int step = 10) -> double;

   // auto effective_spectral_complexity(const std::vector<double>& signal) -> double;
   auto effective_spectral_complexity(const Signal& signal,
                                      int           block_size     = 0,
                                      int           step           = 10,
                                      bool          change_shuffle = false) -> double;
}  // namespace lz