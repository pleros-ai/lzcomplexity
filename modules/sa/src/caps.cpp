#include <lz/caps.h>

namespace lz {
  namespace suffixarray {

    CaPS_SA::CaPS_SA(std::vector<char> T, lz_int n, lz_int subproblem_count, lz_int max_context)
      : T_(T)
      , n_(n)
      , SA_(n_, 0)
      , LCP_(n_, 0)
      , SA_w(nullptr)
      , LCP_w(nullptr)
      , p_(subproblem_count > 0 ? subproblem_count
             : n < 100          ? 1
             : n < 1e6          ? utils::num_workers()
             : n < 1e7          ? 100
                                : default_subproblem_count)
      , max_context(max_context ? max_context : n_)
      , pivot_(nullptr)
      , pivot_per_part_(p_ == 1 ? 1 : p_ - 1)
      , part_size_scan_(nullptr)
      , part_ruler_() {
      if (n_ < 0 || (n_ > 0 && p_ > n_)) {
        // std::cerr << subproblem_count << " " << n << "The environment variable `PARLAY_NUM_THREADS` needs
        // to be set. Aborting.\n";
        throw LZSuffixArrayError();
        std::exit(EXIT_FAILURE);
      }

      c = p_ == 1 ? 1 : p_ - 1;
      debug = false;
    }

    CaPS_SA::CaPS_SA(lz_int subproblem_count, lz_int max_context)
      : T_()
      , n_(0)
      , SA_()
      , LCP_()
      , SA_w(nullptr)
      , LCP_w(nullptr)
      , p_(subproblem_count)
      , max_context(max_context > 0 ? max_context : n_)
      , pivot_(nullptr)
      , pivot_per_part_(p_ == 1 ? 1 : p_ - 1)
      , part_size_scan_(nullptr)
      , part_ruler_() {
      c = p_ == 1 ? 1 : p_ - 1;
      debug = false;
    }

    CaPS_SA::CaPS_SA(utils::SA_Args args)
      : T_()
      , n_(0)
      , SA_()
      , LCP_()
      , SA_w(nullptr)
      , LCP_w(nullptr)
      , p_(args.chunks)
      , max_context(args.max_context > 0 ? args.max_context : n_)
      , pivot_(nullptr)
      , pivot_per_part_(p_ == 1 ? 1 : p_ - 1)
      , part_size_scan_(nullptr)
      , part_ruler_() {
      c = p_ == 1 ? 1 : p_ - 1;
      debug = false;
    }

    CaPS_SA::CaPS_SA(const CaPS_SA& other)
      : CaPS_SA(other.T_, other.n_, other.p_, other.max_context) {
      // std::memcpy(SA_, other.SA_, n_ * sizeof(lz_int));
      SA_ = other.SA_;
      // std::memcpy(LCP_, other.LCP_, n_ * sizeof(lz_int));
      LCP_ = other.LCP_;
    }

    CaPS_SA::CaPS_SA(CaPS_SA&& other) noexcept { *this = std::move(other); };

    CaPS_SA::~CaPS_SA() {
      // std::free(SA_);
      // std::free(LCP_);
    }

