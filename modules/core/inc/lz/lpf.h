/***************************************************************************
                                                                  lpf_stack.h  -  description
                                                                          -------------------
         begin                : 26 Oct 2023
         email                : estevez@imre.oc.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013 by Ernesto Estevez Rams   						    *
 *   estevez@imre.oc.uh.cu   											    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <lz/parallel_utils.h>

#include <utility>
#include <vector>

#include "sequence.h"

namespace lz {

   namespace utils {

      /**
       * @brief Element stored in the LPF computation stack.
       *
       * Represents a suffix array position along with the longest common prefix (LCP)
       * length to the suffix immediately below it in the stack.
       */
      struct stack_element {
         lz_uint len;  ///< Longest common prefix length with the element below in the stack (or 0 if none).
         lz_uint pos;  ///< Position in the suffix array.

         /**
          * @brief Default constructor. Initializes both len and pos to 0.
          */
         stack_element(void) { len = pos = 0; };

         /**
          * @brief Constructs a stack element with specified length and position.
          * @param l The LCP length value.
          * @param p The suffix array position.
          */
         stack_element(lz_uint l, lz_uint p)
           : len(l), pos(p){};

         /**
          * @brief Copy constructor.
          * @param s The stack element to copy.
          */
         stack_element(const stack_element& s)
           : len(s.len), pos(s.pos){};

         /**
          * @brief Move constructor.
          * @param s The stack element to move from.
          */
         stack_element(stack_element&& s)
           : len(std::exchange(s.len, 0)), pos(std::exchange(s.pos, 0)){};

         /**
          * @brief Destructor.
          */
         ~stack_element(void){};

         /**
          * @brief Copy-and-swap assignment operator.
          * @param s The stack element to assign from.
          * @return Reference to this element.
          */
         stack_element& operator=(stack_element s) {
            swap(*this, s);
            return *this;
         };

         /**
          * @brief Copy assignment operator.
          * @param s The stack element to copy from.
          * @return Reference to this element.
          */
         stack_element& operator=(stack_element& s) {
            if (this != &s) {
               this->~stack_element();
               new (this) stack_element(s);
            }
            return *this;
         };

         /**
          * @brief Move assignment operator.
          * @param s The stack element to move from.
          * @return Reference to this element.
          */
         stack_element& operator=(stack_element&& s) {
            if (this != &s) {
               len = std::exchange(s.len, std::numeric_limits<lz_uint>::max());
               pos = std::exchange(s.pos, std::numeric_limits<lz_uint>::max());
            }
            return *this;
         };

         /**
          * @brief Equality comparison operator.
          * @param s The stack element to compare with.
          * @return true if both len and pos are equal.
          */
         bool operator==(const stack_element& s) const { return pos == s.pos && len == s.len; };

         /**
          * @brief Inequality comparison operator.
          * @param s The stack element to compare with.
          * @return true if elements differ.
          */
         bool operator!=(const stack_element& s) const { return !operator==(s); };

         /**
          * @brief Swaps the contents of two stack elements.
          * @param first The first element.
          * @param second The second element.
          */
         friend void swap(stack_element&, stack_element&);
      };

      /**
       * @brief Stack data structure used in Longest Previous Factor (LPF) computation.
       *
       * A specialized stack for computing the LPF array from suffix arrays.
       * Each element stores a suffix array position and its LCP with the element below.
       */
      class LPFStack {
     protected:
         std::vector<struct stack_element> element;  ///< Internal storage for stack elements.

     public:
         /**
          * @brief Default constructor. Creates an empty stack.
          */
         LPFStack(void);

         /**
          * @brief Constructor with capacity reservation.
          * @param n The expected maximum number of elements (for memory pre-allocation).
          */
         LPFStack(lz_size n);

         /**
          * @brief Destructor. Clears all elements.
          */
         ~LPFStack();

         /**
          * @brief Pushes a new element onto the stack.
          * @param len The LCP length between the new suffix and the current top.
          * @param pos The suffix array position.
          * @return The new size of the stack.
          */
         lz_size push(lz_uint len, lz_uint pos);

         /**
          * @brief Pushes an existing stack element onto the stack.
          * @param el The element to push.
          * @return The new size of the stack.
          */
         lz_size push(struct stack_element el);

         /**
          * @brief Removes the top element from the stack.
          * @return The new size of the stack.
          * @note Behavior is undefined if the stack is empty.
          */
         lz_size pop(void);

         /**
          * @brief Returns the top element without removing it.
          * @return A copy of the top stack element.
          * @note Behavior is undefined if the stack is empty.
          */
         struct stack_element Top(void) const;

         /**
          * @brief Returns the LCP length of the top element.
          * @return The len field of the top element.
          * @note Behavior is undefined if the stack is empty.
          */
         lz_uint TopLen(void) const;

         /**
          * @brief Returns the suffix array position of the top element.
          * @return The pos field of the top element.
          * @note Behavior is undefined if the stack is empty.
          */
         lz_uint TopPos(void) const;

         /**
          * @brief Returns the number of elements in the stack.
          * @return The current stack size.
          */
         lz_size length(void) const;

         /**
          * @brief Returns the number of elements in the stack (alias for length()).
          * @return The current stack size.
          */
         lz_size No(void) const;

         /**
          * @brief Returns the number of elements in the stack (alias for length()).
          * @return The current stack size.
          */
         lz_size size(void) const;

         /**
          * @brief Checks if the stack is empty.
          * @return true if the stack contains no elements, false otherwise.
          */
         bool empty(void) const;

         /**
          * @brief Removes all elements from the stack.
          */
         void clear(void);
      };

      //*****************************************************
      //               class member functions
      //*****************************************************

      inline LPFStack::LPFStack(lz_size n) {
         element.clear();
         element.reserve(n);
      }

      inline LPFStack::LPFStack(void) {
         element.clear();
      }

      inline LPFStack::~LPFStack() {
         element.clear();
      }

      inline lz_size LPFStack::push(lz_uint len, lz_uint pos) {
         struct stack_element s(len, pos);

         return push(s);
      }

      inline lz_size LPFStack::push(struct stack_element el) {
         element.push_back(el);

         return element.size();
      }

      inline struct stack_element LPFStack::Top(void) const {
         return element.back();
      }

      inline lz_uint LPFStack::TopLen(void) const {
         return element.back().len;
      }

      inline lz_uint LPFStack::TopPos(void) const {
         return element.back().pos;
      }

      inline lz_size LPFStack::pop(void) {
         element.pop_back();
         return element.size();
      }

      inline lz_bool LPFStack::empty(void) const {
         return element.empty();
      }

      inline lz_size LPFStack::length(void) const {
         return element.size();
      }

      inline lz_size LPFStack::No(void) const {
         return length();
      }

      inline lz_size LPFStack::size(void) const {
         return length();
      }

      //-------------------------------------------------
      //                  Friend functions
      //-------------------------------------------------
      inline void swap(stack_element& first, stack_element& second) {
         std::swap(first.len, second.len);
         std::swap(first.pos, second.pos);
      }

      //*****************************************************
      //         Longest Previous Factor (LPF)
      //*****************************************************

      /**
       * @brief Computes the Longest Previous Factor (LPF) array from suffix array and LCP.
       *
       * For each position i in the original sequence, LPF[i] gives the length of the
       * longest factor starting at i that also occurs earlier in the sequence.
       *
       * @param[out] lpf The resulting LPF array (will be resized as needed).
       * @param[in] SA The suffix array of the sequence.
       * @param[in] LCP The Longest Common Prefix array.
       * @param[in] n The length of the sequence (and arrays).
       * @return The number of elements in the computed LPF array.
       */
      lz_int LPF(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      /**
       * @brief Computes the LPF array with optimized memory usage.
       *
       * This variant reuses the LCP array for intermediate storage, reducing memory
       * footprint. The original LCP array contents are destroyed in the process.
       *
       * @param[out] lpf The resulting LPF array.
       * @param[in] SA The suffix array of the sequence.
       * @param[in,out] LCP The LCP array (will be modified/destroyed).
       * @param[in] n The length of the sequence.
       * @return The number of elements in the computed LPF array.
       * @warning The LCP array is modified and its original values are lost.
       */
      lz_int LPF_opt(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      /**
       * @brief Computes the LPF array using parallel processing.
       *
       * A parallelized version of the LPF computation for improved performance
       * on multi-core systems.
       *
       * @param[out] lpf The resulting LPF array.
       * @param[in] SA The suffix array of the sequence.
       * @param[in] LCP The LCP array.
       * @param[in] n The length of the sequence.
       * @return The number of elements in the computed LPF array.
       */
      lz_int LPF_par(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      /**
       * @brief Alternative LPF computation directly from sequence and suffix array.
       *
       * Computes the LPF array without requiring a pre-computed LCP array.
       *
       * @param[in] seq The input sequence.
       * @param[in] sa The suffix array.
       * @param[in] n The length of the sequence.
       * @param[out] lpf The resulting LPF array.
       * @return The number of elements in the computed LPF array.
       */
      lz_int LPF_2(const sequence& seq, std::vector<lz_uint> sa, lz_int n, std::vector<lz_int>& lpf);

   }  // namespace utils

}  // namespace lz
