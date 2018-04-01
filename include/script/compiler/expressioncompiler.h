// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_COMPILE_EXPRESSION_H
#define LIBSCRIPT_COMPILE_EXPRESSION_H

#include "script/compiler/compiler.h"

#include "script/context.h"
#include "script/conversions.h"

namespace script
{

class NameLookup;
class Template;
struct TemplateArgument;

namespace ast
{
class Expression;
class ArrayExpression;
class ArraySubscript;
class BraceConstruction;
class ConditionalExpression;
class FunctionCall;
class Identifier;
class IntegerLiteral;
class LambdaExpression;
class Literal;
class ListExpression;
class Node;
class Operation;
class UserDefinedLiteral;
class QualifiedType;
} // namespace ast

namespace program
{
class Expression;
class LambdaExpression;
} // namespace program

namespace compiler
{

class AbstractExpressionCompiler : public CompilerComponent
{
public:
  AbstractExpressionCompiler() = delete;
  AbstractExpressionCompiler(Compiler *c, CompileSession *s);

  TemplateArgument generateTemplateArgument(const std::shared_ptr<ast::Node> & arg);
  TemplateArgument generateTemplateArgument(const std::shared_ptr<program::Expression> & e, const std::shared_ptr<ast::Node> &);
  std::vector<TemplateArgument> generateTemplateArguments(const std::vector<std::shared_ptr<ast::Node>> & args);
  bool isConstExpr(const std::shared_ptr<program::Expression> & expr);
  Value evalConstExpr(const std::shared_ptr<program::Expression> & expr);

  virtual Scope currentScope() const = 0;
  virtual NameLookup unqualifiedLookup(const std::shared_ptr<ast::Identifier> & name);

protected:
  NameLookup resolve(const std::shared_ptr<ast::Identifier> & identifier);

  struct OperatorLookupPolicy {
    enum Value { RemoveDuplicates = 1, FetchParentOperators = 2, ConsiderCurrentScope = 4 };
  };
  virtual std::vector<Operator> getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy = OperatorLookupPolicy::FetchParentOperators | OperatorLookupPolicy::RemoveDuplicates | OperatorLookupPolicy::ConsiderCurrentScope);

  virtual std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup) = 0;

  virtual std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op);
  virtual std::shared_ptr<program::Expression> generateCall(const std::shared_ptr<ast::FunctionCall> & call);

  virtual std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr) = 0;

  virtual std::string repr(const std::shared_ptr<ast::Identifier> & id);

  std::shared_ptr<program::Expression> generateExpression(const std::shared_ptr<ast::Expression> & expr);


protected:
  Type resolveFunctionType(const ast::QualifiedType & qt);
  Type resolve(const ast::QualifiedType & qt);

  static std::vector<Operator> & removeDuplicates(std::vector<Operator> & list);
  std::vector<Operator> getScopeOperators(Operator::BuiltInOperator op, const script::Scope & scp, int lookup_policy);
  std::vector<Operator> getBinaryOperators(Operator::BuiltInOperator op, Type a, Type b);
  std::vector<Operator> getUnaryOperators(Operator::BuiltInOperator op, Type a);
  std::vector<Function> getCallOperator(const Type & functor_type);
  std::vector<Function> getLiteralOperators(const std::string & suffix);


  std::vector<std::shared_ptr<program::Expression>> generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions);
  void generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out);

  std::shared_ptr<program::Expression> applyStandardConversion(const std::shared_ptr<program::Expression> & arg, const Type & type, const StandardConversion & conv);
  std::shared_ptr<program::Expression> performListInitialization(const std::shared_ptr<program::Expression> & arg, const Type & type, const std::shared_ptr<ListInitializationSequence> & linit);
  std::shared_ptr<program::Expression> prepareFunctionArgument(const std::shared_ptr<program::Expression> & arg, const Type & type, const ConversionSequence & conv);
  void prepareFunctionArguments(std::vector<std::shared_ptr<program::Expression>> & args, const Prototype & proto, const std::vector<ConversionSequence> & conversions);

  int generateIntegerLiteral(const std::shared_ptr<ast::IntegerLiteral> & l);

  std::shared_ptr<program::Expression> generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr);
  std::shared_ptr<program::Expression> generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc);
  std::shared_ptr<program::Expression> generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr);
  std::shared_ptr<program::Expression> generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as);
  std::shared_ptr<program::Expression> generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args);
  std::shared_ptr<program::Expression> generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl);
  Value generateStringLiteral(const std::shared_ptr<ast::Literal> & l, std::string && str);
  std::shared_ptr<program::Expression> generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation);
  std::shared_ptr<program::Expression> generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce);
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier);
  std::shared_ptr<program::Expression> generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup);
  std::shared_ptr<program::Expression> generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index);

};


class ExpressionCompiler : public AbstractExpressionCompiler
{
public:
  ExpressionCompiler(Compiler *c, CompileSession *s);

  void setScope(const Scope & s);

  std::shared_ptr<program::Expression> compile(const std::shared_ptr<ast::Expression> & expr, const Context & context);

protected:
  NameLookup unqualifiedLookup(const std::shared_ptr<ast::Identifier> & name) override;
  Scope currentScope() const override;
  std::vector<Operator> getOperators(Operator::BuiltInOperator op, Type type, int lookup_policy = OperatorLookupPolicy::FetchParentOperators | OperatorLookupPolicy::RemoveDuplicates | OperatorLookupPolicy::ConsiderCurrentScope)  override;
  std::shared_ptr<program::Expression> generateOperation(const std::shared_ptr<ast::Expression> & op) override;
  std::shared_ptr<program::Expression> generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)  override;
  std::shared_ptr<program::LambdaExpression> generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr) override;

private:
  Context mContext;
  Scope mScope;
};


} // namespace compiler

} // namespace script

#endif // LIBSCRIPT_COMPILE_EXPRESSION_H