    void CaPS_SA::merge(const lz_int* X,
                        lz_int        len_x,
                        const lz_int* Y,
                        lz_int        len_y,
                        const lz_int* LCP_x,
                        const lz_int* LCP_y,
                        lz_int*       Z,
                        lz_int*       LCP_z) const {
      lz_int m = 0;  // LCP of the last compared pair.
      lz_int l_x;    // LCP(X_i, X_{i - 1}).
      lz_int i = 0;  // Index into `X`.
      lz_int j = 0;  // Index into `Y`.
      lz_int k = 0;  // Index into `Z`.

      while (i < len_x && j < len_y) {
        l_x = LCP_x[i];

        if (l_x > m) Z[k] = X[i], LCP_z[k] = l_x, m = m;
        else if (l_x < m) Z[k] = Y[j], LCP_z[k] = m, m = l_x;
        else  // Compute LCP of X_i and Y_j through linear scan.
        {
          const lz_int max_n = n_ - std::max(X[i], Y[j]);       // Length of the shorter suffix.
          const lz_int context = std::min(max_context, max_n);  // Prefix-context length for the suffixes.
#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
          const lz_int n
            = m + lcp_opt_avx(T_.data() + (X[i] + m), T_.data() + (Y[j] + m), context - m);  // LCP(X_i, Y_j)
#else
          const lz_int n
            = m + lcp_opt(T_.data() + (X[i] + m), T_.data() + (Y[j] + m), context - m);  // LCP(X_i, Y_j)
#endif

          // Whether the shorter suffix is a prefix of the longer one.
          Z[k] = (n == max_n ? std::max(X[i], Y[j]) : (T_[X[i] + n] < T_[Y[j] + n] ? X[i] : Y[j]));
          LCP_z[k] = (Z[k] == X[i] ? l_x : m);
          m = n;
        }

        if (Z[k] == X[i]) i++;
        else  // Swap X and Y (and associated data structures) when Y_j gets pulled into Z.
        {
          j++;
          std::swap(X, Y), std::swap(len_x, len_y), std::swap(LCP_x, LCP_y), std::swap(i, j);
        }

        k++;
      }

      for (; i < len_x; ++i, ++k)  // Copy rest of the data from X to Z.
        Z[k] = X[i], LCP_z[k] = LCP_x[i];

      if (j < len_y) {  // Copy rest of the data from Y to Z.
        Z[k] = Y[j], LCP_z[k] = m;
        for (j++, k++; j < len_y; ++j, ++k) Z[k] = Y[j], LCP_z[k] = LCP_y[j];
      }
    }

    void CaPS_SA::merge_sort(lz_int* const X,
                             lz_int* const Y,
                             const lz_int  n,
                             lz_int* const LCP,
                             lz_int* const W) const {
      assert(std::memcmp(X, Y, n * sizeof(lz_int)) == 0);

      if (n == 1) LCP[0] = 0;
      else {
        const lz_int m = n / 2;
        const auto   f = [&]() { merge_sort(Y, X, m, W, LCP); };
        const auto   g = [&]() { merge_sort(Y + m, X + m, n - m, W + m, LCP + m); };

        m < nested_par_grain_size ? (f(), g()) : lz::utils::par_do(f, g);
        merge(X, m, X + m, n - m, W, W + m, Y, LCP);
      }
    }

    void CaPS_SA::initialize() {
      const auto t_s = now();

      SA_w = utils::allocate<lz_int>(n_);   // Working space for the SA construction.
      LCP_w = utils::allocate<lz_int>(n_);  // Working space for the LCP construction.

      const auto sample_count = p_ * pivot_per_part_;
      pivot_ = utils::allocate<lz_int>(sample_count);

      const auto t_e = now();
      if (debug)
        std::cerr << "Initialized required data structures. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::sort_subarrays() {
      const auto t_s = now();

      const auto mem_init = [&](const lz_int i) { SA_[i] = SA_w[i] = i; };
      lz::utils::parallel_for(0, n_, mem_init);

      const auto subarr_size = n_ / p_;  // Size of each subarray to be sorted independently.
      const auto sort_subarr = [&](const lz_int i) {
        merge_sort(SA_w + i * subarr_size,
                   SA_.data() + i * subarr_size,
                   subarr_size + (i < p_ - 1 ? 0 : n_ % p_),
                   LCP_.data() + i * subarr_size,
                   LCP_w + i * subarr_size);

        if (++solved_ % 8 == 0 && debug) std::cerr << "\rSorted " << solved_ << " subarrays.";
      };

      solved_ = 0;
      lz::utils::parallel_for(0, p_, sort_subarr, 1);
      if (debug) std::cerr << "\n";

      const auto t_e = now();
      if (debug)
        std::cerr << "Sorted the subarrays independently. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::sample_pivots(const lz_int* const X, const lz_int n, const lz_int m, lz_int* const P) {
      const auto gap = n / (m + 1);  // Distance-gap between pivots.
      // std::cout << "gap value: " << gap << " and M: " << m << " and n: " << n << std::endl;
      for (lz_int i = 0; i < m; ++i) P[i] = X[(i + 1) * gap - 1];
    }

