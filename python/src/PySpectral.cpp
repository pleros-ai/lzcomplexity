#include <nanobind/stl/function.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <lz/spectral.hpp>

namespace py = nanobind;

auto py_spectral_entropy = [](std::vector<double> signal,
                              int                 sample_frequency,
                              bool                use_complex,
                              bool                cut,
                              int                 step,
                              bool                use_window,
                              std::string         win) {
   lz::Signal sig;
   sig.signal      = signal;
   sig.sample_rate = sample_frequency;
   sig.useComplex  = use_complex;
   sig.useWindow   = use_window;
   sig.cut         = cut;
   sig.window      = win;

   return lz::spectral_entropy(sig, step);
};

auto py_process_signal = [](std::vector<double> signal,
                            int                 sample_rate,
                            bool                use_complex,
                            bool                cut,
                            int                 step,
                            bool                use_window,
                            std::string         win) {
   lz::Signal sig;
   sig.signal      = signal;
   sig.sample_rate = sample_rate;
   sig.useComplex  = use_complex;
   sig.useWindow   = use_window;
   sig.cut         = cut;
   sig.window      = win;

   return lz::process_signal(sig, step);
};

auto py_effective_spectral_complexity = [](std::vector<double> signal,
                                           int                 sample_rate,
                                           int                 block_size,
                                           bool                use_complex,
                                           bool                cut,
                                           bool                change_shuffle,
                                           int                 step,
                                           bool                use_window,
                                           std::string         win) {
   lz::Signal sig;
   sig.signal      = signal;
   sig.sample_rate = sample_rate;
   sig.useComplex  = use_complex;
   sig.useWindow   = use_window;
   sig.cut         = cut;
   sig.window      = win;

   return lz::effective_spectral_complexity(sig, block_size, step, change_shuffle);
};

