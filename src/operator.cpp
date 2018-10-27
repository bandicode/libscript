// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/operator.h"
#include "script/private/operator_p.h"

#include "script/name.h"

namespace script
{

OperatorImpl::OperatorImpl(OperatorName op, Engine *engine, FunctionImpl::flag_type flags)
  : FunctionImpl(engine, flags)
  , operatorId(op)
{

}

Name OperatorImpl::get_name() const
{
  return operatorId;
}


UnaryOperatorImpl::UnaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto.returnType(), proto.at(0))
{

}

const Prototype & UnaryOperatorImpl::prototype() const
{
  return this->proto_;
}

void UnaryOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}

BinaryOperatorImpl::BinaryOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto.returnType(), proto.at(0), proto.at(1))
{

}

const Prototype & BinaryOperatorImpl::prototype() const
{
  return this->proto_;
}

void BinaryOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}

FunctionCallOperatorImpl::FunctionCallOperatorImpl(OperatorName op, const Prototype & proto, Engine *engine, FunctionImpl::flag_type flags)
  : OperatorImpl(op, engine, flags)
  , proto_(proto)
{

}

FunctionCallOperatorImpl::FunctionCallOperatorImpl(OperatorName op, DynamicPrototype && proto, Engine *engine, FunctionImpl::flag_type flags)
  : OperatorImpl(op, engine, flags)
  , proto_(std::move(proto))
{

}

const Prototype & FunctionCallOperatorImpl::prototype() const
{
  return this->proto_;
}

void FunctionCallOperatorImpl::set_return_type(const Type & t)
{
  this->proto_.setReturnType(t);
}



Operator::Operator(const std::shared_ptr<OperatorImpl> & impl)
  : Function(impl)
{

}

OperatorName Operator::operatorId() const
{
  if (d == nullptr)
    return Null;
  return impl()->operatorId;
}

bool Operator::isBinary() const
{
  return d->prototype().count() == 2;
}

bool Operator::isBinary(BuiltInOperator op)
{
  return !isUnary(op) && op != ConditionalOperator
    && op != FunctionCallOperator;
}

bool Operator::isUnary(BuiltInOperator op)
{
  return op == PostIncrementOperator ||
    op == PostDecrementOperator ||
    op == PreIncrementOperator ||
    op == PreDecrementOperator ||
    op == UnaryPlusOperator ||
    op == UnaryMinusOperator ||
    op == LogicalNotOperator ||
    op == BitwiseNot;
}

bool Operator::onlyAsMember(BuiltInOperator op)
{
  return op == AssignmentOperator || op == FunctionCallOperator || op == SubscriptOperator;
}


int Operator::precedence(BuiltInOperator op)
{
  if (op == ScopeResolutionOperator)
    return 1;
  else if (PostIncrementOperator <= op && op <= MemberAccessOperator)
    return 2;
  else if (PreIncrementOperator <= op && op <= BitwiseNot)
    return 3;
  else if (MultiplicationOperator <= op && op <= RemainderOperator)
    return 4;
  else if (AdditionOperator == op || op == SubstractionOperator)
    return 5;
  else if (LeftShiftOperator == op || op == RightShiftOperator)
    return 6;
  else if (LessOperator <= op && op <= GreaterEqualOperator)
    return 7;
  else if (EqualOperator == op || op == InequalOperator)
    return 8;
  else if (op == BitwiseAndOperator)
    return 9;
  else if (op == BitwiseXorOperator)
    return 10;
  else if (op == BitwiseOrOperator)
    return 11;
  else if (op == LogicalAndOperator)
    return 12;
  else if (op == LogicalOrOperator)
    return 13;
  else if (ConditionalOperator <= op && op <= BitwiseXorAssignmentOperator)
    return 14;
  else if (op == CommaOperator)
    return 15;
  return 0;
}

Operator::Associativity Operator::associativity(int group)
{
  static Associativity table[] = {
    LeftToRight,
    LeftToRight,
    RightToLeft,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    LeftToRight,
    RightToLeft,
    LeftToRight,
  };

  if (group > 0 && group <= 15)
    return table[group - 1];

  throw std::runtime_error{ "Operator::associativity() : Invalid group" };
}

Type Operator::firstOperand() const
{
  return d->prototype().at(0);
}

Type Operator::secondOperand() const
{
  return d->prototype().at(1);
}

const std::string & Operator::getSymbol(BuiltInOperator op)
{
  if (op == Null)
    throw std::runtime_error{ "Invalid operator" };

  static const std::string names[] = {
    "",
    "::",
    "++",
    "--",
    "()",
    "[]",
    ".",
    "++",
    "--",
    "+",
    "-",
    "!",
    "~",
    "*",
    "/",
    "%",
    "+",
    "-",
    "<<",
    ">>",
    "<",
    ">",
    "<=",
    ">=",
    "==",
    "!=",
    "&",
     "^",
    "|",
     "&&",
     "||",
     "?:",
     "=",
     "*=",
     "/=",
     "%=",
    "+=",
    "-=",
    "<<=",
    ">>=",
    "&=",
    "|=",
    "^=",
    ","
  };

  return names[static_cast<int>(op)];
}

const std::string & Operator::getFullName(BuiltInOperator op)
{
  if (op == Null)
    throw std::runtime_error{ "Invalid operator" };

  static const std::string names[] = {
    "", "operator::", "operator++", "operator--", "operator()", "operator[]", "operator.",
    "operator++","operator--","operator+","operator-", "operator!","operator~", "operator*",
    "operator/","operator%", "operator+", "operator-", "operator<<", "operator>>", "operator<",
    "operator>","operator<=","operator>=", "operator==",  "operator!=", "operator&", "operator^",
    "operator|","operator&&", "operator||","operator?:", "operator=","operator*=","operator/=",
    "operator%=","operator+=","operator-=","operator<<=","operator>>=", "operator&=",  "operator|=",
    "operator^=", "operator,",
  };

  return names[static_cast<int>(op)];
}

Operator & Operator::operator=(const Operator & other)
{
  d = other.d;
  return *(this);
}

bool Operator::operator==(const Operator & other) const
{
  return d == other.d;
}

bool Operator::operator!=(const Operator & other) const
{
  return d != other.d;
}


std::shared_ptr<OperatorImpl> Operator::impl() const
{
  return std::static_pointer_cast<OperatorImpl>(d);
}

} // namespace script
