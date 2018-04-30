// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/compiler/expressioncompiler.h"

#include "script/compiler/compilererrors.h"
#include "script/compiler/conversionprocessor.h"
#include "script/compiler/diagnostichelper.h"
#include "script/compiler/lambdacompiler.h"
#include "script/compiler/literalprocessor.h"
#include "script/compiler/valueconstructor.h"

#include "script/ast/ast.h"
#include "script/ast/node.h"

#include "script/program/expression.h"
#include "script/program/statements.h"

#include "script/cast.h"
#include "../engine_p.h"
#include "script/functiontype.h"
#include "script/lambda.h"
#include "script/literals.h"
#include "script/namelookup.h"
#include "../namelookup_p.h"
#include "script/overloadresolution.h"

namespace script
{

namespace compiler
{

std::shared_ptr<program::LambdaExpression> LambdaProcessor::generate(ExpressionCompiler & ec, const std::shared_ptr<ast::LambdaExpression> & le)
{
  throw NotImplementedError{ "Default LambdaProcessor cannot generate lambda expression" };
}

std::shared_ptr<program::Expression> VariableAccessor::data_member(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  return member_access(ec, implicit_object(ec), offset, dpos);
}

std::shared_ptr<program::Expression> VariableAccessor::global_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  throw NotImplementedError{ "Default VariableAccessor does not support access to globals" };
}

std::shared_ptr<program::Expression> VariableAccessor::local_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  throw NotImplementedError{ "Default VariableAccessor does not support access to locals" };
}

std::shared_ptr<program::Expression> VariableAccessor::capture_name(ExpressionCompiler & ec, int offset, const diagnostic::pos_t dpos)
{
  throw NotImplementedError{ "Default VariableAccessor does not support access to captures" };
}

std::shared_ptr<program::Expression> VariableAccessor::member_access(ExpressionCompiler & ec, const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos)
{
  return ec.generateMemberAccess(object, index, dpos);
}

std::shared_ptr<program::Expression> VariableAccessor::implicit_object(ExpressionCompiler & ec) const
{
  return ec.implicit_object();
}



ExpressionCompiler::ExpressionCompiler()
{
  lambda_ = &default_lambda_;
  variable_ = &default_variable_;
}

ExpressionCompiler::ExpressionCompiler(const Scope & scp)
  : scope_(scp)
{
  lambda_ = &default_lambda_;
  variable_ = &default_variable_;
}

std::string ExpressionCompiler::dstr(const Type & t) const
{
  return engine()->typeName(t);
}

Type ExpressionCompiler::resolve(const ast::QualifiedType & qt)
{
  return type_resolver.resolve(qt, scope());
}

std::vector<Function> ExpressionCompiler::getBinaryOperators(Operator::BuiltInOperator op, Type a, Type b)
{
  return NameLookup::resolve(op, a, b, scope(), OperatorLookup::ConsiderCurrentScope | OperatorLookup::FetchParentOperators);
}

std::vector<Function> ExpressionCompiler::getUnaryOperators(Operator::BuiltInOperator op, Type a)
{
  return NameLookup::resolve(op, a, scope(), OperatorLookup::ConsiderCurrentScope | OperatorLookup::FetchParentOperators);
}

std::vector<Function> ExpressionCompiler::getLiteralOperators(const std::string & suffix)
{
  /// TODO : improve this impl
  std::vector<Function> ret;

  auto insert_if = [&ret, &suffix](const LiteralOperator & lop) -> void {
    if (lop.suffix() == suffix)
      ret.push_back(lop);
  };

  Scope s = scope();
  while (!s.isNull())
  {
    const auto & lops = s.literalOperators();
    for (const auto & lop : lops)
      insert_if(lop);
    s = s.parent();
  }

  return ret;
}

std::vector<Function> ExpressionCompiler::getCallOperator(const Type & functor_type)
{
  std::vector<Function> result;

  if (functor_type.isObjectType())
  {
    Class cla = engine()->getClass(functor_type);
    const auto & operators = cla.operators();
    for (const auto & op : operators)
    {
      if (op.operatorId() == Operator::FunctionCallOperator)
        result.push_back(op);
    }

    if (!result.empty())
      return result;

    if (!cla.parent().isNull())
      return getCallOperator(cla.parent().id());
  }
  else if (functor_type.isClosureType())
  {
    Lambda closure = engine()->getLambda(functor_type);
    return { closure.function() };
  }

  return result;
}

