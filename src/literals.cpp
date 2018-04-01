// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/literals.h"
#include "literals_p.h"

namespace script
{

LiteralOperatorImpl::LiteralOperatorImpl(std::string && suffix, const Prototype & proto, Engine *engine, uint8 flags)
  : FunctionImpl(proto, engine, flags)
  , suffix(std::move(suffix))
{

}


LiteralOperator::LiteralOperator(const std::shared_ptr<LiteralOperatorImpl> & impl)
  : Function(impl)
{

}

Type LiteralOperator::input() const
{
  return d->prototype.argv(0);
}

Type LiteralOperator::output() const
{
  return d->prototype.returnType();
}

const std::string & LiteralOperator::suffix() const
{
  return impl()->suffix;
}

LiteralOperatorImpl * LiteralOperator::impl() const
{
  return dynamic_cast<LiteralOperatorImpl*>(d.get());
}

} // namespace script
