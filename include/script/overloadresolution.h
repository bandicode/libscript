// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_OVERLOAD_RESOLUTION_H
#define LIBSCRIPT_OVERLOAD_RESOLUTION_H

#include "script/engine.h" /// TODO: forward declare
#include "script/function.h"
#include "script/initialization.h"

namespace script
{

namespace diagnostic
{
class DiagnosticMessage;
class MessageBuilder;
} // namespace diagnostic

class Initialization;

struct OverloadResolutionImpl;

// see http://en.cppreference.com/w/cpp/language/overload_resolution
// http://en.cppreference.com/w/cpp/language/implicit_conversion
// http://en.cppreference.com/w/cpp/language/cast_operator

// processORWithoutObject() is for ctor, which we could process with an implicit_object that is a script::Type
// and for when there are no implicit object
// Two functions: one taking an implicit object, the other not, with templates for the implicit objects and the args

class LIBSCRIPT_API OverloadResolution
{
public:
  OverloadResolution();
  OverloadResolution(const OverloadResolution &) = default;
  ~OverloadResolution() = default;

  struct Candidate
  {
    Function function;
    std::vector<Initialization> initializations;

  public:
    Candidate() = default;
    Candidate(const Candidate&) = delete;
    Candidate(Candidate&&) = default;
    ~Candidate() = default;

    Candidate& operator=(Candidate&) = delete;
    Candidate& operator=(Candidate&&) = default;

    inline void set(const Function& f)
    {
      function = f;
      initializations.clear();
    }

    inline void reset()
    {
      function = Function{};
      initializations.clear();
    }

    operator bool() const { return !function.isNull(); }
  };

  // no options for now
  enum Option {
    NoOptions = 0,
  };

  inline bool isNull() const { return d == nullptr; }
  inline bool isValid() const { return !isNull(); }

  void setOption(Option opt, bool on = true);
  bool testOption(Option opt) const;
  int options() const;
  
  bool success() const;
  inline bool failure() const { return !success(); }

  Function selectedOverload() const;
  const std::vector<Initialization> & initializations() const;

  Function ambiguousOverload() const;

  const std::vector<Function> & candidates() const;

  enum ViabilityStatus {
    Viable,
    IncorrectParameterCount,
    CouldNotConvertArgument,
  };

  ViabilityStatus getViabilityStatus(const Function & f, std::vector<Initialization> *conversions = nullptr) const;
  ViabilityStatus getViabilityStatus(int candidate_index, std::vector<Initialization> *conversions = nullptr) const;

  enum InputKind {
    NullInputs = 0,
    TypeInputs,
    ValueInputs,
    ExpressionInputs,
  };

  InputKind inputKind() const;
  size_t inputSize() const;
  const std::vector<Type> & typeInputs() const;
  const std::vector<Value> & valueInputs() const;
  const std::vector<std::shared_ptr<program::Expression>> & expressionInputs() const;

  const std::shared_ptr<program::Expression> & implicit_object() const;

  bool process(const std::vector<Function> & candidates, const std::vector<Type> & types);
  bool process(const std::vector<Function> & candidates, const std::vector<Value> & values);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments);
  bool process(const std::vector<Function> & candidates, const std::vector<std::shared_ptr<program::Expression>> & arguments, const std::shared_ptr<program::Expression> & object);

  bool processConstructors(const std::vector<Function> & candidates, const std::vector<Value> & values);

  enum OverloadComparison {
    FirstIsBetter = 1,
    SecondIsBetter = 2,
    Indistinguishable = 3,
    NotComparable = 4,
  };

  static OverloadComparison compare(const Candidate& a, const Candidate& b);

  /// TODO: is passing an Engine* here absolutely necessary ?
  static OverloadResolution New(Engine *engine, int options = 0);

  static Function select(const std::vector<Function> & candidates, const std::vector<Type> & types);
  static Function select(const std::vector<Function> & candidates, const std::vector<Value> & args);
  static Function selectConstructor(const std::vector<Function> & candidates, const std::vector<Value> & args);

  OverloadResolution & operator=(const OverloadResolution & other) = default;

private:
  std::shared_ptr<OverloadResolutionImpl> d;
};


