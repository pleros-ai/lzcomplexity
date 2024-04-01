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
#define UNKNOWN_ERROR -199

class Errors {
  public:
   int type;
   std::string msg;

   Errors(void) {
      type = ERROR_OK;
      msg = "Some general error.";
   };
   Errors(std::string _msg)
       : msg(_msg) {
      type = ERROR_OK;
   };
};

class OutOfBounds : public Errors {
  public:
   OutOfBounds(void)
       : Errors("Out of bounds") {
      type = OUT_OF_BOUNDS;
   };
};
class BadInitialization : public Errors {
  public:
   BadInitialization(void)
       : Errors("Bad initialization") {
      type = BAD_INITIALIZATION;
   };
};
class NullPointer : public Errors {
  public:
   NullPointer(void)
       : Errors("Null pointer") {
      type = NULL_POINTER;
   };
};
class BadAssignment : public Errors {
  public:
   BadAssignment(void)
       : Errors("Bad assignment") {
      type = BAD_ASSIGNMENT;
   };
};
class BadOperation : public Errors {
  public:
   BadOperation(void)
       : Errors("Bad operation") {
      type = BAD_OPERATION;
   };
};
class BadInternalCondition : public Errors {
  public:
   BadInternalCondition(void)
       : Errors("Bad internal condition") {
      type = BAD_INTERNAL_CONDITION;
   };
};
class BadSize : public Errors {
  public:
   BadSize(void)
       : Errors("Bad size") {
      type = BAD_SIZE;
   };
};
class NotEqualSize : public Errors {
  public:
   NotEqualSize(void)
       : Errors("Nor equal size") {
      type = NOT_EQUAL_SIZE;
   };
};
class OutOfChoices : public Errors {
  public:
   OutOfChoices(void)
       : Errors("Out of choices") {
      type = OUT_OF_CHOICES;
   };
};
class BadAlloc : public Errors {
  public:
   BadAlloc(void)
       : Errors("Bad alloc") {
      type = BAD_ALLOC;
   };
};
class BadMemoryLimits : public Errors {
  public:
   BadMemoryLimits(void)
       : Errors("Bad memory limits") {
      type = MEMORY_MAX_LIMIT;
   };
};
class BadString : public Errors {
  public:
   BadString(void)
       : Errors("Bad string") {
      type = BAD_STRING;
   };
};
class Singular : public Errors {
  public:
   Singular(void)
       : Errors("Singular") {
      type = SINGULAR;
   };
};
class OverUnderFlow : public Errors {
  public:
   OverUnderFlow(void)
       : Errors("Over-Under flow") {
      type = OVER_UNDER_FLOW;
   };
};
class OverFlow : public Errors {
  public:
   OverFlow(void)
       : Errors("Over flow") {
      type = OVER_FLOW;
   };
};
class UnderFlow : public Errors {
  public:
   UnderFlow(void)
       : Errors("Under flow") {
      type = UNDER_FLOW;
   };
};
class OutOfDegree : public Errors {
  public:
   OutOfDegree(void)
       : Errors("Out of degree") {
      type = OUT_OF_BOUNDS;
   };
};
class LossOfData : public Errors {
  public:
   LossOfData(void)
       : Errors("Loss of data") {
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
       : Errors() {
      type = FILE_NAME_ERROR;
   };
   FileNameError(std::string _msg)
       : Errors(_msg) {
      type = FILE_NAME_ERROR;
   };
};
class FileFormatError : public Errors {
  public:
   FileFormatError(void)
       : Errors("File format error") {
      type = FILE_FORMAT_ERROR;
   };
};
class IntervalWithoutPoints : public Errors {
  public:
   IntervalWithoutPoints(void)
       : Errors("Interval without points") {
      type = INTERVAL_WITHOUT_POINTS;
   };
};
class UnknownError : public Errors {
  public:
   UnknownError(void)
       : Errors("Unknow error") {
      type = UNKNOWN_ERROR;
   };
};
