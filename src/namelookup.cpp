// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include "script/namelookup.h"
#include "script/private/namelookup_p.h"

#include "script/datamember.h"
#include "script/engine.h"
#include "script/private/scope_p.h"
#include "script/functiontype.h"
#include "script/staticdatamember.h"
#include "script/templateargumentprocessor.h"
#include "script/typesystem.h"

#include "script/parser/parser.h"

#include "script/program/expression.h"

namespace script
{

class ScopeParentGuard
{
  std::shared_ptr<ScopeImpl> guarded_scope;
  std::shared_ptr<ScopeImpl> parent_value;
public:
  explicit ScopeParentGuard(const Scope & s)
    : guarded_scope(s.impl()),
      parent_value(guarded_scope->parent)
  {

  }

  ~ScopeParentGuard()
  {
    guarded_scope->parent = parent_value;
  }
};


NameLookupImpl::NameLookupImpl()
  : dataMemberIndex(-1)
  , globalIndex(-1)
  , localIndex(-1)
  , captureIndex(-1)
  , templateParameterIndex(-1)
{

}

NameLookupImpl::~NameLookupImpl()
{
}

Class NameLookupImpl::getClassTemplateInstance(const Scope& scp, ClassTemplate& ct, const std::shared_ptr<ast::TemplateIdentifier>& tmplt)
{
  if (this->options.test(NameLookupOptions::IgnoreTemplateArguments))
  {
    return {};
  }

  TemplateArgumentProcessor tnp;
  return tnp.process(scp, ct, tmplt);
}

NameLookupOptions::NameLookupOptions()
  : d(0)
{

}

NameLookupOptions::NameLookupOptions(TemplateInstantiationPolicy tip)
  : d(tip)
{

}

bool NameLookupOptions::test(TemplateInstantiationPolicy flag) const
{
  return d & flag;
}

void NameLookupOptions::set(TemplateInstantiationPolicy flag, bool on)
{
  if (on)
    d |= flag;
  else
    d &= ~flag;
}

NameLookup::NameLookup(const std::shared_ptr<NameLookupImpl> & impl)
  : d(impl)
{

}

const Scope & NameLookup::scope() const
{
  return d->scope;
}

const std::shared_ptr<ast::Identifier> & NameLookup::identifier() const
{
  return d->identifier;
}

NameLookupOptions NameLookup::options() const
{
  return d->options;
}

NameLookup::ResultType NameLookup::resultType() const
{
  if (!d->functions.empty())
    return FunctionName;
  else if (d->dataMemberIndex != -1)
    return DataMemberName;
  else if (!d->staticDataMemberResult.isNull())
    return StaticDataMemberName;
  else if (d->enumeratorResult.isValid())
    return EnumValueName;
  else if (d->globalIndex != -1)
    return GlobalName;
  else if (d->localIndex != -1)
    return LocalName;
  else if (d->captureIndex != -1)
    return CaptureName;
  else if (!d->scopeResult.isNull())
    return NamespaceName;
  else if (!d->classTemplateResult.isNull() || !d->functionTemplateResult.empty())
    return TemplateName;
  else if (!d->typeResult.isNull())
    return TypeName;
  else if (!d->valueResult.isNull())
    return VariableName;
  else if (d->templateParameterIndex != -1)
    return TemplateParameterName;

  return UnknownName;
}


const std::vector<Function> & NameLookup::functions() const
{
  return d->functions;
}

const Type & NameLookup::typeResult() const
{
  return d->typeResult;
}

const Value & NameLookup::variable() const
{
  return d->valueResult;
}

const Template & NameLookup::classTemplateResult() const
{
  return d->classTemplateResult;
}

int NameLookup::captureIndex() const
{
  return d->captureIndex;
}

int NameLookup::dataMemberIndex() const
{
  return d->dataMemberIndex;
}

int NameLookup::globalIndex() const
{
  return d->globalIndex;
}

int NameLookup::localIndex() const
{
  return d->localIndex;
}

int NameLookup::templateParameterIndex() const
{
  return d->templateParameterIndex;
}

const Enumerator & NameLookup::enumeratorResult() const
{
  return d->enumeratorResult;
}

const Scope & NameLookup::scopeResult() const
{
  return d->scopeResult;
}

const Class::StaticDataMember & NameLookup::staticDataMemberResult() const
{
  return d->staticDataMemberResult;
}

const Class & NameLookup::memberOf() const
{
  return d->memberOfResult;
}

NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const Scope & scope)
{
  auto result = std::make_shared<NameLookupImpl>();
  result->identifier = name;
  result->scope = scope;

  NameLookup l{ result };
  l.process();
  return l;
}

