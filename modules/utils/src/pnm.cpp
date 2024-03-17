/***************************************************************************
                          pnm.cpp  -  description
                             -------------------
    begin                : 16 Nov 2013
    last modified        : 15 Mar 2024
    email                : estevez@fisica.uh.cu
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2013-2018 by Ernesto Estevez Rams   						   *
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

#include <lz/pnm.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

namespace lz {
   namespace internal {
      // .............................................................................
      // Name: InsertComment
      //
      // Synopsis: outputs to a stream a comment string inserting the comment token at each new line
      //
      // Parameters:
      //			std::ostream & os        ----> the output sream
      //          const std::string & str  ----> the comment string
      //          char commenttoken='#'    ----> the commnet token
      //
      // Returns:
      //         std::ostream & os        ----> the output sream
      //
      // Exceptions:
      //            None
      //..............................................................................
      inline std::ostream& InsertComment(std::ostream& os, const std::string& str, char commenttoken = '#') {
         os << commenttoken << " ";
         for (char c: str) {
            os << c;

            if (c == utils::newline_char) os << commenttoken << " ";
         }
         os << std::endl;
         return os;
      }

      // .............................................................................
      // Name: ToChar
      //
      // Synopsis: Converts a two byte int to two char
      //
      // Parameters:
      //			std::uint16_t val           ----> to be converted
      //          std::uint8_t c[2]           ----> converted char
      //
      // Returns:
      //         None
      //
      // Exceptions:
      //            None
      //..............................................................................

      inline void ToChar(std::uint16_t val, std::uint8_t c[2]) {
         c[0] = static_cast<unsigned char>((val & 0xff00) >> 8);
         c[1] = static_cast<unsigned char>((val & 0x00ff));
      }
   }  // namespace internal

   namespace utils {
      // .............................................................................
      // Name: ReadBin
      //
      // Synopsis: serialization input operator. Binary input is expected
      //
      // Parameters:
      //			std::istream& is          -----> input stream
      //       const binsequence & obj   -----> upon returning the read input sequence
      //       std::vector<bool>::size_type size=0  ----> Number of bits to read. If zero, reads until eof.
      //
      // Returns:
      //         the last read line
      //
      // Exceptions:
      //            BinarySequenceBadAlloc      --------> Memory allocation failed.
      //            BinarySequenceError         --------> Generic error.
      //..............................................................................
      std::istream& ReadBin(std::istream& is, sequence& obj, lz_size size) {
         lz_size i = 0;        // the current bit index
         unsigned char c = 0;  // the current byte
         short bits = 0;       // to process next byte
         char mask = 0;
         lz_size ss = size;

         if (ss == 0) ss = std::numeric_limits<unsigned int>::max();  // size was set to zero, so we read until EOF

         obj.clear();

         while (is.good() && !is.eof()) {
            c = is.get();

            bits = CHARBITS;

            while (bits) {
               mask = 0x01 << (bits - 1);

               try {
                  obj.push(((c & mask) != 0));
               } catch (std::bad_alloc& ba) {
                  throw SequenceBadAlloc();
               } catch (...) {
                  throw SequenceError();
               }

               --bits;

               if (++i >= ss) break;
            }

            if (i >= size) {
               break;
            }
         }

         return is;
      }

      // .............................................................................
      // Name: WriteBin
      //
      // Synopsis: serialization output operator
      //
      // Notes:
      //      It dumps the binsequence to the output stream as a binary stream of the sequence values.
      //      No special formatting is done, just the binary dumping.
      //
      // Parameters:
      //			std::ostream& os          -----> output stream
      //          const binsequence & obj   -----> object to be serialized
      //
      // Returns:
      //         the output stream
      //
      // Exceptions:
      //            None
      //..............................................................................
      std::ostream& WriteBin(std::ostream& os, const sequence& obj) {
         lz_size i = 0;        // the current bit index
         unsigned char c = 0;  // the current byte
         short bits = 0;       // to process next byte

         while (i < obj.size()) {
            c = c << 1;
            if (obj.const_at(i)) ++c;  // adding 1 if bit is true
            ++bits;
            if (bits == CHARBITS) {  // The byte has been built
               os.put((char)c);
               c = 0;
               bits = 0;
            }
            ++i;
         }

         // dump remaining
         if (bits != 0) {
            // pad the byte so that first bits are in the most significant positions.
            while (bits != CHARBITS) {
               c = c << 1;
               ++bits;
            }
            os.put((char)c);
         }

         return os;
      }

      // .............................................................................
      // Name: WriteNonBin
      //
      // Synopsis: serialization output operator
      //
      // Notes:
      //      It dumps the binsequence to the output stream as a text stream of the sequence values.
      //      A new line character is written everytime the linesizebound is reached.
      //
      // Parameters:
      //			std::ostream& os            -----> output stream
      //          const binsequence & obj     -----> object to be serialized
      //          unsigned int linepos        -----> current position in output line
      //          unsigned int linesizebound  -----> maximum line size ( if linepos > linesizebound a new line is
      //          forced)
      //
      // Returns:
      //         the line position at the end of the output operation
      //
      // Exceptions:
      //            None
      //..............................................................................
      unsigned int WriteNonBin(std::ostream& os, const sequence& obj, unsigned int linepos,
                               unsigned int linesizebound) {
         static unsigned int linechar = linepos;
         char val = 0;

         for (std::vector<bool>::size_type i = 0; i < obj.size(); ++i) {
            val = (obj.const_at(i)) ? '1' : '0';
            os << val;
            if (++linechar > linesizebound) {  // force a new line
               linechar = 0;
               os << std::endl;
            }
         }

         return linechar;
      }

      // .............................................................................
      // Name: FileTypeQ
      //
      // Synopsis: asks for the type of fyle
      //
      // Parameters:
      //			std::istream & is -----> input string
      //
      // Returns:
      //         The fyle type as a MagickNumber
      // Notes:
      //       1) If the file is RAW (non PNM) and the three first
      //          character are alphanumeric, it is assumed to be a text file, binary otherwise
      //
      // Exceptions:
      //            None
      //..............................................................................
      MagickNumber pnm::FileTypeQ(std::istream& is) {
         char c[5];

         c[0] = c[1] = c[2] = c[3] = c[4] = 'A';  // just in case the line is shorter than three
         std::iostream::pos_type fpos = is.tellg();
         is.seekg(0);     // rewind to the beginning of the file
         is.get(c, 5);    // get signature
         is.seekg(fpos);  // restore the file pointer to the initial position

         if (c[0] == 'P' || c[0] == 'p') {
            switch (c[1]) {
               case '1':
                  magick_number = PNM_P1;
                  break;
               case '2':
                  magick_number = PNM_P2;
                  break;
               case '3':
                  magick_number = PNM_P3;
                  break;
               case '4':
                  magick_number = PNM_P4;
                  break;
               case '5':
                  magick_number = PNM_P5;
                  break;
               case '6':
                  magick_number = PNM_P6;
                  break;
               case '7':
                  magick_number = PNM_P7;
                  break;
               default:
                  throw PNMBadFileFormat();
            }
         } else {
            if ((std::isalnum(c[0]) || c[0] == ' ' || c[0] == '\t' || c[0] == '\n') &&
                (std::isalnum(c[1]) || c[1] == ' ' || c[1] == '\t' || c[1] == '\n') &&
                (std::isalnum(c[2]) || c[2] == ' ' || c[2] == '\t' || c[2] == '\n'))
               magick_number = PNM_RAWTXT;
            else
               magick_number = PNM_RAWBIN;
         }

         return magick_number;
      }

      // .............................................................................
      // Name: ReadPBM
      //
      // Synopsis: Read a PBM file
      //
      // Parameters:
      //			std::istream& is                -----> input stream
      //          binsequence & s                 -----> the input as a binsequence.
      //          std::vector<binsequence> & sv   -----> The input as a vector of binsequence, each member a line of the
      //          image.
      //
      // Returns:
      //         the input stream
      //
      // Exceptions:
      //            BinarySequenceBadAlloc      --------> Memory allocation failed.
      //            PNMUnknownError             --------> Generic error.
      //            PNMBadFileFormat            --------> The file was not a valid PNM file
      //..............................................................................
      std::istream& pnm::ReadPBM(std::istream& is, sequence& s, bool bin) {
         std::string line;
         char c[10];
         const std::string newl = "\n";

         try {
            // get signature
            is.get(c, 3, '\n');

            if ((c[0] == 'P' || c[0] == 'p') && (c[1] == '1' || c[1] == '4')) {  // read header of PBM file

               magick_number = (c[1] == '1') ? PNM_P1 : PNM_P4;
               maxvalue = 1;

               while (line.size() == 0 && is.good()) {
                  std::getline(is, line);
                  line = string_trim(line);
               }

               while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                  if (line.at(0) == '#') {
                     header_dump += newl;
                     header_dump += line;
                  }
                  std::getline(is, line);
                  line = string_trim(line);
               }

               int val = 0;
               int twice = 0;

               // reading image size
               {
                  std::stringstream iss(line);  // local scope
                  while (iss >> val) {
                     if (twice == 0) width = val;
                     if (twice == 1) height = val;
                     if (twice > 1) break;
                     ++twice;
                  }
               }

               if (twice == 1) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newl;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  iss >> val;

                  height = val;
               }
            }  // end reading image size
            else
               throw PNMBadFileFormat();

            if (magick_number == PNM_P4) {  // binary raw data
               if (!bin) throw PNMBadFileFormat();
               s.clear();
               ReadBin(is, s, width * height);
            } else {  // text data
               if (bin) throw PNMBadFileFormat();
               s.clear();
               unsigned int currentline = 0;
               unsigned int currentwidth = 0;

               line.clear();

               // text raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newl;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  char val = 0;

                  for (std::string::size_type i = 0; i < line.size(); ++i) {
                     val = line[i];

                     if (val != '0' && val != '1') {
                        continue;
                     }

                     s.push((val == '1'));

                     if (++currentwidth >= width) {
                        currentwidth = 0;
                        currentline++;
                     }
                     if (currentline >= height) break;
                  }
                  line.clear();
                  if (currentline >= height) break;
               }  // while true
            }     // if P1
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }
      //..............................................................................
      std::istream& pnm::ReadPBM(std::istream& is, std::vector<sequence>& s, bool bin) {
         std::string line;
         char c[10];
         sequence bs;

         try {
            // get signature
            is.get(c, 3, '\n');

            if ((c[0] == 'P' || c[0] == 'p') && (c[1] == '1' || c[1] == '4')) {  // read header of PBM file

               magick_number = (c[1] == '1') ? PNM_P1 : PNM_P4;
               maxvalue = 1;

               while (line.size() == 0 && is.good()) {
                  std::getline(is, line);
                  line = string_trim(line);
               }

               while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                  if (line.at(0) == '#') {
                     header_dump += newline_char;
                     header_dump += line;
                  }
                  std::getline(is, line);
                  line = string_trim(line);
               }

               int val = 0;
               int twice = 0;

               // reading image size
               {
                  std::stringstream iss(line);  // local scope
                  while (iss >> val) {
                     if (twice == 0) width = val;
                     if (twice == 1) height = val;
                     if (twice > 1) break;
                     ++twice;
                  }
               }

               if (twice == 1) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newline_char;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  iss >> val;

                  height = val;
               }
            }  // end reading image size
            else
               throw PNMBadFileFormat();

            if (magick_number == PNM_P4) {
               if (!bin) throw PNMBadFileFormat();

               s.clear();
               // binary raw data
               for (unsigned int i = 0; i < height; ++i) {
                  if (!is.good()) break;
                  ReadBin(is, bs, width);
                  s.push_back(bs);
               }
            } else {  // text data
               if (bin) throw PNMBadFileFormat();
               s.clear();
               unsigned int currentline = 0;
               unsigned int currentwidth = 0;

               bs.clear();
               line.clear();

               // raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newline_char;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  char val = 0;

                  for (std::string::size_type i = 0; i < line.size(); ++i) {
                     val = line[i];

                     if (val != '0' && val != '1') continue;

                     bs.push((val == '1'));

                     if (++currentwidth >= width) {
                        currentwidth = 0;
                        s.push_back(bs);
                        currentline++;
                        bs.clear();
                     }
                     if (currentline >= height) break;
                  }
                  line.clear();
                  if (currentline >= height) break;
               }  // while true
            }     // if P1
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }

      // .............................................................................
      // Name: SavePBM
      //
      // Synopsis: Save a PBM file
      //
      // Parameters:
      //			std::ostream& os                 -----> output stream
      //          std::vector<binsequence> & s     -----> the output sequence vector
      //          bool binary                      -----> true: P4, false P1
      //          int newfile                      -----> 0: The output is a new line in an existing PBM output
      //                                                  >0: New PBM file. newfile is the number of lines (height) in
      //                                                  the output.
      //
      // Returns:
      //         the output stream
      //
      // Exceptions:
      //           None
      //..............................................................................
      std::ostream& pnm::SavePBM(std::ostream& os, const sequence& s, bool binary, int newfile) {
         static unsigned int linepos = 0;

         if (newfile > 0) {
            // signature
            os.put((char)'P');
            os.put((char)((binary) ? '4' : '1'));
            os.put((char)'\n');

            // class signature
            os << "# Sequence dumping." << std::endl;

            internal::InsertComment(os, header_dump, '#');

            // dimensions
            width = s.size();
            height = newfile;

            os << width << "   " << height << std::endl;

            linepos = 0;
         }

         if (binary)  // binary dump
            WriteBin(os, s);
         else  // text dump
            linepos = WriteNonBin(os, s, linepos, linesizebound);

         return os;
      }

      //.................................................................................
      std::ostream& pnm::SavePBM(std::ostream& os, const std::vector<sequence>& s, bool binary) {
         std::vector<sequence>::const_iterator it = s.begin();

         SavePBM(os, *it, binary, s.size());
         it++;

         while (it != s.end()) SavePBM(os, *it++, binary, 0);

         return os;
      }

      // .............................................................................
      // Name: ReadPGM
      //
      // Synopsis: Read a PGM file
      //
      // Parameters:
      //			std::istream& is                -----> input stream
      //          sequence & s                    -----> the input as a binsequence.
      //          std::vector<binsequence> & sv   -----> The input as a vector of binsequence, each member a line of the
      //          image.
      //
      // Returns:
      //         the input stream
      //
      // Exceptions:
      //            BinarySequenceBadAlloc      --------> Memory allocation failed.
      //            PNMDepth                    --------> The image depth is not valid
      //            PNMUnknownError             --------> Generic error.
      //            PNMBadFileFormat            --------> The file was not a valid PNM file
      //..............................................................................
      std::istream& pnm::ReadPGM(std::istream& is, sequence& s, bool bin) {
         std::string line;
         char c[10];

         try {
            // get signature
            is.get(c, 3, '\n');

            if ((c[0] == 'P' || c[0] == 'p') && (c[1] == '2' || c[1] == '5')) {  // read header of PGM file

               magick_number = (c[1] == '2') ? PNM_P2 : PNM_P5;

               while (line.size() == 0 && is.good()) {
                  std::getline(is, line);
                  line = string_trim(line);
               }

               while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                  if (line.at(0) == '#') {
                     header_dump += newline_char;
                     header_dump += line;
                  }
                  std::getline(is, line);
                  line = string_trim(line);
               }

               int val = 0;
               int trice = 0;

               // reading image size
               {
                  std::stringstream iss(line);  // local scope
                  while (iss >> val) {
                     if (trice == 0) width = val;
                     if (trice == 1) height = val;
                     if (trice == 2) maxvalue = val;
                     if (trice > 2) break;
                     ++trice;
                  }
               }

               if (trice == 1) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newline_char;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  while (iss >> val) {
                     if (trice == 1) height = val;
                     if (trice == 2) maxvalue = val;
                     if (trice > 2) break;
                     ++trice;
                  }
               }

               if (trice == 2) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newline_char;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  iss >> val;

                  maxvalue = val;
               }

               if (maxvalue > 255) throw PNMDepth();
            }  // end reading image size
            else
               throw PNMBadFileFormat();

            if (magick_number == PNM_P5) {  // binary raw data

               if (!bin) throw PNMBadFileFormat();
               s.clear();
               unsigned int value = 0;

               for (unsigned long index = 0; index < height * width; ++index) {
                  value = is.get();
                  s.push((unsigned char)value);
               }
            } else {  // text data

               if (bin) throw PNMBadFileFormat();
               s.clear();

               line.clear();
               unsigned int total_l = width * height;
               unsigned int number = 0;

               // raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newline_char;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope
                  int val = 0;

                  while (iss >> val) {
                     s.push((unsigned char)val);
                     number++;
                     if (number > total_l) break;
                  }
                  line.clear();
                  if (number > total_l)  // are we done ?
                     break;              // yes
               }                         // while true
            }                            // if P2
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }

      //..........................................................................
      std::istream& pnm::ReadPGM(std::istream& is, std::vector<sequence>& s, bool bin) {
         std::string line;
         char c[10];
         const std::string newl = "\n";
         sequence bs;

         try {
            // get signature
            is.get(c, 3, '\n');

            if ((c[0] == 'P' || c[0] == 'p') && (c[1] == '2' || c[1] == '5')) {  // read header of PGM file

               magick_number = (c[1] == '2') ? PNM_P2 : PNM_P5;

               while (line.size() == 0 && is.good()) {
                  std::getline(is, line);
                  line = string_trim(line);
               }

               while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                  if (line.at(0) == '#') {
                     header_dump += newl;
                     header_dump += line;
                  }
                  std::getline(is, line);
                  line = string_trim(line);
               }

               int val = 0;
               int trice = 0;

               // reading image size
               {
                  std::stringstream iss(line);  // local scope
                  while (iss >> val) {
                     if (trice == 0) width = val;
                     if (trice == 1) height = val;
                     if (trice == 2) maxvalue = val;
                     if (trice > 2) break;
                     ++trice;
                  }
               }

               if (trice == 1) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newl;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  while (iss >> val) {
                     if (trice == 1) height = val;
                     if (trice == 2) maxvalue = val;
                     if (trice > 2) break;
                     ++trice;
                  }
               }

               if (trice == 2) {
                  line.clear();
                  val = 0;

                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newl;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope

                  iss >> val;

                  maxvalue = val;
               }

               if (maxvalue > 255) throw PNMDepth();
            }  // end reading image size
            else
               throw PNMBadFileFormat();

            if (magick_number == PNM_P5) {  // binary raw data

               if (!bin) throw PNMBadFileFormat();

               s.clear();
               unsigned int value;

               for (unsigned int indexh = 0; indexh < height; ++indexh) {
                  for (unsigned int indexw = 0; indexw < width; ++indexw) {
                     value = is.get();
                     bs.push((unsigned char)value);
                  }
                  s.push_back(bs);
                  bs.clear();
               }
            } else {  // text data

               if (bin) throw PNMBadFileFormat();

               s.clear();
               unsigned int currentline = 0;
               unsigned int currentwidth = 0;

               bs.clear();
               line.clear();

               // raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  while ((line.at(0) == '#' || line.size() == 0) && is.good()) {  // get rid of comments
                     if (line.at(0) == '#') {
                        header_dump += newl;
                        header_dump += line;
                     }
                     std::getline(is, line);
                     line = string_trim(line);
                  }

                  std::stringstream iss(line);  // local scope
                  int val = 0;

                  while (iss >> val) {
                     bs.push((unsigned char)val);

                     if (++currentwidth >= width) {
                        s.push_back(bs);
                        currentwidth = 0;
                        currentline++;
                        bs.clear();
                     }
                     if (currentline >= height) break;
                  }
                  line.clear();
                  if (currentline >= height)  // are we done ?
                     break;
               }  // while true
            }     // if P2
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }

      // .............................................................................
      // Name: SavePGM
      //
      // Synopsis: Save a PGM file
      //
      // Parameters:
      //			std::ostream& os                 -----> output stream
      //          std::vector<sequence> & s        -----> the output sequence vector
      //          sequence & s                     -----> the output sequence
      //          bool binary                      -----> true: P5, false P2
      //          int newfile                      -----> 0: The output is a new line in an existing PGM output
      // 												    >0: New PGM
      // file. newfile is the number of lines (height) in the output.
      //
      // Returns:
      //         the output stream
      //
      // Exceptions:
      //           None
      //
      // Notes:
      //       1) if for some value of s.size() (with newfile==0) the length of the sequence is not
      //          equal to the original width (s.size()!= width), binary file option should not be used.
      //          Some PGM visualizers do not know how to handle it.
      //..............................................................................
      std::ostream& pnm::SavePGM(std::ostream& os, const sequence& s, bool binary, int newfile, char maxv) {
         static unsigned int linepos = 0;
         unsigned char max = 0;
         unsigned char min = 0;

         if (newfile > 0) {
            // signature
            os.put((char)'P');
            os.put((char)((binary) ? '5' : '2'));
            os.put((char)'\n');

            // class signature
            os << "# Sequence dumping." << std::endl;

            internal::InsertComment(os, header_dump, '#');

            // dimensions
            width = s.size();
            height = newfile;

            if (maxv == 0) {
               max = (unsigned char)s.Max();
               min = (unsigned char)s.Min();

               if (max < min) std::swap(max, min);
            } else
               max = maxv;

            os << width << " " << height << std::endl;
            os << (unsigned int)max << std::endl;

            linepos = 0;
         }

         if (binary) {  // binary dump
            char val = 0;
            for (unsigned int i = 0; i < width && i < s.size(); ++i) {
               val = s.const_at(i);
               os << val;
            }
         } else {  // text dump
            unsigned int val = 0;
            unsigned int linechar = linepos;
            for (unsigned int i = 0; i < width && i < s.size(); ++i) {
               val = (unsigned char)s.const_at(i);
               os << std::to_string(val) << " ";
               linechar++;

               if (linechar > linesizebound) {
                  linechar = 0;
                  os << std::endl;
               }
            }

            linepos = linechar;
         }

         return os;
      }

      //.............
      std::ostream& pnm::SavePGM(std::ostream& os, const std::vector<unsigned int>::iterator start,
                                 const std::vector<unsigned int>::iterator end, bool binary, int newfile,
                                 unsigned int maxv) {
         static unsigned int linepos = 0;
         std::vector<unsigned int>::const_iterator it = start;

         if (newfile > 0) {
            // signature
            os.put((char)'P');
            os.put((char)((binary) ? '5' : '2'));
            os.put((char)'\n');

            // class signature
            os << "# Sequence dumping." << std::endl;

            internal::InsertComment(os, header_dump, '#');

            // dimensions
            width = (end - start);
            height = newfile;

            if (maxv == 0)
               maxvalue = *std::max_element(start, end);
            else
               maxvalue = maxv;

            os << width << " " << height << std::endl;
            os << maxvalue << std::endl;

            linepos = 0;
         }

         if (binary) {  // binary dump
            std::uint8_t c[2];
            for (unsigned int i = 0; i < width && it < end; ++i) {
               internal::ToChar(static_cast<std::uint16_t>(*it++), c);

               if (maxvalue < 256)
                  os << c[1];
               else
                  os << c[0] << c[1];
            }
         } else {  // text dump
            unsigned int linechar = linepos;
            for (unsigned int i = 0; i < width && it < end; ++i) {
               os << std::to_string(static_cast<std::uint16_t>(*it++)) << " ";
               if (linechar > linesizebound) {
                  linechar = 0;
                  os << std::endl;
               }
            }

            linepos = linechar;
         }

         return os;
      }

      //...........
      std::ostream& pnm::SavePGM(std::ostream& os, const std::vector<sequence>& s, bool binary, char maxv) {
         std::vector<sequence>::const_iterator it = s.begin();

         SavePGM(os, *it, binary, s.size(), maxv);
         it++;

         while (it != s.end()) SavePGM(os, *it++, binary, 0, maxv);

         return os;
      }

      //...........
      std::ostream& pnm::SavePGM(std::ostream& os, std::vector<unsigned int>& data, unsigned int w, bool binary,
                                 unsigned int maxv) {
         std::vector<unsigned int>::iterator it = data.begin();
         std::vector<unsigned int>::iterator itend;
         unsigned int h = data.size() / w;

         width = w;

         if (data.size() % width > 0) h++;

         height = h;

         itend = it + width;
         if (itend >= data.end()) itend = data.end();

         SavePGM(os, it, itend, binary, height, maxv);
         it += width;

         while (it < data.end()) {
            itend = it + width;
            if (itend >= data.end()) itend = data.end();
            SavePGM(os, it, itend, binary, 0, maxv);
            it += width;
         }

         return os;
      }

      // .............................................................................
      // Name: SavePPM
      //
      // Synopsis: Save a PPM file
      //
      // Parameters:
      //			std::ostream& os                 -----> output stream
      //          std::vector<sequence> & s        -----> the output sequence vector
      //          sequence & s                     -----> the output sequence
      //          bool binary                      -----> true: P5, false P2
      //          int newfile                      -----> 0: The output is a new line in an existing PGM output
      // 												    >0: New PGM
      // file. newfile is the number of lines (height) in the output.
      //
      // Returns:
      //         the output stream
      //
      // Exceptions:
      //           PNMInsuficientData  ---> The data has not enough values according to the image width
      //
      // Notes:
      //       1) if for some value of s.size() (with newfile==0) the length of the sequence is not
      //          equal to the original width (s.size()!= width), binary file option should not be used.
      //          Some PGM visualizers do not know how to handle it.
      //..............................................................................
      std::ostream& pnm::SavePPM(std::ostream& os, const std::vector<unsigned int>::iterator start,
                                 const std::vector<unsigned int>::iterator end, bool binary, int newfile,
                                 unsigned int maxv) {
         static unsigned int linepos = 0;
         static unsigned int max = 0;
         static unsigned int min = 0;
         static unsigned int maxmin = 0;
         std::vector<unsigned int>::const_iterator it = start;

         if (newfile > 0) {
            // signature
            os.put((char)'P');
            os.put((char)((binary) ? '6' : '3'));
            os.put((char)'\n');

            // class signature
            os << "# data dumping." << std::endl;

            internal::InsertComment(os, header_dump, '#');

            // dimensions
            width = (end - start) / 3;
            height = newfile;

            if (maxv == 0) {
               max = *std::max_element(start, end);
               min = *std::min_element(start, end);

               maxmin = max - min;
            } else {
               max = maxmin = maxv;
               min = 0;
            }

            os << width << " " << height << std::endl;
            os << max << std::endl;

            linepos = 0;
         }

         if (binary) {  // binary dump
            std::uint8_t c[2];
            for (unsigned int i = 0; i < width && it < end; ++i) {
               internal::ToChar(static_cast<std::uint16_t>(*it++), c);

               if (max < 256)
                  os << c[1];
               else
                  os << c[0] << c[1];

               if (it == end) throw PNMInsuficientData();

               internal::ToChar(static_cast<std::uint16_t>(*it++), c);

               if (max < 256)
                  os << c[1];
               else
                  os << c[0] << c[1];

               if (it == end) throw PNMInsuficientData();

               internal::ToChar(static_cast<std::uint16_t>(*it++), c);

               if (max < 256)
                  os << c[1];
               else
                  os << c[0] << c[1];
            }
         } else {  // text dump
            unsigned int linechar = linepos;
            for (unsigned int i = 0; i < width && it < end; ++i) {
               os << std::to_string(static_cast<std::uint16_t>(*it++)) << " ";
               if (it == end) throw PNMInsuficientData();
               os << std::to_string(static_cast<std::uint16_t>(*it++)) << " ";
               if (it == end) throw PNMInsuficientData();
               os << std::to_string(static_cast<std::uint16_t>(*it++)) << "  ";
               linechar++;

               if (linechar > linesizebound) {
                  linechar = 0;
                  os << std::endl;
               }
            }

            linepos = linechar;
         }

         return os;
      }

      //.................................................................................
      std::ostream& pnm::SavePPM(std::ostream& os, std::vector<unsigned int>& data, unsigned int ancho, bool binary,
                                 unsigned int maxv) {
         std::vector<unsigned int>::iterator it = data.begin();
         std::vector<unsigned int>::iterator itend;
         unsigned int w = ancho * 3;
         unsigned int h = data.size() / w;

         width = ancho;

         if (data.size() % w > 0) h++;

         height = h;

         itend = it + w;
         if (itend >= data.end()) itend = data.end();
         SavePPM(os, it, itend, binary, height, maxv);
         it += w;

         while (it < data.end()) {
            itend = it + w;
            if (itend >= data.end()) itend = data.end();
            SavePPM(os, it, itend, binary, 0, maxv);
            it += w;
         }

         return os;
      }

      // .............................................................................
      // Name: ReadRaw
      //
      // Synopsis: Read a Raw file
      //
      // Parameters:
      //			std::istream& is                  -----> input stream
      //          sequence & s                      -----> the input as a sequence.
      //          std::vector<sequence> & sv        -----> The input as a vector of sequence, each member a line of the
      //          image.
      //
      // Returns:
      //         the input stream
      //
      // Note:
      //      1) If the three first character are alphanumeric, it is
      //         assumed to be a text file, binary otherwise
      //      2) If input is binary, then sequence is read as a binary stream.
      //      3) If input is a text file, then each character is ofsset by '0'.
      //
      // Exceptions:
      //            BinarySequenceBadAlloc      --------> Memory allocation failed.
      //            PNMUnknownError             --------> Generic error.
      //..............................................................................
      std::istream& pnm::ReadRAW(std::istream& is, sequence& s, bool bin) {
         std::string line;

         try {
            if (bin)
               magick_number = PNM_RAWBIN;
            else
               magick_number = PNM_RAWTXT;

            if (magick_number == PNM_RAWBIN) {  // binary raw data
               s.clear();

               int fpos = is.tellg();
               is.seekg(0, std::ios_base::end);  // rewind to the end of the file
               int fsize = is.tellg();           // size of the file
               fsize = (fsize - 1) * CHARBITS;
               is.seekg(fpos);  // restore the file pointer to the initial position

               ReadBin(is, s, fsize);
            } else if (magick_number == PNM_RAWTXT) {  // text data

               s.clear();
               line.clear();

               // raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {  // skip blank lines
                     std::getline(is, line);
                     line = string_trim(line);  // get rid of initial and trailing white spaces
                  }

                  // for (char c: line) s.push(c - '0');
                  for (char c: line) s.push(c);

                  line.clear();
                  break;
               }  // while is.good()
            }
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }

      //..............................................................................
      std::istream& pnm::ReadRAW(std::istream& is, std::vector<sequence>& s, bool bin) {
         std::string line;
         sequence seq;

         try {
            if (bin)
               magick_number = PNM_RAWBIN;
            else
               magick_number = PNM_RAWTXT;

            if (magick_number == PNM_RAWBIN) {  // binary raw data
               s.clear();

               int fpos = is.tellg();
               is.seekg(0, std::ios_base::end);  // rewind to the end of the file
               int fsize = is.tellg();           // size of the file
               fsize = (fsize - 1) * CHARBITS;
               is.seekg(fpos);  // restore the file pointer to the initial position

               ReadBin(is, seq, fsize);
               s.push_back(seq);
            } else if (magick_number == PNM_RAWTXT) {  // text data
               s.clear();

               seq.clear();
               line.clear();

               // raw data
               while (is.good()) {
                  while (line.size() == 0 && is.good()) {
                     std::getline(is, line);
                     line = string_trim(line);

                     if (line.at(0) == '#') continue;
                  }

                  seq.clear();
                  for (unsigned char c: line) seq.push((unsigned char)(c));

                  line.clear();
                  s.push_back(seq);
               }  // while is.good()
               seq.clear();
            }  // if PNM_RAWTXT
         } catch (SequenceBadAlloc& ba) {
            throw SequenceBadAlloc();
         } catch (...) {
            throw PNMUnknownError();
         }

         return is;
      }

      // .............................................................................
      // Name: ReadPNM
      //
      // Synopsis: Read a file of type PBM, PGM or RAW
      //
      // Parameters:
      //			 std::istream& is              -----> input stream
      //          sequence & s                 -----> the input as a sequence.
      //          std::vector<sequence> & sv   -----> The input as a vector of sequence, each member a line of the
      //          image.
      //
      // Returns:
      //         the input stream
      //
      // Note:
      //      1) The funtion determines the file type
      //
      // Exceptions:
      //            BinarySequenceBadAlloc      --------> Memory allocation failed.
      //            PNMBadFileFormat            --------> Unknown file format or file corrupted.
      //            PNMUnknownError             --------> Generic error.
      //..............................................................................
      std::istream& pnm::ReadPNM(std::istream& is, sequence& s) {
         sequence seq;

         magick_number = FileTypeQ(is);

         switch (magick_number) {
            case PNM_P1:
               ReadPBM(is, seq, false);
               s = seq;
               break;
            case PNM_P4:
               ReadPBM(is, seq, true);
               s = seq;
               break;
            case PNM_P2:
               ReadPGM(is, s, false);
               break;
            case PNM_P5:
               ReadPGM(is, s, true);
               break;
            case PNM_RAWTXT:
               ReadRAW(is, s, false);
               break;
            case PNM_RAWBIN:
               ReadRAW(is, s, true);
               break;
            default:
               throw PNMBadFileFormat();
         }

         return is;
      }
      //........................................................................
      std::istream& pnm::ReadPNM(std::istream& is, std::vector<sequence>& sv) {
         std::vector<sequence> bseq;
         sequence s;

         magick_number = FileTypeQ(is);

         switch (magick_number) {
            case PNM_P1:
               ReadPBM(is, bseq, false);
               for (auto bs: bseq) {
                  s = bs;
                  sv.push_back(s);
               }
               break;
            case PNM_P4:
               ReadPBM(is, bseq, true);
               for (auto bs: bseq) {
                  s = bs;
                  sv.push_back(s);
               }
               break;
            case PNM_P2:
               ReadPGM(is, sv, false);
               break;
            case PNM_P5:
               ReadPGM(is, sv, true);
               break;
            case PNM_RAWTXT:
               ReadRAW(is, sv, false);
               break;
            case PNM_RAWBIN:
               ReadRAW(is, sv, true);
               break;
            default:
               throw PNMBadFileFormat();
         }

         return is;
      }

   }  // namespace utils
}  // namespace lz