    void CaPS_SA::select_pivots() {
      const auto t_s = now();

      // lz_int pivot_per_part_ = 32 * std::log(n_);        // c \ln n
      const auto    sample_count = p_ * c;  // Total number of samples to select pivots from.
      lz_int* const pivot_w = utils::allocate<lz_int>(sample_count);  // Working space to sample pivots.
      const auto    subarr_size = n_ / p_;                            // Size of each sorted subarray.

      if (debug) std::cout << "Initalizate pivotes var: " << subarr_size << std::endl;
      for (lz_int i = 0; i < p_; ++i)
        sample_pivots(
          SA_.data() + i * subarr_size, subarr_size + (i < p_ - 1 ? 0 : n_ % p_), c, pivot_ + i * c);

      auto const temp_1 = utils::allocate<lz_int>(sample_count),
                 temp_2 = utils::allocate<lz_int>(sample_count);

      std::memcpy(pivot_w, pivot_, sample_count * sizeof(lz_int));
      merge_sort(pivot_, pivot_w, sample_count, temp_1, temp_2);

      sample_pivots(pivot_w, sample_count, p_ - 1, pivot_);

      std::free(pivot_w), std::free(temp_1), std::free(temp_2);

      const auto t_e = now();
      if (debug)
        std::cerr << "Selected the global pivots. Time taken: " << duration(t_e - t_s) << " seconds.\n";
    }

    void CaPS_SA::locate_pivots(lz_int* const P) const {
      const auto t_s = now();

      const auto subarr_size = n_ / p_;  // Size of each independent sorted subarray.

      const auto locate = [&](const lz_int i) {
        const auto X_i = SA_.data() + i * subarr_size;  // The i'th subarray.
        const auto P_i = P + i * (p_ + 1);              // Pivot locations in `X_i` are to be placed in `P_i`.
        P_i[0] = 0, P_i[p_] = subarr_size + (i < p_ - 1 ? 0 : n_ % p_);  // The two flanking pivot indices.
        for (lz_int j = 0; j < p_ - 1; ++j)  // TODO: try parallelizing this loop; observe performance diff.
          P_i[j + 1] = upper_bound(X_i, P_i[p_], &T_[pivot_[j]], n_ - pivot_[j]);
      };

      lz::utils::parallel_for(0, p_, locate, 1);

      const auto t_e = now();
      if (debug)
        std::cerr << "Located the pivots in each sorted subarray. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    lz_int CaPS_SA::upper_bound(const lz_int* const X,
                                const lz_int        n,
                                const char* const   P,
                                const lz_int        P_len) const {
      // Invariant: SA[l] < s < SA[r].

      lz_int l = -1, r = n;         // (Exclusive-) Range of the iterations in the binary search.
      lz_int c;                     // Midpoint in each iteration.
      lz_int soln = n;              // Solution of the search.
      lz_int lcp_l = 0, lcp_r = 0;  // LCP(s, SA[l]) and LCP(s, SA[r]).
      lz_int approx = 65'536;       // TODO: better tune and document.

      while (r - l > 1)  // Candidate matches exist.
      {
        c = (l + r) / 2;
        const char* const suf = &T_[X[c]];      // The suffix at the middle.
        const auto        suf_len = n_ - X[c];  // Length of the suffix.

        lz_int lcp_c = std::min(lcp_l, lcp_r);    // LCP(X[c], P).
        lcp_c = std::min(lcp_c, approx);          // LCP(X[c], P).
        auto max_lcp = std::min(suf_len, P_len);  // Maximum possible LCP, i.e. length of the shorter string.
        max_lcp = std::min(max_lcp, approx);
#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
        lcp_c += lcp_opt_avx(suf + lcp_c,
                             P + lcp_c,
                             max_lcp - lcp_c);  // Skip an informed number of character comparisons.
#else
        lcp_c += lcp_opt(
          suf + lcp_c, P + lcp_c, max_lcp - lcp_c);  // Skip an informed number of character comparisons.
#endif

        if (lcp_c == max_lcp)  // One is a prefix of the other.
        {
          if (lcp_c == P_len)  // P is a prefix of the suffix.
          {
            if (P_len == suf_len)  // The query is the suffix itself, i.e. P = X[c]
              return c + 1;
            else  // P < X[c]
              r = c, lcp_r = lcp_c, soln = c;
          } else  // The suffix is a prefix of the query, so X[c] < P; technically impossible if the text
                  // terminates with $.
            l = c, lcp_l = lcp_c;
        } else                        // Neither is a prefix of the other.
          if (suf[lcp_c] < P[lcp_c])  // X[c] < P
            l = c, lcp_l = lcp_c;
          else  // P < X[c]
            r = c, lcp_r = lcp_c, soln = c;
      }

      return soln;
    }