void PySpectral(py::module_& m) {
   using namespace nanobind::literals;

   m.def("PowerSpectralDensity",
         py_process_signal,
         "signal"_a,
         "sample_frequency"_a,
         "use_complex"_a  = true,
         "cut"_a          = false,
         "step"_a         = 10,
         "apply_window"_a = false,
         "win"_a          = "hann",
         R"pbdoc(
    Computes the Power Spectral Density (PSD) of a signal using Fast Fourier Transform (FFT).
    
    The PSD represents how the power of a signal is distributed across different frequencies.
    
    Parameters:
    -----------
    signal : array_like
        The input time-domain signal as a 1D array of floating-point values.
    sample_frequency : int
        The sampling rate of the signal in Hz (samples per second).
        This determines the frequency resolution of the output.
    use_complex : bool, optional (default=True)
        Whether to use complex FFT (c2c) or real FFT (r2c).
        - True: Uses complex-to-complex FFT transformation
        - False: Uses real-to-complex FFT transformation
    cut : bool, optional (default=False)
        Whether to segment the signal into blocks of size `sample_frequency`.
        - True: Divides the signal into segments of length `sample_frequency` and 
                computes the average PSD across all segments
        - False: Processes the entire signal as a single segment
    step : int, optional (default=10)
        Step size for processing when cutting the signal into segments.
        Only relevant when `cut=True`. Smaller values provide more overlap between segments.
    apply_window : bool, optional (default=False)
        Whether to apply a window function to the signal before computing the FFT.
        Windowing helps reduce spectral leakage.
    win : str, optional (default="hann")
        The window function to apply if `apply_window=True`.
        Supported values: "hann", "han", "hamming", "hamm", "ham"
    
    Returns:
    --------
    array_like
        The power spectral density of the signal. The length of the output is equal to 
        the length of the input signal. Only the first half contains meaningful frequency 
        information due to symmetry properties of the FFT for real signals.
    
    Notes:
    ------
    - The PSD is normalized by the square of the sample rate
    - When `cut=True`, the function computes the average PSD across all segments
    - The frequency resolution is determined by sample_frequency/signal_length
    )pbdoc");
   m.def("SpectralEntropy",
         py_spectral_entropy,
         "signal"_a,
         "sample_frequency"_a,
         "use_complex"_a  = true,
         "cut"_a          = false,
         "step"_a         = 10,
         "apply_window"_a = false,
         "win"_a          = "hann",
         R"pbdoc(
      Calculates the spectral entropy of a signal.
    
    Spectral entropy quantifies the complexity or irregularity of a signal in the frequency domain.
    A signal with power concentrated in a narrow frequency band will have low spectral entropy,
    while a signal with power distributed across many frequencies will have high spectral entropy.
    
    Parameters:
    -----------
    signal : array_like
        The input time-domain signal as a 1D array of floating-point values.
    sample_frequency : int
        The sampling rate of the signal in Hz (samples per second).
        This determines the frequency resolution of the output.
    use_complex : bool, optional (default=True)
        Whether to use complex FFT (c2c) or real FFT (r2c).
        - True: Uses complex-to-complex FFT transformation
        - False: Uses real-to-complex FFT transformation
    cut : bool, optional (default=False)
        Whether to segment the signal into blocks of size `sample_frequency`.
        - True: Divides the signal into segments of length `sample_frequency` and 
                computes the average spectral entropy across all segments
        - False: Processes the entire signal as a single segment
    step : int, optional (default=10)
        Step size for processing when cutting the signal into segments.
        Only relevant when `cut=True`. Smaller values provide more overlap between segments.
    apply_window : bool, optional (default=False)
        Whether to apply a window function to the signal before computing the FFT.
        Windowing helps reduce spectral leakage.
    win : str, optional (default="hann")
        The window function to apply if `apply_window=True`.
        Supported values: "hann", "han", "hamming", "hamm", "ham"
    
    Returns:
    --------
    float
        The spectral entropy value. Higher values indicate more complexity or 
        irregularity in the frequency domain.
    
    Notes:
    ------
    - The spectral entropy is calculated using the Shannon entropy formula on the 
      normalized power spectral density
    - Only the first half of the spectrum is used in the calculation due to symmetry
    - Values typically range from 0 (pure sine wave) to log2(N/2) (white noise),
      where N is the length of the signal
         )pbdoc");
   m.def("EffectiveSpectralComplexity",
         py_effective_spectral_complexity,
         "signal"_a,
         "sample_frequency"_a,
         "block_size"_a     = 0,
         "use_complex"_a    = true,
         "cut"_a            = false,
         "change_shuffle"_a = false,
         "step"_a           = 10,
         "apply_window"_a   = false,
         "win"_a            = "hann"
                              R"pbdoc(
      Calculates the effective spectral complexity of a signal by comparing its spectral entropy
    to the spectral entropy of randomly shuffled versions of the same signal.
    
    This measure quantifies how much of the signal's spectral structure is non-random,
    providing a measure of the signal's inherent complexity beyond what would be expected
    from random noise with the same amplitude distribution.
    
    Parameters:
    -----------
    signal : array_like
        The input time-domain signal as a 1D array of floating-point values.
    sample_frequency : int
        The sampling rate of the signal in Hz (samples per second).
        This determines the frequency resolution of the output.
    block_size : int, optional (default=0)
        The size of blocks used for shuffling when creating randomized versions of the signal.
        If 0, a default block size is used (half the signal length).
        Larger block sizes preserve more of the original signal's structure.
    use_complex : bool, optional (default=True)
        Whether to use complex FFT (c2c) or real FFT (r2c).
        - True: Uses complex-to-complex FFT transformation
        - False: Uses real-to-complex FFT transformation
    cut : bool, optional (default=False)
        Whether to segment the signal into blocks of size `sample_frequency`.
        - True: Divides the signal into segments of length `sample_frequency`
        - False: Processes the entire signal as a single segment
    change_shuffle: bool, optional (default=False)
        Modify the order where shuffle is apply into the process
        - True: Apply the shuffle to PSD
        - False: Apply the shuffle to signal
    step : int, optional (default=10)
        Step size for processing when cutting the signal into segments.
        Only relevant when `cut=True`. Smaller values provide more overlap between segments.
    apply_window : bool, optional (default=False)
        Whether to apply a window function to the signal before computing the FFT.
        Windowing helps reduce spectral leakage.
    win : str, optional (default="hann")
        The window function to apply if `apply_window=True`.
        Supported values: "hann", "han", "hamming", "hamm", "ham"
    
    Returns:
    --------
    float
        The effective spectral complexity value. Higher positive values indicate 
        that the signal has more non-random spectral structure.
    
    Notes:
    ------
    - The function creates multiple shuffled versions of the input signal and computes
      their spectral entropy
    - The effective spectral complexity is the difference between the average spectral
      entropy of the shuffled signals and the spectral entropy of the original signal
    - Positive values indicate that the original signal has more structure (lower entropy)
      than random signals with the same amplitude distribution
    - The computation is parallelized for better performance
         )pbdoc");
}
