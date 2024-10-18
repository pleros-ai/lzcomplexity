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

namespace lz {

   namespace utils {

      struct stack_element {
         lz_uint len;
         lz_uint pos;

         stack_element(void) { len = pos = 0; };
         stack_element(lz_uint l, lz_uint p)
           : len(l), pos(p){};
         stack_element(const stack_element& s)
           : len(s.len), pos(s.pos){};
         stack_element(stack_element&& s)
           : len(std::exchange(s.len, 0)), pos(std::exchange(s.pos, 0)){};

         ~stack_element(void){};

         stack_element& operator=(stack_element s) {
            swap(*this, s);
            return *this;
         };
         stack_element& operator=(stack_element& s) {
            if (this != &s) {
               this->~stack_element();
               new (this) stack_element(s);
            }
            return *this;
         };
         stack_element& operator=(stack_element&& s) {
            if (this != &s) {
               len = std::exchange(s.len, std::numeric_limits<lz_uint>::max());
               pos = std::exchange(s.pos, std::numeric_limits<lz_uint>::max());
            }
            return *this;
         };
         bool operator==(const stack_element& s) const { return pos == s.pos && len == s.len; };
         bool operator!=(const stack_element& s) const { return !operator==(s); };

         friend void swap(stack_element&, stack_element&);
      };

      //.....................................................
      // auxiliary class defines a simple stack.
      //.....................................................
      /// \brief class defines a simple stack
      class LPFStack {
     protected:
         std::vector<struct stack_element> element;  //!< The elements of the stack

     public:
         LPFStack(void);       //!< Default constructor
         LPFStack(lz_size n);  //!< Constructor. Allocates memory according to n.
         ~LPFStack();          //!< Destructor.

         lz_size push(lz_uint len, lz_uint pos);  //!< Adds an element to the stack.
         lz_size push(struct stack_element el);   //!< Adds an element to the stack.
         lz_size pop(void);                       //!< Pops an element from the stack.

         struct stack_element Top(void) const;  //!< Returns the top element from the stack, without popping.
         lz_uint TopLen(void) const;  //!< Returns the len value of the top element from the stack, without popping.
         lz_uint TopPos(void) const;  //!< Returns the pos value of the top element from the stack, without popping.

         lz_size length(void) const;  //!< Returns the number of elements in the stack.
         lz_size No(void) const;      //!< Returns the number of elements in the stack.
         lz_size size(void) const;    //!< Returns the number of elements in the stack.
         bool    empty(void) const;   //!< Returns True if stack is empty, False otherwise.

         void clear(void);  //!< clears the stack
      };

      //*****************************************************
      //               class member functions
      //*****************************************************

      /// \brief
      /// Constructor. Allocates memory according to n.
      /// \param n the maximum number of elements that the stack will hold
      /// \return none.
      /// \sa ~LPFStack()
      inline LPFStack::LPFStack(lz_size n) {
         element.clear();
         element.reserve(n);
      }

      inline LPFStack::LPFStack(void) {
         element.clear();
      }

      /// \brief
      /// Destructor. Frees memory.
      /// \return none.
      /// \sa LPFStack()
      inline LPFStack::~LPFStack() {
         element.clear();
      }

      /// \brief
      /// Adds an element to the stack.
      /// \param len longest common prefix between \f$ suf_{x.pos} \f$  and the suffix corresponding to the node right
      /// below x in the stack (or 0 if none) \param pos position in suffix array \param el inserted element. For
      /// description of the factors see the two parameters above. \return The number of elements in the stack. \sa
      /// pop()
      inline lz_size LPFStack::push(lz_uint len, lz_uint pos) {
         struct stack_element s(len, pos);

         return push(s);
      }

      inline lz_size LPFStack::push(struct stack_element el) {
         element.push_back(el);

         return element.size();
      }

      /// \brief
      /// Returns the top element from the stack, without popping.
      /// \return The top element
      /// \sa pop()
      /// \sa push()
      inline struct stack_element LPFStack::Top(void) const {
         return element.back();
      }

      /// \brief
      /// Returns the len value of the top element from the stack, without popping.
      /// \return The len value of the top element
      inline lz_uint LPFStack::TopLen(void) const {
         return element.back().len;
      }

      /// \brief
      /// Returns the pos value of the top element from the stack, without popping.
      /// \return The pos value of the top element
      inline lz_uint LPFStack::TopPos(void) const {
         return element.back().pos;
      }

      /// \brief
      /// Pops an element from the stack.
      /// \return the stack size
      /// \sa push()
      inline lz_size LPFStack::pop(void) {
         element.pop_back();
         return element.size();
      }

      /// \brief
      /// Returns True if stack is empty, False otherwise.
      /// \return bool value.
      inline lz_bool LPFStack::empty(void) const {
         return element.empty();
      }

      /// \brief
      /// Returns the number of elements in the stack
      /// \return number of elements in the stack
      /// \sa No()
      inline lz_size LPFStack::length(void) const {
         return element.size();
      }

      /// \brief
      /// Returns the number of elements in the stack
      /// \return number of elements in the stack
      /// \sa length()
      inline lz_size LPFStack::No(void) const {
         return length();
      }

      /// \brief
      /// Returns the number of elements in the stack
      /// \return number of elements in the stack
      /// \sa length()
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

      /** @brief
       * Calculates the LPF of a sequence from its suffix array and LCP
       *
       * @param lpf   LPF resulting array
       * @param SA    suffix array
       * @param LCP   LCP array
       * @param n     length of the arrays
       * @return number of elements in LPF
       */
      lz_int LPF(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      /**
       * \brief
       * Calculates the LPF of a sequence from its suffix array and LCP
       * saving space. Use the LCP for that so its lost the real LCP array.
       *
       * @param lpf   LPF resulting array
       * @param SA    suffix array
       * @param LCP   LCP array
       * @param n     length of the arrays
       * @return number of elements in LPF
       */
      lz_int LPF_opt(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      lz_int LPF_par(std::vector<lz_uint>& lpf, std::vector<lz_uint> SA, std::vector<lz_uint> LCP, lz_size n);

      lz_int LPF_2(const std::string    s,
                   std::vector<lz_uint> sa,
                   lz_int               n,
                   std::vector<lz_uint> lcp,
                   std::vector<lz_int>& lpf,
                   lz_int*              prev_occ);

   }  // namespace utils

}  // namespace lz
