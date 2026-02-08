#include <lz/lpf.h>

namespace lz {
   namespace utils {

      lz_int LPF(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n) {
         // if ((SA == nullptr) || (LCP == nullptr) || (n == 0)) {
         //    return -1;
         // }
         if (n <= 1) {
            if (n == 1)
               lpf[0] = 0;
            return 0;
         }

         LPFStack stack;
         lz_uint  lcp = 0;

         // SA and LCP must have allocated size n+1	!!!
         SA.push_back(0);
         LCP.push_back(0);

         stack.push(0, SA[0] + 1);

         for (lz_size i = 1; i <= n; ++i) {
            lcp = LCP[i];

            while (!stack.empty() && SA[i] + 1 < stack.TopPos()) {
               lpf[stack.TopPos() - 1] = std::max(stack.TopLen(), lcp);
               lcp                     = std::min(stack.TopLen(), lcp);
               stack.pop();
            }

            if (i < n)
               stack.push(lcp, SA[i] + 1);
         }

         return n;
      };

      lz_int LPF_opt(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n) {
         if (n <= 1) {
            if (n == 1)
               lpf[0] = 0;
            return 0;
         }

         SA.push_back(0);
         LCP.push_back(0);
         std::vector<lz_int> stack(1, 0);

         for (lz_size i = 1; i <= n; i++) {
            while (!stack.empty() && (SA[i] + 1 < SA[stack.back()] + 1 ||
                                      (SA[i] + 1 > SA[stack.back()] && LCP[i] <= LCP[stack.back()]))) {
               if (SA[i] + 1 < SA[stack.back()] + 1) {
                  lpf[SA[stack.back()]] = std::max(LCP[i], LCP[stack.back()]);
                  LCP[i]                = std::min(LCP[i], LCP[stack.back()]);
               } else {
                  lpf[SA[stack.back()]] = LCP[stack.back()];
               }
               stack.pop_back();
            }
            if (i < n)
               stack.push_back(i);
         }

         return n;
      };

      lz_int LPF_par(std::vector<lz_uint>& lpf,
                     std::vector<lz_uint>  SA,
                     std::vector<lz_uint>  LCP,
                     lz_size               init,
                     lz_size               end) {

         auto n = end - init;
         if (n <= 1) {
            if (n == 1)
               lpf[SA[init]] = 0;
            return 0;
         }

         LPFStack stack;
         lz_uint  lcp = 0;

         // SA and LCP must have allocated size n+1	!!!
         SA.push_back(0);
         SA[end] = 0;
         LCP.push_back(0);
         LCP[end] = 0;

         stack.push(0, SA[init] + 1);

         for (lz_size i = init + 1; i <= end; ++i) {
            lcp = LCP[i];

            while (!stack.empty() && SA[i] + 1 < stack.TopPos()) {
               lpf[stack.TopPos() - 1] = std::max(stack.TopLen(), lcp);
               lcp                     = std::min(stack.TopLen(), lcp);
               stack.pop();
            }

            if (i < end)
               stack.push(lcp, SA[i] + 1);
         }

         return n;
      };

      lz_int LPF_par(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n) {
         if (n <= 1) {
            if (n == 1)
               lpf[0] = 0;
            return 0;
         }

         std::vector<std::thread> ths;
         auto                     pre_idx = 0ul;

         for (auto i = 1ul; i < SA.size(); i++) {
            if (LCP[i] != 0)
               continue;

            auto fun = [&]() { LPF_par(lpf, SA, LCP, pre_idx, i); };

            ths.emplace_back(fun);
            pre_idx = i;
         }

         auto fun = [&]() { LPF_par(lpf, SA, LCP, pre_idx, SA.size()); };
         ths.emplace_back(fun);

         for (auto& th: ths)
            th.join();

         return n;
      };

   }  // namespace utils
}  // namespace lz
