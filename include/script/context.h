// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CONTEXT_H
#define LIBSCRIPT_CONTEXT_H

#include <map>
#include <memory>
#include <string>

#include "script/value.h"

namespace script
{

class Module;
class Scope;
class Script;

class ContextImpl;

class LIBSCRIPT_API Context
{
public:
  Context() = default;
  Context(const Context & other) = default;
  ~Context() = default;

  explicit Context(const std::shared_ptr<ContextImpl> & impl);

  int id() const;
  bool isNull() const;

  Engine * engine() const;

  const std::string & name() const;
  void setName(const std::string & name);

  const std::map<std::string, Value> & vars() const;
  void addVar(const std::string & name, const Value & val);
  bool exists(const std::string & name) const;
  Value get(const std::string & name) const;

  void use(const Module &m);
  void use(const Script &s);
  Scope scope() const;

  void clear();

  inline const std::shared_ptr<ContextImpl> & impl() const { return d; }

private:
  std::shared_ptr<ContextImpl> d;
};

} // namespace script


#endif // LIBSCRIPT_CONTEXT_H
