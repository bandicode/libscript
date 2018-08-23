// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/valueconstructor.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/expressioncompiler.h"

#include "script/class.h"
#include "script/engine.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

Value ValueConstructor::fundamental(Engine *e, const Type & t)
{
  assert(t.isFundamentalType());

  switch (t.baseType().data())
  {
  case Type::Null:
  case Type::Void:
    throw NotImplementedError{ "Could not construct value of type void" };
  case Type::Boolean:
    return e->newBool(false);
  case Type::Char:
    return e->newChar('\0');
  case Type::Int:
    return e->newInt(0);
  case Type::Float:
    return e->newFloat(0.f);
  case Type::Double:
    return e->newDouble(0.);
  default:
    throw NotImplementedError{ "Could not construct value of given fundamental type" };
  }
}

std::shared_ptr<program::Expression> ValueConstructor::fundamental(Engine *e, const Type & t, bool copy)
{
  Value val = fundamental(e, t);
  e->manage(val);

  auto lit = program::Literal::New(val);
  if (copy)
    return program::Copy::New(t, lit);
  return lit;
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & type, std::nullptr_t, diagnostic::pos_t dp)
{
  if (type.isReference() || type.isRefRef())
    throw ReferencesMustBeInitialized{ dp };

  if (type.isFundamentalType())
    return ValueConstructor::fundamental(e, type, true);
  else if (type.isEnumType())
    throw EnumerationsCannotBeDefaultConstructed{ dp };
  else if (type.isFunctionType())
    throw FunctionVariablesMustBeInitialized{ dp };
  else if (type.isObjectType())
  {
    Function default_ctor = e->getClass(type).defaultConstructor();
    if (default_ctor.isNull())
      throw VariableCannotBeDefaultConstructed{ dp, e->getClass(type).name() };
    else if (default_ctor.isDeleted())
      throw ClassHasDeletedDefaultCtor{ dp, e->getClass(type).name() };

    return program::ConstructorCall::New(default_ctor, {});
  }

  throw NotImplementedError{ dp, "ValueConstructor::construct() : cannot default construct value" };
}

std::shared_ptr<program::Expression> ValueConstructor::brace_construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp)
{
  if (args.size() == 0)
    return construct(e, type, nullptr, dp);

  if (!type.isObjectType() && args.size() != 1)
    throw TooManyArgumentInInitialization{ dp };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ dp };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), type, e);
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dp, args.front()->type(), type };

    if (seq.isNarrowing())
      throw NarrowingConversionInBraceInitialization{ dp, args.front()->type(), type };

    return ConversionProcessor::convert(e, args.front(), type, seq);
  }
  else if (type.isObjectType())
  {
    const std::vector<Function> & ctors = e->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(e);
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ dp }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();
    for (std::size_t i(0); i < conversions.size(); ++i)
    {
      const auto & conv = conversions.at(i);
      if (conv.isNarrowing())
        throw NarrowingConversionInBraceInitialization{ dp, args.at(i)->type(), ctor.parameter(i) };
    }

    ConversionProcessor::prepare(e, args, ctor.prototype(), conversions);
    return program::ConstructorCall::New(ctor, std::move(args));
  }
  else
    throw NotImplementedError{ dp, "ValueConstructor::brace_construct() : type not implemented" };
}

std::shared_ptr<program::Expression> ValueConstructor::construct(Engine *e, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args, diagnostic::pos_t dp)
{
  if (args.size() == 0)
    return construct(e, type, nullptr, dp);

  if (!type.isObjectType() && args.size() != 1)
    throw TooManyArgumentInInitialization{ dp };

  if ((type.isReference() || type.isRefRef()) && args.size() != 1)
    throw TooManyArgumentInReferenceInitialization{ dp };

  if (type.isFundamentalType() || type.isEnumType() || type.isFunctionType())
  {
    ConversionSequence seq = ConversionSequence::compute(args.front(), type, e);
    if (seq == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dp, args.front()->type(), type };

    return ConversionProcessor::convert(e, args.front(), type, seq);
  }
  else if (type.isObjectType())
  {
    const std::vector<Function> & ctors = e->getClass(type).constructors();
    OverloadResolution resol = OverloadResolution::New(e);
    if (!resol.process(ctors, args))
      throw CouldNotFindValidConstructor{ dp }; /// TODO add a better diagnostic message

    const Function ctor = resol.selectedOverload();
    const auto & conversions = resol.conversionSequence();
    ConversionProcessor::prepare(e, args, ctor.prototype(), conversions);
    return program::ConstructorCall::New(ctor, std::move(args));
  }
  else
    throw NotImplementedError{ dp, "ValueConstructor::construct() : type not implemented" };
}

std::shared_ptr<program::Expression> ValueConstructor::construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::ConstructorInitialization> & init)
{
  auto args = ec.generateExpressions(init->args);
  return construct(ec.engine(), t, std::move(args), dpos(init));
}

std::shared_ptr<program::Expression> ValueConstructor::construct(ExpressionCompiler & ec, const Type & t, const std::shared_ptr<ast::BraceInitialization> & init)
{
  auto args = ec.generateExpressions(init->args);
  return brace_construct(ec.engine(), t, std::move(args), dpos(init));
}

} // namespace compiler

} // namespace script