static bool need_parse(const std::string & name)
{
  for (const char & c : name)
  {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
    if (!ok)
      return true;
  }

  return false;
}

NameLookup NameLookup::resolve(const std::string & name, const Scope & scope)
{
  if (need_parse(name))
  {
    std::shared_ptr<ast::Identifier> id = script::parser::parseIdentifier(name);
    return NameLookup::resolve(id, scope);
  }

  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;

  static const std::map<std::string, Type::BuiltInType> built_in_types = {
    { "void", Type::Void },
    { "bool", Type::Boolean },
    { "char", Type::Char },
    { "int", Type::Int },
    { "float", Type::Float },
    { "double", Type::Double },
    { "auto", Type::Auto },
  };

  auto it = built_in_types.find(name);
  if (it != built_in_types.end())
  {
    result->typeResult = it->second;
    return NameLookup{ result };
  }

  scope.lookup(name, result);

  return NameLookup{ result };
}

NameLookup NameLookup::resolve(OperatorName op, const Scope & scope)
{
  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = scope;

  result->functions = scope.lookup(op);

  return NameLookup{ result };
}


static Template unqualified_template_lookup(const std::string & name, const Scope & scp)
{
  const auto & tmplts = scp.templates();
  for (const auto & t : tmplts)
  {
    if (t.name() == name)
      return t;
  }

  if (scp.parent().isNull())
    return Template{};

  return unqualified_template_lookup(name, scp.parent());
}

static Template qualified_template_lookup(const std::string & name, const Scope & scp)
{
  const auto & tmplts = scp.templates();
  for (const auto & t : tmplts)
  {
    if (t.name() == name)
      return t;
  }

  return Template{};
}

/// TODO : merge this function with the unqualified overload
Scope NameLookup::qualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope)
{
  assert(!name->is<ast::ScopedIdentifier>());

  if (scope.isNull())
    return Scope{};

  Scope result;

  if (name->is<ast::OperatorName>())
    throw std::runtime_error{ "NameLookup error : an operator cannot be used as a scope" };

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & str = name->as<ast::SimpleIdentifier>().getName();
    result = scope.child(str);
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::dynamic_pointer_cast<ast::TemplateIdentifier>(name);
    Template t = qualified_template_lookup(tempid->getName(), scope);
    ClassTemplate cla_tmplt = t.asClassTemplate();
    if (cla_tmplt.isNull())
      throw std::runtime_error{ "Name does not refer to a template" };

    Class result = d->getClassTemplateInstance(d->scope, cla_tmplt, tempid);
    if (!result.isNull())
      return result;
    return Scope{};
  }

  return result;
}

Scope NameLookup::unqualified_scope_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & scope)
{
  if (scope.isNull())
    return Scope{};

  Scope result;

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    const std::string & str = name->as<ast::SimpleIdentifier>().getName();
    result = scope.child(str);
  }
  else if (name->is<ast::OperatorName>())
    throw std::runtime_error{ "NameLookup error : an operator cannot be used as a scope" };
  else if (name->is<ast::TemplateIdentifier>())
  {
    const auto tempid = std::static_pointer_cast<ast::TemplateIdentifier>(name);
    Template t = unqualified_template_lookup(tempid->getName(), scope);
    ClassTemplate cla_tmplt = t.asClassTemplate();
    if (cla_tmplt.isNull())
      throw std::runtime_error{ "Name does not refer to a template" };

    Class result = d->getClassTemplateInstance(d->scope, cla_tmplt, tempid);
    if (!result.isNull())
      return result;
    return Scope{};
  }
  else if (name->is<ast::ScopedIdentifier>())
  {
    const auto & scpid = name->as<ast::ScopedIdentifier>();
    Scope leftScope = unqualified_scope_lookup(scpid.lhs, scope);
    if(!leftScope.isNull())
      return qualified_scope_lookup(scpid.rhs, leftScope);
  }

  if (!result.isNull())
    return result;

  return unqualified_scope_lookup(name, scope.parent());
}
 