namespace details
{

inline bool overloadresolution_is_null(const Type& type)
{
  return type.isNull();
}

inline bool overloadresolution_is_null(const Value& value)
{
  return value.isNull();
}

inline bool overloadresolution_is_null(const std::shared_ptr<program::Expression>& expr)
{
  return expr == nullptr;
}

inline Type overloadresolution_get_type(const Type& type)
{
  return type;
}

inline Type overloadresolution_get_type(const Value& value)
{
  return value.type();
}

inline Initialization overloadresolution_initialization(const Type& parameter_type, const Type& argtype, Engine* e)
{
  return Initialization::compute(parameter_type, argtype, e, Initialization::CopyInitialization);
}

inline Initialization overloadresolution_initialization(const Type& parameter_type, const Value& arg, Engine* e)
{
  return Initialization::compute(parameter_type, arg.type(), e, Initialization::CopyInitialization);
}

inline Initialization overloadresolution_initialization(const Type& parameter_type, const std::shared_ptr<program::Expression>& expr, Engine* e)
{
  return Initialization::compute(parameter_type, expr, e);
}

inline void overloadresolution_process_candidate(OverloadResolution::Candidate& current, OverloadResolution::Candidate& selected, OverloadResolution::Candidate& ambiguous)
{
  if (current.function == selected.function || current.function == ambiguous.function)
    return;

  OverloadResolution::OverloadComparison comp = OverloadResolution::compare(current, selected);
  if (comp == OverloadResolution::Indistinguishable || comp == OverloadResolution::NotComparable)
  {
    std::swap(ambiguous, current);
  }
  else if (comp == OverloadResolution::FirstIsBetter)
  {
    std::swap(selected, current);

    if (!ambiguous.function.isNull())
    {
      comp = OverloadResolution::compare(selected, ambiguous);
      if (comp == OverloadResolution::FirstIsBetter)
        ambiguous.reset();
    }
  }
  else if (comp == OverloadResolution::SecondIsBetter)
  {
    if (!ambiguous.function.isNull())
    {
      comp = OverloadResolution::compare(current, ambiguous);
      if (comp == OverloadResolution::FirstIsBetter)
      {
        std::swap(ambiguous, current);
      }
    }
  }
}

} // namespace details

template<typename T>
OverloadResolution::Candidate resolve_overloads(const std::vector<Function>& candidates, const std::vector<T>& args)
{
  OverloadResolution::Candidate current;
  OverloadResolution::Candidate selected;
  OverloadResolution::Candidate ambiguous;

  const size_t argc = args.size();

  for (const auto& func : candidates)
  {
    Engine* engine = func.engine();

    current.set(func);

    if (argc > func.prototype().count() || argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    bool ok = true;
    for (size_t i(0); i < argc; ++i)
    {
      Initialization init = details::overloadresolution_initialization(func.parameter(i), args.at(i), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current.initializations.push_back(init);
    }

    if (!ok)
      continue;

    details::overloadresolution_process_candidate(current, selected, ambiguous);
  }

  if (ambiguous.function.isNull() && !selected.function.isNull())
    return selected;
  return {};
}

template<typename T, typename U>
OverloadResolution::Candidate resolve_overloads(const std::vector<Function>& candidates, const T& implicit_object, const std::vector<U>& args)
{
  if (details::overloadresolution_is_null(implicit_object))
    return resolve_overloads(candidates, args);

  OverloadResolution::Candidate current;
  OverloadResolution::Candidate selected;
  OverloadResolution::Candidate ambiguous;

  const int argc = args.size();

  for (const auto& func : candidates)
  {
    Engine* engine = func.engine();

    current.set(func);
    const int actual_argc = func.hasImplicitObject() ? argc + 1 : argc;

    if (actual_argc > func.prototype().count() || actual_argc + int(func.defaultArguments().size()) < func.prototype().count())
      continue;

    if (func.hasImplicitObject())
    {
      Conversion conv = Conversion::compute(details::overloadresolution_get_type(implicit_object), func.parameter(0), engine);
      if (conv == Conversion::NotConvertible() || conv.firstStandardConversion().isCopy())
        continue;
      current.initializations.push_back(Initialization{ Initialization::DirectInitialization, conv });
    }

    const int parameter_offset = func.hasImplicitObject() ? 1 : 0;

    bool ok = true;
    for (int i(0); i < argc; ++i)
    {
      Initialization init = details::overloadresolution_initialization(func.parameter(i + parameter_offset), args.at(i), engine);
      if (init.kind() == Initialization::InvalidInitialization)
      {
        ok = false;
        break;
      }
      current.initializations.push_back(init);
    }

    if (!ok)
      continue;

    details::overloadresolution_process_candidate(current, selected, ambiguous);
  }

  if (ambiguous.function.isNull() && !selected.function.isNull())
    return selected;
  return {};
}

} // namespace script

#endif // LIBSCRIPT_OVERLOAD_RESOLUTION_H