    void CaPS_SA::partition_sub_subarrays(const lz_int* const P) {
      const auto t_s = now();

      part_size_scan_ = utils::allocate<lz_int>(p_ + 1);

      const auto collect_size =  // Collects the size of the `j`'th partition.
        [&](const lz_int j) {
          part_size_scan_[j] = 0;
          for (lz_int i = 0; i < p_; ++i)  // For subarray `i`.
          {
            const auto P_i = P + i * (p_ + 1);            // Pivot collection of subarray `i`.
            part_size_scan_[j] += (P_i[j + 1] - P_i[j]);  // Collect its `j`'th sub-subarray's size.
          }
        };

      lz::utils::parallel_for(0, p_, collect_size,
                              1);  // Collect the individual size of each partition.
      // Compute inclusive-scan (prefix sum) of the partition sizes.
      lz_int curr_sum = 0;
      for (lz_int j = 0; j < p_; ++j)  // For partition `j`.
      {
        const auto part_size = part_size_scan_[j];

        part_size_scan_[j] = curr_sum;
        curr_sum += part_size;
      }

      part_size_scan_[p_] = curr_sum;
      assert(part_size_scan_[p_] == n_);
      if (debug) std::cout << "Finish idx sum\n";

      // Collate the sorted sub-subarrays to appropriate partitions.
      part_ruler_ = std::vector<lz_int>(p_ * (p_ + 1));
      const lz_int subarr_size = n_ / p_;
      const auto   collate =  // Collates the `j`'th sub-subarray from each sorted subarray to partition `j`.
        [&](const lz_int j) {
          auto const Y_j = SA_w + part_size_scan_[j];       // Memory-base for partition `j`.
          auto const LCP_Y_j = LCP_w + part_size_scan_[j];  // Memory-base for LCPs of partition `j`.
          auto const sub_subarr_idx
            = part_ruler_.data() + j * (p_ + 1);  // Index of the sorted sub-subarrays in `Y_j`.
          lz_int curr_idx = 0;                    // Current index into `Y_j`.

          for (lz_int i = 0; i < p_; ++i)  // Subarray `i`.
          {
            const auto X_i = SA_.data() + i * subarr_size;       // `i`'th sorted subarray.
            const auto LCP_X_i = LCP_.data() + i * subarr_size;  // LCP array of `X_i`.
            const auto P_i = P + i * (p_ + 1);                   // Pivot collection of subarray `i`.

            const auto sub_subarr_size
              = P_i[j + 1] - P_i[j];  // Size of the `j`'th sub-subarray of subarray `i`.
            sub_subarr_idx[i] = curr_idx;
            std::memcpy(Y_j + sub_subarr_idx[i], X_i + P_i[j], sub_subarr_size * sizeof(lz_int));
            std::memcpy(LCP_Y_j + sub_subarr_idx[i], LCP_X_i + P_i[j], sub_subarr_size * sizeof(lz_int));
            LCP_Y_j[sub_subarr_idx[i]] = 0;
            curr_idx += sub_subarr_size;
            // if (debug) std::cout << "Collect: " << std::to_string(sub_subarr_size) + "< j: " +
            // std::to_string(j) + "< " << std::endl;
          }

          sub_subarr_idx[p_] = curr_idx;
          assert(curr_idx == part_size_scan_[j + 1] - part_size_scan_[j]);
        };

      lz::utils::parallel_for(0, p_, collate, 1);
      if (debug) std::cout << "Finish collect\n";

      const auto t_e = now();
      if (debug)
        std::cerr << "Collated the sorted sub-subarrays into partitions. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::merge_sub_subarrays() {
      const auto t_s = now();

      const auto mem_init = [&](const lz_int j) {
        const auto part_size = part_size_scan_[j + 1] - part_size_scan_[j];
        std::memcpy(SA_.data() + part_size_scan_[j], SA_w + part_size_scan_[j], part_size * sizeof(lz_int));
        std::memcpy(LCP_.data() + part_size_scan_[j], LCP_w + part_size_scan_[j], part_size * sizeof(lz_int));
      };

      lz::utils::parallel_for(0, p_, mem_init, 1);  // Fulfill `sort_partition`'s precondition.

      const auto sort_part = [&](const lz_int j) {
        const auto part_idx
          = part_size_scan_[j];                  // Index of the partition in the partitions' flat collection.
        auto const X_j = SA_w + part_idx;        // Memory-base for partition `j`.
        auto const Y_j = SA_.data() + part_idx;  // Location to sort partition `j`.
        auto const LCP_X_j = LCP_w + part_idx;   // Memory-base for the LCP-arrays of partition `j`.
        auto const LCP_Y_j = LCP_.data() + part_idx;  // LCP array of `Y_j`.
        auto const sub_subarr_idx
          = part_ruler_.data() + j * (p_ + 1);  // Indices of the sorted subarrays in `X_i`.

        sort_partition(X_j, Y_j, p_, sub_subarr_idx, LCP_X_j, LCP_Y_j);

        if (++solved_ % 8 == 0 && debug) std::cerr << "\rMerged " << solved_ << " partitions.";
      };

      solved_ = 0;
      lz::utils::parallel_for(0, p_, sort_part,
                              1);  // Merge the sorted subarrays in each partitions.
      if (debug) std::cerr << "\n";

      const auto t_e = now();
      if (debug)
        std::cerr << "Merged the sorted subarrays in each partition. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::sort_partition(lz_int* const       X,
                                 lz_int* const       Y,
                                 const lz_int        n,
                                 const lz_int* const S,
                                 lz_int* const       LCP_x,
                                 lz_int* const       LCP_y) {
      if (n == 1) return;

      const auto m = n / 2;
      const auto flat_count_l = S[m] - S[0];
      const auto flat_count_r = S[n] - S[m];

      const auto f = [&]() { sort_partition(Y, X, m, S, LCP_y, LCP_x); };
      const auto g = [&]() {
        sort_partition(
          Y + flat_count_l, X + flat_count_l, n - m, S + m, LCP_y + flat_count_l, LCP_x + flat_count_l);
      };

      (flat_count_l < nested_par_grain_size || flat_count_r < nested_par_grain_size)
        ? (f(), g())
        : lz::utils::par_do(f, g);
      merge(X, flat_count_l, X + flat_count_l, flat_count_r, LCP_x, LCP_x + flat_count_l, Y, LCP_y);
    }