std::shared_ptr<program::Expression> ExpressionCompiler::implicit_object() const
{
  /// TODO : is this correct in the body of a Lambda ?
  /// TODO : add correct const-qualification
  if (caller().isNull())
    return nullptr;
  if (caller().isDestructor() || caller().isConstructor())
    return program::StackValue::New(0, Type::ref(caller().memberOf().id()));
  else if(caller().isMemberFunction())
    return program::StackValue::New(1, Type::ref(caller().memberOf().id()));
  return nullptr;
}

std::vector<std::shared_ptr<program::Expression>> ExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & expressions)
{
  std::vector<std::shared_ptr<program::Expression>> ret;
  generateExpressions(expressions, ret);
  return ret;
}

void ExpressionCompiler::generateExpressions(const std::vector<std::shared_ptr<ast::Expression>> & in, std::vector<std::shared_ptr<program::Expression>> & out)
{
  for (const auto & e : in)
    out.push_back(generateExpression(e));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateArrayConstruction(const std::shared_ptr<ast::ArrayExpression> & array_expr)
{
  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(array_expr->elements);

  if (args.size() == 0)
    throw NotImplementedError{ "ExpressionCompiler::generateArrayConstruction() : array of size 0" };

  const Type element_type = args.front()->type().baseType();
  if (element_type == Type::InitializerList)
    throw InitializerListAsFirstArrayElement{};

  std::vector<ConversionSequence> conversions;
  conversions.reserve(args.size());
  for (const auto & arg : args)
  {
    auto conv = ConversionSequence::compute(arg, element_type, engine());
    if (conv == ConversionSequence::NotConvertible())
      throw ArrayElementNotConvertible{};

    conversions.push_back(conv);
  }

  auto array_template = engine()->getTemplate(Engine::ArrayTemplate);
  Class array_class = array_template.getInstance({ TemplateArgument::make(element_type) });

  for (size_t i(0); i < args.size(); ++i)
    args[i] = ConversionProcessor::convert(engine(), args.at(i), element_type, conversions.at(i));

  return program::ArrayExpression::New(array_class.id(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateBraceConstruction(const std::shared_ptr<ast::BraceConstruction> & bc)
{
  using diagnostic::dstr;

  NameLookup lookup = resolve(bc->temporary_type);
  if (lookup.typeResult().isNull())
    throw UnknownTypeInBraceInitialization{ dpos(bc), dstr(bc->temporary_type) };

  /// TODO : refactor this huge duplicate of FunctionCompiler::generateVariableDeclaration()
  const Type & type = lookup.typeResult();

  if (!type.isObjectType() && bc->arguments.size() != 1)
    throw TooManyArgumentInVariableInitialization{ dpos(bc) };

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(bc->arguments);

  return ValueConstructor::brace_construct(engine(), type, std::move(args), dpos(bc));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateConstructorCall(const std::shared_ptr<ast::FunctionCall> & fc, const Type & type, std::vector<std::shared_ptr<program::Expression>> && args)
{
  return ValueConstructor::construct(engine(), type, std::move(args), dpos(fc));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateListExpression(const std::shared_ptr<ast::ListExpression> & list_expr)
{
  auto elements = generateExpressions(list_expr->elements);
  return program::InitializerList::New(std::move(elements));
}


std::shared_ptr<program::Expression> ExpressionCompiler::generateArraySubscript(const std::shared_ptr<ast::ArraySubscript> & as)
{
  std::shared_ptr<program::Expression> obj = generateExpression(as->array);
  std::shared_ptr<program::Expression> index = generateExpression(as->index);

  const Type & objType = obj->type();
  if (!objType.isObjectType())
    throw ArraySubscriptOnNonObject{ dpos(as) };

  const Type & argType = index->type();

  std::vector<Function> candidates = this->getBinaryOperators(Operator::SubscriptOperator, objType, argType);
  if (candidates.empty())
    throw CouldNotFindValidSubscriptOperator{ dpos(as) };

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(candidates, std::vector<Type>{objType, argType}))
    throw CouldNotFindValidSubscriptOperator{ dpos(as) };

  Function selected = resol.selectedOverload();

  std::vector<std::shared_ptr<program::Expression>> args;
  args.push_back(obj);
  args.push_back(index);

  const auto & conversions = resol.conversionSequence();

  ConversionProcessor::prepare(engine(), args, selected.prototype(), conversions);

  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateCall(const std::shared_ptr<ast::FunctionCall> & call)
{
  using diagnostic::dstr;

  std::vector<std::shared_ptr<program::Expression>> args = generateExpressions(call->arguments);
  const auto & callee = call->callee;

  if (callee->is<ast::Identifier>())
  {
    const std::shared_ptr<ast::Identifier> callee_name = std::static_pointer_cast<ast::Identifier>(callee);
    NameLookup lookup = NameLookup::resolve(callee_name, scope());
    
    /// TODO : complete with provided template arguments if any !
    FunctionTemplate::complete(lookup, std::vector<TemplateArgument>{}, args);

    if (lookup.resultType() == NameLookup::FunctionName)
    {
      std::shared_ptr<program::Expression> object = implicit_object();

      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args, object))
        throw CouldNotFindValidMemberFunction{ dpos(call) };

      Function selected = resol.selectedOverload();
      if (selected.isDeleted())
        throw CallToDeletedFunction{ dpos(call) };
      else if (!Accessibility::check(caller(), selected))
        throw InaccessibleMember{dpos(call), dstr(callee_name), dstr(selected.accessibility())};

      if (selected.isTemplateInstance() && (selected.native_callback() == nullptr && selected.program() == nullptr))
      {
        FunctionTemplate ft = selected.instanceOf();
        ft.instantiate(selected);
      }

      if (selected.isMemberFunction() && !selected.isConstructor() && object != nullptr)
        args.insert(args.begin(), object);

      const auto & convs = resol.conversionSequence();
      ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);
      if (selected.isConstructor()) /// TODO : can this happen, shouldn't the lookup returns TypeName in this case ?
        return program::ConstructorCall::New(selected, std::move(args));
      else if (selected.isVirtual() && callee->type() == ast::NodeType::SimpleIdentifier)
        return generateVirtualCall(call, selected, std::move(args));
      return program::FunctionCall::New(selected, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::VariableName || lookup.resultType() == NameLookup::GlobalName
      || lookup.resultType() == NameLookup::DataMemberName || lookup.resultType() == NameLookup::LocalName)
    {
      auto functor = generateVariableAccess(std::dynamic_pointer_cast<ast::Identifier>(callee), lookup);
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::TypeName)
    {
      return generateConstructorCall(call, lookup.typeResult(), std::move(args));
    }

    throw NotImplementedError{ dpos(callee), "ExpressionCompiler : call not implemented" };

  }
  else if (callee->is<ast::Operation>() && callee->as<ast::Operation>().operatorToken == parser::Token::Dot)
  {
    auto member_access = std::dynamic_pointer_cast<ast::Operation>(callee);
    assert(member_access->arg2->is<ast::Identifier>());

    auto object = generateExpression(member_access->arg1);

    const std::shared_ptr<ast::Identifier> callee_name = std::static_pointer_cast<ast::Identifier>(member_access->arg2);
    NameLookup lookup = NameLookup::member(callee_name->getName(), engine()->getClass(object->type()));
    if (lookup.resultType() == NameLookup::DataMemberName)
    {
      auto functor = generateMemberAccess(object, lookup.dataMemberIndex(), dpos(call));
      return generateFunctorCall(call, functor, std::move(args));
    }
    else if (lookup.resultType() == NameLookup::FunctionName)
    {
      args.insert(args.begin(), object);

      OverloadResolution resol = OverloadResolution::New(engine());
      if (!resol.process(lookup.functions(), args))
        throw CouldNotFindValidOverload{ dpos(call) };

      Function selected = resol.selectedOverload();
      
      if (selected.isDeleted())
        throw CallToDeletedFunction{ dpos(call) };
      else if (!Accessibility::check(caller(), selected))
        throw InaccessibleMember{ dpos(call), dstr(callee_name), dstr(selected.accessibility()) };

      const auto & convs = resol.conversionSequence();
      ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);
      assert(!selected.isConstructor()); /// TODO : check that this is not possible
      if (selected.isVirtual() && member_access->arg2->type() == ast::NodeType::SimpleIdentifier)
        return generateVirtualCall(call, selected, std::move(args));
      return program::FunctionCall::New(selected, std::move(args));
    }
    else
      throw std::runtime_error{ "Not implemented" };
  }
  else if (callee->is<ast::Expression>())
  {
    auto functor = generateExpression(std::dynamic_pointer_cast<ast::Expression>(callee));
    return generateFunctorCall(call, functor, std::move(args));
  }
  else
    throw std::runtime_error{ "Invalid callee / implementation error" }; /// TODO : which one

  throw std::runtime_error{ "Not implemented" };
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVirtualCall(const std::shared_ptr<ast::FunctionCall> & call, const Function & f, std::vector<std::shared_ptr<program::Expression>> && args)
{
  assert(f.isVirtual());

  Class c = f.memberOf();
  const auto & vtable = c.vtable();
  auto it = std::find(vtable.begin(), vtable.end(), f);
  if (it == vtable.end())
    throw NotImplementedError{ "Implementation error when calling virtual member" };

  auto object = args.front();
  args.erase(args.begin());
  return program::VirtualCall::New(object, std::distance(vtable.begin(), it), f.returnType(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctorCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  using diagnostic::dstr;

  if (functor->type().isFunctionType())
    return generateFunctionVariableCall(call, functor, std::move(args));
  
  std::vector<Function> functions = getCallOperator(functor->type());
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(functions, args, functor))
    throw CouldNotFindValidCallOperator{ dpos(call) };

  Function selected = resol.selectedOverload();

  if (selected.isDeleted())
    throw CallToDeletedFunction{ dpos(call) };
  else if (!Accessibility::check(caller(), selected))
    throw InaccessibleMember{ dpos(call), "operator()", dstr(selected.accessibility()) };

  assert(selected.isMemberFunction());
  args.insert(args.begin(), functor);
  const auto & convs = resol.conversionSequence();
  ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctionVariableCall(const std::shared_ptr<ast::FunctionCall> & call, const std::shared_ptr<program::Expression> & functor, std::vector<std::shared_ptr<program::Expression>> && args)
{
  auto function_type = engine()->getFunctionType(functor->type());
  const Prototype & proto = function_type.prototype();

  std::vector<ConversionSequence> conversions;
  for (size_t i(0); i < args.size(); ++i)
  {
    const auto & a = args.at(i);
    ConversionSequence conv = ConversionSequence::compute(a, proto.argv(i), engine());
    if (conv == ConversionSequence::NotConvertible())
      throw CouldNotConvert{ dpos(call->arguments.at(i)), dstr(a->type()), dstr(proto.argv(i)) };
    conversions.push_back(conv);
  }

  ConversionProcessor::prepare(engine(), args, proto, conversions);
  return program::FunctionVariableCall::New(functor, proto.returnType(), std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateExpression(const std::shared_ptr<ast::Expression> & expr)
{
  switch (expr->type())
  {
  case ast::NodeType::Operation:
    return generateOperation(std::dynamic_pointer_cast<ast::Operation>(expr));
  case ast::NodeType::SimpleIdentifier:
  case ast::NodeType::QualifiedIdentifier:
  case ast::NodeType::TemplateIdentifier:
    return generateVariableAccess(std::dynamic_pointer_cast<ast::Identifier>(expr));
  case ast::NodeType::FunctionCall:
    return generateCall(std::dynamic_pointer_cast<ast::FunctionCall>(expr));
  case ast::NodeType::BraceConstruction:
    return generateBraceConstruction(std::dynamic_pointer_cast<ast::BraceConstruction>(expr));
  case ast::NodeType::ArraySubscript:
    return generateArraySubscript(std::dynamic_pointer_cast<ast::ArraySubscript>(expr));
  case ast::NodeType::ConditionalExpression:
    return generateConditionalExpression(std::dynamic_pointer_cast<ast::ConditionalExpression>(expr));
  case ast::NodeType::ArrayExpression:
    return generateArrayConstruction(std::dynamic_pointer_cast<ast::ArrayExpression>(expr));
  case ast::NodeType::ListExpression:
    return generateListExpression(std::dynamic_pointer_cast<ast::ListExpression>(expr));
  case ast::NodeType::LambdaExpression:
    return generateLambdaExpression(std::dynamic_pointer_cast<ast::LambdaExpression>(expr));
  case ast::NodeType::BoolLiteral:
  case ast::NodeType::IntegerLiteral:
  case ast::NodeType::FloatingPointLiteral:
  case ast::NodeType::StringLiteral:
  case ast::NodeType::UserDefinedLiteral:
    return generateLiteral(std::dynamic_pointer_cast<ast::Literal>(expr));
  default:
    break;
  }


  throw std::runtime_error{ "Not impelmented" };
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateUserDefinedLiteral(const std::shared_ptr<ast::UserDefinedLiteral> & udl)
{
  std::string str = udl->toString();

  // suffix extraction
  std::string suffix = LiteralProcessor::take_suffix(str);
  Value val = LiteralProcessor::generate(engine(), str);
  engine()->manage(val);
  auto lit = program::Literal::New(val);
  std::vector<std::shared_ptr<program::Expression>> args{ lit };

  const auto & lops = getLiteralOperators(suffix);
  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(lops, args))
    throw CouldNotFindValidLiteralOperator{ dpos(udl) };

  Function selected = resol.selectedOverload();
  const auto & convs = resol.conversionSequence();
  ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);

  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::LambdaExpression> ExpressionCompiler::generateLambdaExpression(const std::shared_ptr<ast::LambdaExpression> & lambda_expr)
{
  return lambda_->generate(*this, lambda_expr);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateLiteral(const std::shared_ptr<ast::Literal> & literalExpr)
{
  if (literalExpr->is<ast::UserDefinedLiteral>())
    return generateUserDefinedLiteral(std::static_pointer_cast<ast::UserDefinedLiteral>(literalExpr));

  Value val = LiteralProcessor::generate(engine(), literalExpr);
  engine()->manage(val);
  return program::Literal::New(val);
}




NameLookup ExpressionCompiler::resolve(const std::shared_ptr<ast::Identifier> & identifier)
{
  /// TODO : pass a true TemplateNameProcessor !!
  return NameLookup::resolve(identifier, scope());
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateOperation(const std::shared_ptr<ast::Expression> & in_op)
{
  auto operation = std::dynamic_pointer_cast<ast::Operation>(in_op);

  if (operation->operatorToken == parser::Token::Dot)
    return generateMemberAccess(operation);
  else if (operation->arg2 == nullptr)
    return generateUnaryOperation(operation);

  return generateBinaryOperation(operation);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateMemberAccess(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->operatorToken == parser::Token::Dot);

  auto object = generateExpression(operation->arg1);

  if (!object->type().isObjectType())
    throw CannotAccessMemberOfNonObject{ dpos(operation) };

  Class cla = engine()->getClass(object->type());
  const int attr_index = cla.attributeIndex(operation->arg2->as<ast::Identifier>().getName());
  if (attr_index == -1)
    throw NoSuchMember{ dpos(operation) };

  return generateMemberAccess(object, attr_index, dpos(operation));
}


std::shared_ptr<program::Expression> ExpressionCompiler::generateBinaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  assert(operation->arg2 != nullptr);
  assert(operation->operatorToken != parser::Token::Dot);

  auto lhs = generateExpression(operation->arg1);
  auto rhs = generateExpression(operation->arg2);

  Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(operation->operatorToken, ast::OperatorName::BuiltInOpResol::InfixOp);

  const std::vector<Function> operators = getBinaryOperators(op, lhs->type(), rhs->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{lhs->type(), rhs->type()}))
    throw CouldNotFindValidOperator{ dpos(operation) };

  Operator selected = resol.selectedOverload().toOperator();
  const auto & convs = resol.conversionSequence();
  std::vector<std::shared_ptr<program::Expression>> args{ lhs, rhs };
  ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateUnaryOperation(const std::shared_ptr<ast::Operation> & operation)
{
  using diagnostic::dstr;

  assert(operation->arg2 == nullptr);

  auto operand = generateExpression(operation->arg1);

  const bool postfix = operation->arg1->pos().pos < operation->operatorToken.pos;
  const auto opts = postfix ? ast::OperatorName::BuiltInOpResol::PostFixOp : ast::OperatorName::BuiltInOpResol::PrefixOp;
  Operator::BuiltInOperator op = ast::OperatorName::getOperatorId(operation->operatorToken, opts);

  const std::vector<Function> operators = getUnaryOperators(op, operand->type());

  OverloadResolution resol = OverloadResolution::New(engine());
  if (!resol.process(operators, std::vector<Type>{operand->type()}))
    throw CouldNotFindValidOperator{ dpos(operation) };

  Operator selected = resol.selectedOverload().toOperator();

  if (selected.isDeleted())
    throw CallToDeletedFunction{ dpos(operation) };
  else if (!Accessibility::check(caller(), selected))
    throw InaccessibleMember{ dpos(operation), Operator::getFullName(selected.operatorId()), dstr(selected.accessibility()) };

  const auto & convs = resol.conversionSequence();
  std::vector<std::shared_ptr<program::Expression>> args{ operand };
  ConversionProcessor::prepare(engine(), args, selected.prototype(), convs);
  return program::FunctionCall::New(selected, std::move(args));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateConditionalExpression(const std::shared_ptr<ast::ConditionalExpression> & ce)
{
  auto tru = generateExpression(ce->onTrue);
  auto fal = generateExpression(ce->onFalse);
  auto con = generateExpression(ce->condition);
  return program::ConditionalExpression::New(con, tru, fal);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier)
{
  return generateVariableAccess(identifier, resolve(identifier));
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateVariableAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  switch (lookup.resultType())
  {
  case NameLookup::FunctionName:
    return generateFunctionAccess(identifier, lookup);
  case NameLookup::TemplateName:
    throw TemplateNamesAreNotExpressions{ dpos(identifier) };
  case NameLookup::TypeName:
    throw TypeNameInExpression{ dpos(identifier) };
  case NameLookup::VariableName:
    return program::Literal::New(lookup.variable()); // perhaps a VariableAccess would be better
  case NameLookup::StaticDataMemberName:
    return generateStaticDataMemberAccess(identifier, lookup);
  case NameLookup::DataMemberName:
    return variable_->data_member(*this, lookup.dataMemberIndex(), dpos(identifier));
  case NameLookup::GlobalName:
    return variable_->global_name(*this, lookup.globalIndex(), dpos(identifier));
  case NameLookup::LocalName:
    return variable_->local_name(*this, lookup.localIndex(), dpos(identifier));
  case NameLookup::CaptureName:
    return variable_->capture_name(*this, lookup.captureIndex(), dpos(identifier));
  case NameLookup::EnumValueName:
    return program::Literal::New(Value::fromEnumValue(lookup.enumValueResult()));
  case NameLookup::NamespaceName:
    throw NamespaceNameInExpression{ dpos(identifier) };
  default:
    break;
  }

  throw NotImplementedError{ dpos(identifier), "ExpressionCompiler::generateVariableAccess() : kind of variable not implemented" };
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateFunctionAccess(const std::shared_ptr<ast::Identifier> & identifier, const NameLookup & lookup)
{
  if (lookup.functions().size() != 1)
    throw AmbiguousFunctionName{ dpos(identifier) };

  Function f = lookup.functions().front();
  FunctionType ft = engine()->getFunctionType(f.prototype());
  Value val = Value::fromFunction(f, ft.type());
  engine()->manage(val);
  return program::Literal::New(val); /// TODO : perhaps a program::VariableAccess would be better ?
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateMemberAccess(const std::shared_ptr<program::Expression> & object, const int index, const diagnostic::pos_t dpos)
{
  using diagnostic::dstr;

  Class cla = engine()->getClass(object->type());
  int relative_index = index;
  while (relative_index - int(cla.dataMembers().size()) >= 0)
  {
    relative_index = relative_index - cla.dataMembers().size();
    cla = cla.parent();
  }

  const auto & dm = cla.dataMembers().at(relative_index);

  if (!Accessibility::check(caller(), cla, dm.accessibility()))
    throw InaccessibleMember{ dpos, dm.name, dstr(dm.accessibility()) };

  const Type access_type = object->type().isConst() ? Type::cref(dm.type) : Type::ref(dm.type);
  return program::MemberAccess::New(access_type, object, index);
}

std::shared_ptr<program::Expression> ExpressionCompiler::generateStaticDataMemberAccess(const std::shared_ptr<ast::Identifier> & id, const NameLookup & lookup)
{
  using diagnostic::dstr;

  const Class c = lookup.memberOf();
  const Class::StaticDataMember & sdm = lookup.staticDataMemberResult();

  if (!Accessibility::check(caller(), c, sdm.accessibility()))
    throw InaccessibleMember{ dpos(id), sdm.name, dstr(sdm.accessibility()) };

  return program::Literal::New(sdm.value);
}

} // namespace compiler

} // namespace script

