/***************************************************************************
                          pnm.h  -  description
                             -------------------
    begin                : 16 Nov 2013
    last modified        : 05 Oct 2018
    email                : estevez@fisica.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2014-2018 by Ernesto Estevez Rams   						   *
 *   estevez@fisica.uh.cu   											   *
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

#ifndef PNM_HEADER_H
#define PNM_HEADER_H

#include <lz/sequence.h>

#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "exceptions.h"
#include "utils.h"

#define PBM_MAXCHARLINE 70
#ifndef PPM_MAXVALUE
#define PPM_MAXVALUE 0xffff
#endif

#ifndef CHARBITS
#define CHARBITS                                                                                                       \
   std::numeric_limits<unsigned char>::digits  // This is the number of bits not counting the sign bit and the padding
                                               // bits (if any).
#endif

typedef enum { PNM_P1, PNM_P2, PNM_P3, PNM_P4, PNM_P5, PNM_P6, PNM_P7, PNM_RAWTXT, PNM_RAWBIN, CSV, TCSV, AUTO } MagickNumber;

inline std::unordered_map<MagickNumber, std::string> MagicValues{
   {MagickNumber::CSV, "CSV"},
   {MagickNumber::TCSV, "CSV"},
   {MagickNumber::AUTO, "AUTO"},
   {MagickNumber::PNM_P1, "PNM_P1"},
   {MagickNumber::PNM_P2, "PNM_P2"},
   {MagickNumber::PNM_P3, "PNM_P3"},
   {MagickNumber::PNM_P4, "PNM_P4"},
   {MagickNumber::PNM_P5, "PNM_P5"},
   {MagickNumber::PNM_P6, "PNM_P6"},
   {MagickNumber::PNM_P7, "PNM_P7"},
   {MagickNumber::PNM_RAWTXT, "PNM_RAWTXT"},
   {MagickNumber::PNM_RAWBIN, "PNM_RAWBIN"},
};

namespace lz {
   namespace utils {
      class pnm {
     protected:
         MagickNumber magick_number;
         std::string  header_dump;

         unsigned int height;
         unsigned int width;

         unsigned int maxvalue;

         std::string tultype;

         unsigned int linesizebound;  // maximum line size ( if linesizebound is exceeded a new line is forced)

     public:
         pnm(void)
           : magick_number(PNM_P1), height(0), width(0), maxvalue(1), linesizebound(PBM_MAXCHARLINE){};
         pnm(pnm& obj) { *this = obj; };

         pnm& operator=(pnm obj);

         std::istream& ReadPBM(std::istream& is, sequence& s, bool bin);
         std::istream& ReadPBM(std::istream& is, std::vector<sequence>& s, bool bin);

         std::ostream& SavePBM(std::ostream& os, const std::vector<sequence>& s, bool binary = true);
         std::ostream& SavePBM(std::ostream& os, const sequence& s, bool binary = true, int newfile = 0);

         std::istream& ReadPGM(std::istream& is, sequence& s, bool bin);
         std::istream& ReadPGM(std::istream& is, std::vector<sequence>& s, bool bin);

         std::ostream& SavePGM(std::ostream& os, const std::vector<sequence>& s, bool binary = true, char maxv = 0);
         std::ostream& SavePGM(std::ostream& os, const sequence& s, bool binary = true, int newfile = 0, char maxv = 0);

         std::ostream& SavePGM(std::ostream&                             os,
                               const std::vector<unsigned int>::iterator start,
                               const std::vector<unsigned int>::iterator end,
                               bool                                      binary,
                               int                                       newfile,
                               unsigned int                              maxv = 0);
         std::ostream& SavePGM(std::ostream&              os,
                               std::vector<unsigned int>& data,
                               unsigned int               width,
                               bool                       binary = true,
                               unsigned int               maxv   = 0);

         std::ostream& SavePPM(std::ostream&                             os,
                               const std::vector<unsigned int>::iterator start,
                               const std::vector<unsigned int>::iterator end,
                               bool                                      binary,
                               int                                       newfile,
                               unsigned int                              maxv = 0);
         std::ostream& SavePPM(std::ostream&              os,
                               std::vector<unsigned int>& data,
                               unsigned int               width,
                               bool                       binary = true,
                               unsigned int               maxv   = 0);

         std::istream& ReadRAW(std::istream& is, sequence& s, bool bin);
         std::istream& ReadRAW(std::istream& is, std::vector<sequence>& s, bool bin);

         std::istream& ReadPNM(std::istream& is, sequence& s);
         std::istream& ReadPNM(std::istream& is, std::vector<sequence>& s);

         unsigned int Height(void) const { return height; };
         unsigned int Width(void) const { return width; };
         unsigned int MaxChannelValue(void) const { return maxvalue; };
         std::string  TulType(void) const { return tultype; };

         std::string HeaderClear() {
            std::string str = header_dump;
            header_dump.clear();
            return str;
         };
         std::string HeaderDump(void) const { return header_dump; };
         std::string HeaderDump(const std::string& comment) {
            header_dump += comment;
            return header_dump;
         };

         void Clear(void) {
            width = height = 0;
            maxvalue       = 1;
            magick_number  = PNM_P1;
            header_dump.clear();
            linesizebound = 0;
            tultype.clear();
         };

         MagickNumber FileTypeQ(std::istream& is);

         unsigned int LineSizeBound(unsigned int lb) {
            std::swap(linesizebound, lb);
            return lb;
         };
         unsigned int LineSizeBound(void) { return linesizebound; };

         bool BinaryQ(void) const {
            return (magick_number == PNM_P4 || magick_number == PNM_P5 || magick_number == PNM_P6 ||
                    magick_number == PNM_P7);
         };
         bool TextQ(void) const { return !BinaryQ(); };

         bool PBMQ(void) const { return (magick_number == PNM_P1 || magick_number == PNM_P4); };
         bool PGMQ(void) const { return (magick_number == PNM_P2 || magick_number == PNM_P5); };
         bool PPMQ(void) const { return (magick_number == PNM_P3 || magick_number == PNM_P6); };
         bool PAMQ(void) const { return (magick_number == PNM_P7); };

         bool P1Q(void) const { return magick_number == PNM_P1; };
         bool P2Q(void) const { return magick_number == PNM_P2; };
         bool P3Q(void) const { return magick_number == PNM_P3; };
         bool P4Q(void) const { return magick_number == PNM_P4; };
         bool P5Q(void) const { return magick_number == PNM_P5; };
         bool P6Q(void) const { return magick_number == PNM_P6; };
         bool P7Q(void) const { return magick_number == PNM_P7; };
         bool RawQ(void) const { return magick_number == PNM_RAWTXT || magick_number == PNM_RAWBIN; };

         friend bool SameSizeQ(const pnm& obj1, const pnm& obj2);
         friend void swap(pnm& obj1, pnm& obj2);
      };

      // The error condition exceptions
      class PNMBadFileFormat : public FileFormatError {};
      class PNMBadFileOperation : public BadOperation {};
      class PNMDepth : public Errors {};
      class PNMUnknownError : public UnknownError {};
      class PNMInsuficientData : public Errors {};

      // .............................................................................
      // Name: operator=
      //
      // Synopsis: Copy operator
      //
      // Parameters:
      //			pnm obj     -----> source operand
      //
      // Returns:
      //         *this         -----> The object
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline pnm& pnm::operator=(pnm obj) {
         swap(*this, obj);

         return *this;
      }

      // ........................  Friend functions .................................

      // .............................................................................
      // Name: SameSizeQ
      //
      // Synopsis: Bool operator. Compares images sizes.
      //
      // Parameters:
      //			const pnm & obj1 -----> first operand
      //			const pnm & obj2 -----> second operand
      //
      // Returns:
      //         true if both images have equal size
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline bool SameSizeQ(const pnm& obj1, const pnm& obj2) {
         return obj1.height == obj2.height && obj1.width == obj2.width;
      }

      // .............................................................................
      // Name: swap
      //
      // Synopsis: swap operands
      //
      // Parameters:
      //			const pnm & obj1 -----> first operand
      //			const pnm & obj2 -----> second operand
      //
      // Returns:
      //         None
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline void swap(pnm& obj1, pnm& obj2) {
         std::swap(obj1.magick_number, obj2.magick_number);
         std::swap(obj1.header_dump, obj2.header_dump);
         std::swap(obj1.height, obj2.height);
         std::swap(obj1.width, obj2.width);
         std::swap(obj1.maxvalue, obj2.maxvalue);
         std::swap(obj1.tultype, obj2.tultype);
         std::swap(obj1.linesizebound, obj2.linesizebound);
      }
   }  // namespace utils
}  // namespace lz

#endif