NameLookup NameLookup::resolve(const std::shared_ptr<ast::Identifier> & name, const Scope &scp, NameLookupOptions opts)
{
  auto result = std::make_shared<NameLookupImpl>();
  result->identifier = name;
  result->scope = scp;
  result->options = opts;

  NameLookup l{ result };
  l.process();
  return l;
}

void NameLookup::recursive_member_lookup(NameLookupImpl *result, const std::string & name, const Class & cla)
{
  for (const auto & f : cla.memberFunctions())
  {
    if (f.name() == name)
      result->functions.push_back(f);
  }

  /// TODO : implement using-directive for functions

  for (const auto & ft : cla.templates())
  {
    if (ft.isFunctionTemplate() && ft.name() == name)
      result->functionTemplateResult.push_back(ft.asFunctionTemplate());
  }

  const auto & data_members = cla.dataMembers();
  for (size_t i(0); i < data_members.size(); ++i)
  {
    if (data_members.at(i).name == name)
    {
      result->dataMemberIndex = static_cast<int>(i + cla.attributesOffset());
      return;
    }
  }

  if (!result->functions.empty() || !result->functionTemplateResult.empty())
    return;

  Class base = cla.parent();
  if (base.isNull())
    return;

  recursive_member_lookup(result, name, base);
}

NameLookup NameLookup::member(const std::string & name, const Class & cla)
{
  std::shared_ptr<NameLookupImpl> result = std::make_shared<NameLookupImpl>();
  result->scope = Scope{ cla };
  
  recursive_member_lookup(result.get(), name, cla);

  return NameLookup{ result };
}

static void remove_duplicated_operators(std::vector<Function> & list)
{
  /// TODO : quicksort and remove duplicate
  // note that this is not required for OverloadResolution to work
  // we need to check if that is faster to remove the duplicates or not
}

static void get_scope_operators(std::vector<Function> & list, OperatorName op, const script::Scope & scp)
{
  const auto & candidates = scp.operators();
  for (const auto & c : candidates)
  {
    if (c.operatorId() != op)
      continue;
    list.push_back(c);
  }

  if (list.empty() && !scp.parent().isNull())
  {
    get_scope_operators(list, op, scp.parent());
  }
}

static void get_operators(std::vector<Function> & list, OperatorName op, const Namespace & ns)
{
  if (ns.isNull())
    return;

  const auto & candidates = ns.operators();
  for (const auto & c : candidates)
  {
    if (c.operatorId() != op)
      continue;
    list.push_back(c);
  }
}


static void get_operators(std::vector<Function> & list, OperatorName op, const Class & c)
{
  const auto & candidates = c.operators();
  for (const auto & c : candidates)
  {
    if (c.operatorId() != op)
      continue;
    list.push_back(c);
  }
}

static void resolve_operators(std::vector<Function> &result, OperatorName op, const Class & type)
{
  get_operators(result, op, type);

  /// TODO : optimize for operators that cannot be non-member
  Namespace type_namespace = type.enclosingNamespace();
  get_operators(result, op, type_namespace);

  if (!type.parent().isNull())
    resolve_operators(result, op, type.parent());
}

static void resolve_operators(std::vector<Function> &result, OperatorName op, const Type & type, const Scope & scp)
{
  Engine *engine = scp.engine();
  TypeSystem* ts = engine->typeSystem();

  if (type.isClosureType() || type.isFunctionType())
  {
    // these two don't have a definition scope, so we must process them separatly
    if (type.isFunctionType() && op == AssignmentOperator)
    {
      result.push_back(ts->getFunctionType(type).assignment());
      return;
    }
    else if (type.isClosureType() && op == FunctionCallOperator)
    {
      result.push_back(ts->getLambda(type).function());
      return;
    }

    return;
  }

  if (type.isEnumType() && op == AssignmentOperator)
  {
    result.push_back(ts->getEnum(type).getAssignmentOperator());
    return;
  }

  if (type.isObjectType())
    resolve_operators(result, op, ts->getClass(type));
  else 
  {
    Namespace type_namespace = Scope::enclosingNamespace(type, engine);
    get_operators(result, op, type_namespace);
  }
}

std::vector<Function> NameLookup::resolve(OperatorName op, const Type & type, const Scope & scp)
{
  std::vector<Function> result;

  get_scope_operators(result, op, scp);

  resolve_operators(result, op, type, scp);

  remove_duplicated_operators(result);

  return result;
}