    void CaPS_SA::compute_partition_boundary_lcp() {
      const auto t_s = now();

      const auto compute_boundary_lcp = [&](const lz_int j) {
        const auto part_idx = part_size_scan_[j];
#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)
        LCP_[part_idx] = lcp_opt_avx(
          &T_[SA_[part_idx - 1]], &T_[SA_[part_idx]], n_ - std::max(SA_[part_idx - 1], SA_[part_idx]));
#else
        LCP_[part_idx] = lcp_opt(
          &T_[SA_[part_idx - 1]], &T_[SA_[part_idx]], n_ - std::max(SA_[part_idx - 1], SA_[part_idx]));
#endif
      };

      lz::utils::parallel_for(1, p_, compute_boundary_lcp, 1);

      const auto t_e = now();
      if (debug)
        std::cerr << "Computed the LCPs at the partition boundaries. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::clean_up() {
      const auto t_s = now();

      if (SA_w != nullptr) {
        std::free(SA_w);
        SA_w = nullptr;
      }

      if (LCP_w != nullptr) {
        std::free(LCP_w);
        LCP_w = nullptr;
      }

      if (pivot_ != nullptr) {
        std::free(pivot_);
        pivot_ = nullptr;
      }

      if (part_size_scan_ != nullptr) {
        std::free(part_size_scan_);
        part_size_scan_ = nullptr;
      }

      // std::free(part_ruler_);

      const auto t_e = now();
      if (debug)
        std::cerr << "Released the temporary data structures. Time taken: " << duration(t_e - t_s)
                  << " seconds.\n";
    }

    void CaPS_SA::refresh() {
      clean_up();

      // if (SA_ != nullptr)
      // std::free(SA_);
      // if (LCP_ != nullptr)
      //    std::free(LCP_);

      if (max_context == 0) max_context = n_;

      // SA_  = utils::allocate<lz_int>(n_);
      SA_.resize(n_);
      // LCP_ = utils::allocate<lz_int>(n_);
      LCP_.resize(n_);
    }

