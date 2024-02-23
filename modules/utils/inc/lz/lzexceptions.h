/***************************************************************************
                          lzexceptions.h  -  description
                             -------------------
    begin                : Fri Nov 28 2003
    copyright            : (C) 2003 by Ernesto Estevez Rams
    email                : estevez@imre.oc.uh.cu
 ***************************************************************************/

 /***************************************************************************
  *   Copyright (C) 2013 by Ernesto Estevez Rams   						   *
  *   estevez@imre.oc.uh.cu   											   *
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

#include <unordered_map>
#include "myexceptions.h"

#define LZOK                           0
#define NOFILE_ERROR                  -3
#define SIZE_NOMATCH_ERROR            -4
#define LZ_SUFFIXARRAY_ERROR             -5
#define LZ_OUTOFBOUNDS                   -6
#define BADCOMMANDLINE                -7
#define BADFILE_ERROR                 -8
#define NONORMALIZATIONFILE_ERROR     -9
#define NOCONFIGURATIONFILE_ERROR     -10
#define BADFORMAT                     -11
#define FIRSTPERFORMANSATZFIT         -12
#define FILENAME_ERROR                -13
#define LZ_MISSINGINPUTFILE           -14
#define LZ_FEWCOMMANDLINE             -15
#define LZ_OPENFILEERR                -16
#define LZ_ERROR       		 	      -200
#define LZ_BAD_ALLOC   		 	      -201
#define LZ_NO_MATCH_SIZE     	      -202
#define LZ_OUT_OF_BOUNDS     	      -203
#define LZ_SUFFIX_ARRAY_ERROR         -204
#define LZ_ANSATZ_FIT_ERROR 	      -205
#define COMPARISON_SIZE 	          -206
#define WHILERND                      -207
#define WHILEBRUIJN                   -208
#define UNKNOWN_STATUS                -209
#define LZ_MISSINGOUTPUTFILE          -210
#define ALPHABETSIZEMISSING           -211
#define LOGBASE_ERROR                 -212
#define ARGINPUTYPE                   -213
#define ARGKSEQ                       -214
#define MISSARG                       -215
#define STARTIRRELEVANT               -216
#define KSEQIRRELEVANT                -217
#define UKNOWNOPT                     -218
#define EXCESSENTROPYLINEMISSING      -219
#define EXCESSENTROPYMAXMISSING       -220

typedef std::unordered_map<int, std::string> emap;

namespace lz {
   class LZError : public Errors { public: int lztype; LZError(void) :Errors() { lztype = LZ_ERROR; }; };
   class LZBadAlloc : public BadAlloc, public LZError { public: LZBadAlloc(void) : BadAlloc(), LZError() { lztype = LZ_BAD_ALLOC; }; };
   class LZNoMatchSize : public NotEqualSize, public LZError { public: LZNoMatchSize(void) :NotEqualSize(), LZError() { lztype = LZ_NO_MATCH_SIZE; }; };
   class LZOutOfBounds : public OutOfBounds, public LZError { public: LZOutOfBounds(void) : OutOfBounds(), LZError() { lztype = LZ_OUT_OF_BOUNDS; }; };
   class LZSuffixArrayError : public LZError { public: LZSuffixArrayError(void) : LZError() { lztype = LZ_SUFFIX_ARRAY_ERROR; }; };
   class LZAnsatzFitError : public LZError { public: LZAnsatzFitError(void) : LZError() { lztype = LZ_ANSATZ_FIT_ERROR; }; };
}