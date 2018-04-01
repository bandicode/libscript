// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_ENUM_H
#define LIBSCRIPT_ENUM_H

#include <map>

#include "libscriptdefs.h"

namespace script
{

class EnumImpl;

class Engine;
class Operator;
class Script;

class LIBSCRIPT_API Enum
{
public:
  Enum() = default;
  Enum(const Enum & other) = default;
  ~Enum() = default;

  Enum(const std::shared_ptr<EnumImpl> & impl);

  int id() const;
  bool isNull() const;

  const std::string & name() const;

  bool isEnumClass() const;

  const std::map<std::string, int> & values() const;
  bool hasKey(const std::string & k) const;
  bool hasValue(int val) const;
  int getValue(const std::string & k, int defaultValue = -1) const;
  int addValue(const std::string & key, int value = -1);

  Operator getAssignmentOperator() const;

  Enum & operator=(const Enum & other) = default;
  bool operator==(const Enum & other) const;
  inline bool operator!=(const Enum & other) const { return !operator==(other); }

  Engine * engine() const;
  Script script() const;

  EnumImpl * implementation() const;
  std::weak_ptr<EnumImpl> weakref() const;

private:
  std::shared_ptr<EnumImpl> d;
};

} // namespace script

#endif // LIBSCRIPT_ENUM_H
