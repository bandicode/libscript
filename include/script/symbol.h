// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_SYMBOL_H
#define LIBSCRIPT_SYMBOL_H

#include "libscriptdefs.h"

namespace script
{

class Class;
class Namespace;
class SymbolImpl;

class LIBSCRIPT_API Symbol
{
public:
  Symbol() = default;
  Symbol(const Symbol &) = default;
  ~Symbol() = default;

  explicit Symbol(const Class & c);
  explicit Symbol(const Namespace & n);

  explicit Symbol(const std::shared_ptr<SymbolImpl> & impl);

  inline bool isNull() const { return d == nullptr; }

  bool isClass() const;
  Class toClass() const;

  bool isNamespace() const;
  Namespace toNamespace() const;

  Symbol & operator=(const Symbol &) = default;

private:
  std::shared_ptr<SymbolImpl> d;
};

} // namespace script


#endif // LIBSCRIPT_SYMBOL_H