std::vector<Function> NameLookup::resolve(OperatorName op, const Type & lhs, const Type & rhs, const Scope & scp)
{
  /// TODO : this needs some optimization !!
  std::vector<Function> result;

  get_scope_operators(result, op, scp);

  resolve_operators(result, op, lhs, scp);
  resolve_operators(result, op, rhs, scp);

  remove_duplicated_operators(result);

  return result;
}

bool NameLookup::checkBuiltinName()
{
  if (d->identifier->type() != ast::NodeType::SimpleIdentifier)
    return false;

  switch (d->identifier->as<ast::SimpleIdentifier>().name.id)
  {
  case parser::Token::Void:
    d->typeResult = Type::Void;
    return true;
  case parser::Token::Bool:
    d->typeResult = Type::Boolean;
    return true;
  case parser::Token::Char:
    d->typeResult = Type::Char;
    return true;
  case parser::Token::Int:
    d->typeResult = Type::Int;
    return true;
  case parser::Token::Float:
    d->typeResult = Type::Float;
    return true;
  case parser::Token::Double:
    d->typeResult = Type::Double;
    return true;
  case parser::Token::Auto:
    d->typeResult = Type::Auto;
    return true;
  default:
    break;
  }

  return false;
}

void NameLookup::process()
{
  if (checkBuiltinName())
    return;

  auto name = d->identifier;
  auto scope = d->scope;

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    scope.lookup(name->as<ast::SimpleIdentifier>().getName(), d.get());
  }
  else if (name->is<ast::OperatorName>())
  {
    OperatorName op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().symbol, ast::OperatorName::All);
    const auto & ops = scope.lookup(op);
    d->functions.insert(d->functions.end(), ops.begin(), ops.end());
  }
  else if (name->type() == ast::NodeType::QualifiedIdentifier)
  {
    auto qualid = std::dynamic_pointer_cast<ast::ScopedIdentifier>(name);
    Scope scp = unqualified_scope_lookup(qualid->lhs, scope);
    return qualified_lookup(qualid->rhs, scp);
  }
  else if (name->type() == ast::NodeType::TemplateIdentifier)
  {
    auto tempid = std::static_pointer_cast<ast::TemplateIdentifier>(name);
    scope.lookup(tempid->getName(), d.get());
    d->functions.clear(); // we only keep templates

    if (resultType() == NameLookup::TemplateName)
    {
      if (!d->classTemplateResult.isNull())
      {
        Class cla = d->getClassTemplateInstance(d->scope, d->classTemplateResult, tempid);
        if (!cla.isNull())
        {
          d->classTemplateResult = ClassTemplate{};
          d->typeResult = cla.id();
        }
      }
    }
    else
    {
      /// TODO: if the result is not Unknown name, we should throw or at least report an error
    }
  }
}

void NameLookup::qualified_lookup(const std::shared_ptr<ast::Identifier> & name, const Scope & s)
{
  assert(!name->is<ast::ScopedIdentifier>());

  if (s.isNull())
    return;

  ScopeParentGuard guard{ s };
  s.impl()->parent = nullptr; // temporarily setting parent to nullptr

  if (name->type() == ast::NodeType::SimpleIdentifier)
  {
    s.lookup(name->as<ast::SimpleIdentifier>().getName(), d.get());
  }
  else if (name->is<ast::OperatorName>())
  {
    OperatorName op = ast::OperatorName::getOperatorId(name->as<ast::OperatorName>().symbol, ast::OperatorName::All);
    const auto & ops = s.lookup(op);
    d->functions.insert(d->functions.end(), ops.begin(), ops.end());
  }
  else if (name->is<ast::TemplateIdentifier>())
  {
    auto tempid = std::static_pointer_cast<ast::TemplateIdentifier>(name);
    auto fake_template_name = ast::SimpleIdentifier::New(tempid->name);
    qualified_lookup(fake_template_name, s);
    d->functions.clear(); // we only keep templates

    if (resultType() == NameLookup::TemplateName)
    {
      if (!d->classTemplateResult.isNull())
      {
        Class cla = d->getClassTemplateInstance(d->scope, d->classTemplateResult, tempid);
        if (!cla.isNull())
        {
          d->classTemplateResult = ClassTemplate{};
          d->typeResult = cla.id();
        }
      }
    }
    else
    {
      /// TODO: if the result is not Unknown name, we should throw or at least report an error
    }
  }
}

} // namespace script
