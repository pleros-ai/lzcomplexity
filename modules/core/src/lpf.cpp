#include <lz/lpf.h>
#include <tbb/blocked_range.h>

namespace lz {
   namespace internal {

      template<typename T>
      class LZ_BlockedRange;
   }  // namespace internal

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

         lz_int lcp = 0;
         SA.push_back(0);
         LCP.push_back(0);
         std::vector<lz_int> stack(1, 0);

         for (lz_size i = 1; i <= n; i++) {
            lcp = LCP[i];
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

      lz_int getDepth(lz_int i) {
         lz_int a = 0;
         lz_int b = i - 1;
         while (b > 0) {
            b = b >> 1;
            a++;
         }
         return a + 1;
      }

      lz_int cflog2(lz_int i) {
         lz_int res = 0;

         i--;
         if (i >> 16) {
            res += 16;
            i >>= 16;
         } else
            i &= 0xffff;

         for (; i; i >>= 1)
            res++;
         return res;
      }

#define LEFT(i) ((i) << 1)
#define RIGHT(i) (((i) << 1) | 1)
#define PARENT(i) ((i) >> 1)

      const int BLOCK_SIZE = 8192;

      inline lz_int getLeft_opt(std::vector<std::vector<lz_uint>> table, lz_int depth, lz_int index, lz_int start) {
         auto value = table[0][index];
         if (value == table[depth - 1][0])
            return -1;

         lz_int cur = PARENT(start), d, dist = 2;
         for (d = 1; d < depth; d++) {
            if ((cur + 1) * dist > index + 1)
               cur--;
            if (cur < 0)
               return -1;

            if (table[d][cur] >= value)
               cur = PARENT(cur);
            else
               break;

            dist <<= 1;
         }

         for (; d > 0; d--) {
            if (table[d - 1][RIGHT(cur)] < value)
               cur = RIGHT(cur);
            else
               cur = LEFT(cur);
         }
         return cur;
      }

      inline lz_int
         getRight_opt(std::vector<std::vector<lz_uint>> table, lz_int depth, lz_int n, lz_int index, lz_int start) {
         auto value = table[0][index];
         if (value == table[depth - 1][0])
            return -1;

         lz_int cur = PARENT(start), d, dist = 2;
         for (d = 1; d < depth; d++) {
            if (cur * dist < index)
               cur++;
            if (cur * dist >= n)
               return -1;

            if (table[d][cur] >= value)
               cur = PARENT(cur);
            else
               break;

            dist <<= 1;
         }

         for (; d > 0; d--) {
            if (table[d - 1][LEFT(cur)] < value)
               cur = LEFT(cur);
            else
               cur = RIGHT(cur);
         }
         return cur;
      }

      void ComputeANSV_Linear(std::vector<lz_uint>& a,
                              lz_int                n,
                              std::vector<lz_int>&  leftElements,
                              std::vector<lz_int>&  rightElements,
                              lz_int                offset) {
         lz_int  i, top;
         lz_int* stack = new lz_int[n];

         for (i = 0, top = -1; i < n; i++) {
            while (top > -1 && a[stack[top]] > a[i])
               top--;
            if (top == -1)
               leftElements[i + offset] = -1;
            else
               leftElements[i + offset] = stack[top] + offset;
            stack[++top] = i;
         }

         for (i = n - 1, top = -1; i >= 0; i--) {
            while (top > -1 && a[stack[top]] > a[i])
               top--;
            if (top == -1)
               rightElements[i + offset] = -1;
            else
               rightElements[i + offset] = stack[top] + offset;
            stack[++top] = i;
         }
         delete[] stack;
      }

      void ComputeANSV(std::vector<lz_uint>& a, lz_int n, std::vector<lz_int>& left, std::vector<lz_int>& right) {
         lz_int l2    = cflog2(n);
         lz_int depth = l2 + 1;
         // lz_int** table = new lz_int*[depth];
         std::vector<std::vector<lz_uint>> table(depth);

         table[0] = a;
         lz_int m = n;
         for (lz_int i = 1; i < depth; i++) {
            m = (m + 1) / 2;
            // table[i] = new lz_int[m];
            table[i] = std::vector<lz_uint>(m);
         }

         m = n;
         for (lz_int d = 1; d < depth; d++) {
            lz_int m2 = m / 2;

            // std::cout << m << " -- " << depth << std::endl;
            lz::utils::parallel_for(
               0, m2, [&](const lz_int i) { table[d][i] = std::min(table[d - 1][LEFT(i)], table[d - 1][RIGHT(i)]); });

            if (m % 2) {
               table[d][m2] = table[d - 1][LEFT(m2)];
            }

            m = (m + 1) / 2;
         }

         lz::internal::parallel_for_impl_2(
            0,
            n,
            [&](tbb::blocked_range<lz_size> rng) {
               lz_int offset = rng.begin();
               ComputeANSV_Linear(a, rng.size(), left, right, offset);

               lz_int tmp = offset;
               for (lz_uint k = offset; k < rng.end(); k++) {
                  if (left[k] == -1) {
                     if (tmp != -1 && a[tmp] >= a[k]) {
                        tmp = getLeft_opt(table, depth, k, tmp);
                     }
                     left[k] = tmp;
                  }
               }

               tmp = rng.end() - 1;
               for (lz_int k = rng.end() - 1; k >= offset; k--) {
                  if (right[k] == -1) {
                     if (tmp != -1 && a[tmp] >= a[k]) {
                        tmp = getRight_opt(table, depth, n, k, tmp);
                     }
                     right[k] = tmp;
                  }
               }
            },
            n / lz::utils::GetGlobalTaskArena()->TaskArenaSize());
      };

      // lz_int LPF_2(const sequence& seq, std::vector<lz_uint> sa, lz_int n, std::vector<lz_int>& lpf) {

      //    lz_int               d = getDepth(n);
      //    std::vector<lz_int>  leftElements(n), rightElements(n);
      //    std::vector<lz_int>  leftLPF(n), rightLPF(n);
      //    std::vector<lz_int>& rank = lpf;

      //    ComputeANSV(sa, n, leftElements, rightElements);

      //    lz::utils::parallel_for(0, n, [&](const lz_int i) { rank[sa[i]] = i; });

      //    int p = lz::utils::GetGlobalTaskArena()->TaskArenaSize();
      //    p *= 2;
      //    lz_int size = (n + p - 1) / p;

      //    lz::internal::parallel_for_impl_2(
      //       0,
      //       n,
      //       [&](tbb::blocked_range<lz_size> rng) {
      //          lz_int i = rng.begin();
      //          lz_int j = rng.end();

      //          // compute lpf for first element
      //          lz_int mid = rank[i], left = leftElements[rank[i]], right = rightElements[rank[i]];
      //          lz_int llcp = 0, rlcp = 0;

      //          if (left != -1) {
      //             while (sa[left] + llcp < n && i + llcp < n && seq[sa[left] + llcp] == seq[i + llcp]) {
      //                llcp++;
      //             }
      //             leftLPF[i] = llcp;
      //          } else
      //             leftLPF[i] = 0;

      //          if (right != -1) {
      //             while (sa[right] + rlcp < n && i + rlcp < n && seq[sa[right] + rlcp] == seq[i + rlcp]) {
      //                rlcp++;
      //             }
      //             rightLPF[i] = rlcp;
      //          } else
      //             rightLPF[i] = 0;

      //          if (leftLPF[i] == 0 && rightLPF[i] == 0) {
      //             lpf[i] = 0;
      //          } else if (leftLPF[i] > rightLPF[i]) {  // no neighbor
      //             lpf[i] = leftLPF[i];
      //          } else {
      //             lpf[i] = rightLPF[i];
      //          }

      //          // compute lpf for rest elements
      //          // TODO: try parallel for
      //          lz::utils::parallel_for(i + 1, j, [&](const lz_int k) {
      //             left  = leftElements[rank[k]];
      //             right = rightElements[rank[k]];

      //             if (left != -1) {
      //                llcp = std::max<lz_int>(leftLPF[k - 1] - 1, 0);
      //                while (sa[left] + llcp < n && k + llcp < n && seq[sa[left] + llcp] == seq[k + llcp]) {
      //                   llcp++;
      //                }
      //                leftLPF[k] = llcp;
      //             } else
      //                leftLPF[k] = 0;

      //             if (right != -1) {
      //                rlcp = std::max<lz_int>(rightLPF[k - 1] - 1, 0);
      //                while (sa[right] + rlcp < n && k + rlcp < n && seq[sa[right] + rlcp] == seq[k + rlcp]) {
      //                   rlcp++;
      //                }
      //                rightLPF[k] = rlcp;
      //             } else
      //                rightLPF[k] = 0;

      //             if (leftLPF[k] == 0 && rightLPF[k] == 0) {
      //                lpf[k] = 0;
      //             }
      //             // no neighbor
      //             else if (leftLPF[k] > rightLPF[k]) {
      //                lpf[k] = leftLPF[k];
      //             } else {
      //                lpf[k] = rightLPF[k];
      //             }
      //          });
      //          // for (lz_int k = i + 1; k < j; k++) {
      //          //    left  = leftElements[lpf[k]];
      //          //    right = rightElements[lpf[k]];

      //          //    if (left != -1) {
      //          //       llcp = std::max<lz_int>(leftLPF[k - 1] - 1, 0);
      //          //       while (sa[left] + llcp < n && k + llcp < n && seq[sa[left] + llcp] == seq[k + llcp]) {
      //          //          llcp++;
      //          //       }
      //          //       leftLPF[k] = llcp;
      //          //    } else
      //          //       leftLPF[k] = 0;

      //          //    if (right != -1) {
      //          //       rlcp = std::max<lz_int>(rightLPF[k - 1] - 1, 0);
      //          //       while (sa[right] + rlcp < n && k + rlcp < n && seq[sa[right] + rlcp] == seq[k + rlcp]) {
      //          //          rlcp++;
      //          //       }
      //          //       rightLPF[k] = rlcp;
      //          //    } else
      //          //       rightLPF[k] = 0;

      //          //    if (leftLPF[k] == 0 && rightLPF[k] == 0) {
      //          //       lpf[k] = 0;
      //          //    }
      //          //    // no neighbor
      //          //    else if (leftLPF[k] > rightLPF[k]) {
      //          //       lpf[k] = leftLPF[k];
      //          //    } else {
      //          //       lpf[k] = rightLPF[k];
      //          //    }
      //          // }
      //       },
      //       size);

      //    return n;
      // };

   }  // namespace utils
}  // namespace lz