    utils::LZ_SuffixArray CaPS_SA::construct(std::vector<char> T, lz_int n) {
      T_ = std::move(T);
      n_ = n;

      p_ = p_ > 0  ? p_
        : n_ < 100 ? 1
        : n_ < 1e6 ? utils::num_workers()
        : n_ < 1e7 ? 100
                   : default_subproblem_count;
      c = p_ == 1 ? 1 : p_ - 1;

      refresh();
      return construct();
    }

    utils::LZ_SuffixArray CaPS_SA::construct(const std::string& T) {
      std::for_each(T.begin(), T.end(), [&](char ch) { T_.push_back(std::move(ch)); });

      // T_ = std::vector<const char>();
      n_ = T.length();
      p_ = p_ > 0  ? p_
        : n_ < 100 ? 1
        : n_ < 1e6 ? utils::num_workers()
        : n_ < 1e7 ? 100
                   : default_subproblem_count;
      c = p_ == 1 ? 1 : p_ - 1;

      refresh();
      return construct();
    }

    utils::LZ_SuffixArray CaPS_SA::construct() {
      const auto t_start = now();

      initialize();

      // merge_sort(SA_w, SA_, n_, LCP_, LCP_w);  // Monolithic construction.
      sort_subarrays();

      select_pivots();

      lz_int* const P
        = utils::allocate<lz_int>(p_ * (p_ + 1));  // Collection of pivot locations in the subarrays.
      locate_pivots(P);

      partition_sub_subarrays(P);
      std::free(P);

      merge_sub_subarrays();

      compute_partition_boundary_lcp();

      clean_up();

      const auto t_end = now();
      if (debug)
        std::cerr << "Constructed the suffix array. Time taken: " << duration(t_end - t_start)
                  << " seconds.\n";
      return utils::LZ_SuffixArray(SA_, LCP_, n_);
    }

    void CaPS_SA::dump(std::ofstream& output) {
      const auto t_start = now();

      const std::size_t n = n_;
      output << n;
      for (auto i = 0; i < n_; i++) output << SA_[i];
      for (auto i = 0; i < n_; i++) output << LCP_[i];
      // output.write(reinterpret_cast<const char*>(&n), sizeof(std::size_t));
      // output.write(reinterpret_cast<const char*>(SA_.data()), n_ * sizeof(lz_int));
      // output.write(reinterpret_cast<const char*>(LCP_.data()), n_ * sizeof(lz_int));

      const auto t_end = now();
      if (debug)
        std::cerr << "Dumped the suffix array. Time taken: " << duration(t_end - t_start) << " seconds.\n";
    }

    void CaPS_SA::dump_plain(std::ofstream& output) {
      const auto t_start = now();

      const std::size_t n = n_;
      output << n << "\n";
      for (std::size_t idx = 0; idx < n; idx++) {
        output << SA_[idx] << " ";
      }
      output << "\n";
      for (std::size_t idx = 0; idx < n; idx++) {
        output << LCP_[idx] << " ";
      }

      const auto t_end = now();
      if (debug)
        std::cerr << "Plain dumped the suffix array. Time taken: " << duration(t_end - t_start)
                  << " seconds.\n";
    }

    bool CaPS_SA::is_sorted(const lz_int* const X, const lz_int n) const {
      for (lz_int i = 1; i < n; ++i) {
        // std::vector<const char> x = {T_.begin() + X[i - 1], T_.begin() + X[i]};
        // std::vector<const char> y = {T_.begin() + X[i], T_.begin() + X[i + 1]};
        const auto x = &T_[X[i - 1]], y = &T_[X[i]];
        const auto l = std::min(n_ - X[i - 1], n_ - X[i]);

        for (lz_int i = 0; i < l; ++i)
          if (x[i] < y[i]) break;
          else if (x[i] > y[i]) return false;
      }

      return true;
    }

    void swap(CaPS_SA& lhs, CaPS_SA& rhs) {
      std::swap(lhs.n_, rhs.n_);
      std::swap(lhs.SA_, rhs.SA_);
      std::swap(lhs.LCP_, rhs.LCP_);
      std::swap(lhs.T_, rhs.T_);
    }

  }  // namespace suffixarray

}  // namespace lz

// Template instantiations for the required instances.
// template class lz::CaPS_SA::CaPS_SA<uint32_t>;
// template class lz::suffixarray::CaPS_SA<uint64_t>;
