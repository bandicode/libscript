// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/namelookup.h"

#include "script/functionbuilder.h"

TEST(NameLookup, simple_function) {
  using namespace script;

  Engine e;
  e.setup();

  auto fb = FunctionBuilder::Function("foo", Prototype{}, nullptr).setReturnType(Type::Void);
  e.rootNamespace().newFunction(fb);

  NameLookup lookup = NameLookup::resolve("foo", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 1);

  lookup = NameLookup::resolve("bar", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::UnknownName);

  fb = FunctionBuilder::Function("foo", Prototype{}, nullptr).setReturnType(Type::Void).addParam(Type::Int);
  e.rootNamespace().newFunction(fb);

  lookup = NameLookup::resolve("foo", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 2);
}

TEST(NameLookup, variable) {
  using namespace script;

  Engine e;
  e.setup();

  Value n = e.newInt(3);

  ASSERT_FALSE(n.isManaged());
  e.manage(n);
  ASSERT_TRUE(n.isManaged());

  e.rootNamespace().addValue("n", n);

  NameLookup lookup = NameLookup::resolve("n", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);
}

TEST(NameLookup, builtin_types) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve("void", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Void });

  lookup = NameLookup::resolve("bool", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Boolean });

  lookup = NameLookup::resolve("char", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Char });

  lookup = NameLookup::resolve("int", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Int });

  lookup = NameLookup::resolve("float", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Float });

  lookup = NameLookup::resolve("double", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Double });

  lookup = NameLookup::resolve("auto", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TypeName);
  ASSERT_EQ(lookup.typeResult(), Type{ Type::Auto });
}


TEST(NameLookup, nested) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace nested = e.rootNamespace().newNamespace("nested");
  Scope nested_scope{ nested, e.rootNamespace() };

  Value n = e.newInt(3);
  e.manage(n);
  e.rootNamespace().addValue("n", n);

  NameLookup lookup = NameLookup::resolve("n", nested_scope);
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);

  n = e.newInt(4);
  e.manage(n);
  nested.addValue("n", n);

  lookup = NameLookup::resolve("n", nested_scope);
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 4);
}



TEST(NameLookup, array_template) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve("Array", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::TemplateName);
  ASSERT_EQ(lookup.templateResult(), e.getTemplate(Engine::ArrayTemplate));
}

TEST(NameLookup, operators) {
  using namespace script;

  Engine e;
  e.setup();

  NameLookup lookup = NameLookup::resolve(Operator::AssignmentOperator, e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 5);
}

TEST(NameLookup, parsing_operator_name) {
  using namespace script;

  Engine e;
  e.setup();

  // this is less interesting than passing directly the operator name 
  // because it can be ambiguous (that is the case here)
  NameLookup lookup = NameLookup::resolve("operator++", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::FunctionName);
  ASSERT_EQ(lookup.functions().size(), 4);
}

TEST(NameLookup, parsing_nested_name) {
  using namespace script;

  Engine e;
  e.setup();

  Namespace nested = e.rootNamespace().newNamespace("nested");
  Value n = e.newInt(3);
  e.manage(n);
  nested.addValue("n", n);

  NameLookup lookup = NameLookup::resolve("nested::n", e.rootNamespace());
  ASSERT_EQ(lookup.resultType(), NameLookup::VariableName);
  ASSERT_EQ(lookup.variable().toInt(), 3);
}
