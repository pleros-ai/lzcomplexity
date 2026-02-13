/***************************************************************************
                          myexception.h  -  description
                             -------------------
    begin                : Fri Nov 28 2016
    copyright            : (C) 2016 by Ernesto Estevez Rams
    email                : estevez@imre.uh.cu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <string>
#include <unordered_map>

#define ERROR_OK -100
#define OUT_OF_BOUNDS -102
#define BAD_INITIALIZATION -103
#define NULL_POINTER -104
#define BAD_ASSIGNMENT -105
#define BAD_INTERNAL_CONDITION -106
#define BAD_SIZE -107
#define BAD_OPERATION -108
#define NOT_EQUAL_SIZE -109
#define OUT_OF_CHOICES -110
#define BAD_ALLOC -111
#define SINGULAR -112
#define OVER_UNDER_FLOW -113
#define LOSS_OF_DATA -114
#define CONFIG_FILE_ERROR -115
#define NORM_FILE_ERROR -116
#define FILE_NAME_ERROR -117
#define FILE_FORMAT_ERROR -118
#define INTERVAL_WITHOUT_POINTS -119
#define OVER_FLOW -120
#define UNDER_FLOW -121
#define BAD_STRING -122
#define MEMORY_MAX_LIMIT -123
#define OUT_OF_DEGREE -124
#define UNKNOWN_ERROR -199
// App personal execptions
#define LZOK 0
#define NOFILE_ERROR -3
#define SIZE_NOMATCH_ERROR -4
#define LZ_SUFFIXARRAY_ERROR -5
#define LZ_OUTOFBOUNDS -6
#define BADCOMMANDLINE -7
#define BADFILE_ERROR -8
#define NONORMALIZATIONFILE_ERROR -9
#define NOCONFIGURATIONFILE_ERROR -10
#define BADFORMAT -11
#define FIRSTPERFORMANSATZFIT -12
#define FILENAME_ERROR -13
#define LZ_MISSINGINPUTFILE -14
#define LZ_FEWCOMMANDLINE -15
#define LZ_OPENFILEERR -16
#define LZ_ERROR -200
#define LZ_BAD_ALLOC -201
#define LZ_NO_MATCH_SIZE -202
#define LZ_OUT_OF_BOUNDS -203
#define LZ_SUFFIX_ARRAY_ERROR -204
#define LZ_ANSATZ_FIT_ERROR -205
#define COMPARISON_SIZE -206
#define WHILERND -207
#define WHILEBRUIJN -208
#define UNKNOWN_STATUS -209
#define LZ_MISSINGOUTPUTFILE -210
#define ALPHABETSIZEMISSING -211
#define LOGBASE_ERROR -212
#define ARGINPUTYPE -213
#define ARGKSEQ -214
#define MISSARG -215
#define STARTIRRELEVANT -216
#define KSEQIRRELEVANT -217
#define UKNOWNOPT -218
#define EXCESSENTROPYLINEMISSING -219
#define EXCESSENTROPYMAXMISSING -220

using emap = std::unordered_map<int, std::string>;

inline const emap error_msg{{ERROR_OK, "Abstract error."},
                            {OUT_OF_BOUNDS, "Out of bounds."},
                            {OUT_OF_DEGREE, "Out of degree."},
                            {BAD_INITIALIZATION, "Bad initialization."},
                            {NULL_POINTER, "Null pointer."},
                            {BAD_ASSIGNMENT, "Bad assignment."},
                            {BAD_INTERNAL_CONDITION, "Bad internal condition."},
                            {BAD_SIZE, "Bad size."},
                            {BAD_OPERATION, "Bad operation."},
                            {OUT_OF_CHOICES, "Out of choices."},
                            {BAD_ALLOC, "Bad alloc."},
                            {MEMORY_MAX_LIMIT, "Bad memory limits."},
                            {BAD_STRING, "Bad string."},
                            {SINGULAR, "Singular."},
                            {OVER_UNDER_FLOW, "Over-Under flow."},
                            {OVER_FLOW, "Over flow."},
                            {UNDER_FLOW, "Under flow."},
                            {LOSS_OF_DATA, "Loss of data."},
                            {FILE_FORMAT_ERROR, "Wrong file format."},
                            {FILE_NAME_ERROR, "Wrong file name."},
                            {INTERVAL_WITHOUT_POINTS, "Interval without points."},
                            {UNKNOWN_ERROR, "Internal error: Unknown error."},
                            {LZ_SUFFIX_ARRAY_ERROR, "Internal error: Error while building suffix array."},
                            {NOT_EQUAL_SIZE, "Nor equal size."}};

class Errors {
  public:
  int         type;
  std::string id;
  std::string msg;

  Errors(void) {
    type = ERROR_OK;
    id = "Error";
    msg = error_msg.at(ERROR_OK);
  };
  Errors(std::string _msg)
    : msg(_msg) {
    type = ERROR_OK;
    id = "Error";
  };
  Errors(std::string _id, std::string _msg)
    : id(_id), msg(_msg) {
    type = ERROR_OK;
  };
};

class OutOfBounds : public Errors {
  public:
  OutOfBounds(void)
    : Errors(error_msg.at(OUT_OF_BOUNDS)) {
    type = OUT_OF_BOUNDS;
  };
};
class BadInitialization : public Errors {
  public:
  BadInitialization(void)
    : Errors(error_msg.at(BAD_INITIALIZATION)) {
    type = BAD_INITIALIZATION;
  };
  BadInitialization(std::string msg)
    : Errors(msg) {
    type = BAD_INITIALIZATION;
  };
};
class NullPointer : public Errors {
  public:
  NullPointer(void)
    : Errors(error_msg.at(NULL_POINTER)) {
    type = NULL_POINTER;
  };
};
class BadAssignment : public Errors {
  public:
  BadAssignment(void)
    : Errors(error_msg.at(BAD_ASSIGNMENT)) {
    type = BAD_ASSIGNMENT;
  };
};
class BadOperation : public Errors {
  public:
  BadOperation(void)
    : Errors(error_msg.at(BAD_OPERATION)) {
    type = BAD_OPERATION;
  };
};
class BadInternalCondition : public Errors {
  public:
  BadInternalCondition(void)
    : Errors(error_msg.at(BAD_INTERNAL_CONDITION)) {
    type = BAD_INTERNAL_CONDITION;
  };
};
class BadSize : public Errors {
  public:
  BadSize(void)
    : Errors(error_msg.at(BAD_SIZE)) {
    type = BAD_SIZE;
  };
};
class NotEqualSize : public Errors {
  public:
  NotEqualSize(void)
    : Errors(error_msg.at(NOT_EQUAL_SIZE)) {
    type = NOT_EQUAL_SIZE;
  };
};
class OutOfChoices : public Errors {
  public:
  OutOfChoices(void)
    : Errors(error_msg.at(OUT_OF_CHOICES)) {
    type = OUT_OF_CHOICES;
  };
};
class BadAlloc : public Errors {
  public:
  BadAlloc(void)
    : Errors(error_msg.at(BAD_ALLOC)) {
    type = BAD_ALLOC;
  };
};
class BadMemoryLimits : public Errors {
  public:
  BadMemoryLimits(void)
    : Errors(error_msg.at(MEMORY_MAX_LIMIT)) {
    type = MEMORY_MAX_LIMIT;
  };
};
class BadString : public Errors {
  public:
  BadString(void)
    : Errors(error_msg.at(BAD_STRING)) {
    type = BAD_STRING;
  };
};
class Singular : public Errors {
  public:
  Singular(void)
    : Errors(error_msg.at(SINGULAR)) {
    type = SINGULAR;
  };
};
class OverUnderFlow : public Errors {
  public:
  OverUnderFlow(void)
    : Errors(error_msg.at(OVER_UNDER_FLOW)) {
    type = OVER_UNDER_FLOW;
  };
};
class OverFlow : public Errors {
  public:
  OverFlow(void)
    : Errors(error_msg.at(OVER_FLOW)) {
    type = OVER_FLOW;
  };
};
class UnderFlow : public Errors {
  public:
  UnderFlow(void)
    : Errors(error_msg.at(UNDER_FLOW)) {
    type = UNDER_FLOW;
  };
};
class OutOfDegree : public Errors {
  public:
  OutOfDegree(void)
    : Errors(error_msg.at(OUT_OF_DEGREE)) {
    type = OUT_OF_DEGREE;
  };
};
class LossOfData : public Errors {
  public:
  LossOfData(void)
    : Errors(error_msg.at(LOSS_OF_DATA)) {
    type = LOSS_OF_DATA;
  };
};
class ConfigFileError : public Errors {
  public:
  ConfigFileError(void)
    : Errors("Config file error") {
    type = CONFIG_FILE_ERROR;
  };
};
class NormFileError : public Errors {
  public:
  NormFileError(void)
    : Errors("Norm file error") {
    type = NORM_FILE_ERROR;
  };
};
class FileNameError : public Errors {
  public:
  FileNameError(void)
    : Errors(error_msg.at(FILE_NAME_ERROR)) {
    type = FILE_NAME_ERROR;
  };
  FileNameError(std::string _msg)
    : Errors(_msg) {
    type = FILE_NAME_ERROR;
  };
};
class BadCmdOptions : public Errors {
  public:
  BadCmdOptions(void)
    : Errors() {
    type = BAD_OPERATION;
  };
  BadCmdOptions(std::string _msg)
    : Errors(_msg) {
    type = BAD_OPERATION;
  };
};
class FileFormatError : public Errors {
  public:
  FileFormatError(void)
    : Errors(error_msg.at(FILE_FORMAT_ERROR)) {
    type = FILE_FORMAT_ERROR;
  };
  FileFormatError(std::string _msg)
    : Errors(_msg) {
    type = FILE_FORMAT_ERROR;
  };
};
class IntervalWithoutPoints : public Errors {
  public:
  IntervalWithoutPoints(void)
    : Errors(error_msg.at(INTERVAL_WITHOUT_POINTS)) {
    type = INTERVAL_WITHOUT_POINTS;
  };
};
class UnknownError : public Errors {
  public:
  UnknownError(void)
    : Errors(error_msg.at(UNKNOWN_ERROR)) {
    type = UNKNOWN_ERROR;
  };
};

namespace lz {
  class LZError : public Errors {
public:
    int lztype;
    LZError(void)
      : Errors() {
      lztype = LZ_ERROR;
    };
    LZError(std::string msg)
      : Errors(msg) {
      lztype = LZ_ERROR;
    }
  };
  class LZBadAlloc : public BadAlloc, public LZError {
public:
    LZBadAlloc(void)
      : BadAlloc(), LZError() {
      lztype = LZ_BAD_ALLOC;
    };
  };
  class LZNoMatchSize : public NotEqualSize, public LZError {
public:
    LZNoMatchSize(void)
      : NotEqualSize(), LZError() {
      lztype = LZ_NO_MATCH_SIZE;
    };
  };
  class LZOutOfBounds : public OutOfBounds, public LZError {
public:
    LZOutOfBounds(void)
      : OutOfBounds(), LZError() {
      lztype = LZ_OUT_OF_BOUNDS;
    };
  };
  class LZSuffixArrayError : public LZError {
public:
    LZSuffixArrayError(void)
      : LZError(error_msg.at(LZ_SUFFIX_ARRAY_ERROR)) {
      lztype = LZ_SUFFIX_ARRAY_ERROR;
    };
  };
  class LZAnsatzFitError : public LZError {
public:
    LZAnsatzFitError(void)
      : LZError() {
      lztype = LZ_ANSATZ_FIT_ERROR;
    };
  };
}  // namespace lz
