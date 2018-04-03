// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <iostream>

#include <gtest/gtest.h>

#include "script/engine.h"
#include "script/functionbuilder.h"
#include "script/functiontype.h"
#include "script/enumvalue.h"

#include "script/compiler/compiler.h"
#include "script/compiler/compilererrors.h"
#include "script/compiler/functioncompiler.h"
#include "script/compiler/scriptcompiler.h"

#include "script/parser/parser.h"
#include "script/parser/parsererrors.h"

TEST(CompilerErrors, illegal_this) {
  using namespace script;

  const char *source =
    " 3 + this; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::IllegalUseOfThis{}.what());
}

TEST(CompilerErrors, no_destructor) {
  using namespace script;

  const char *source =
    "  class A            "
    "  {                  "
    "    A() = default;   "
    "  };                 "
    "                     "
    "  void foo()         "
    "  {                  "
    "    A a;             "
    "  }                  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ObjectHasNoDestructor{}.what());
}


TEST(CompilerErrors, invalid_use_delegated_ctor) {
  using namespace script;

  const char *source =
    "  class A                   "
    "  {                         "
    "    int n;                  "
    "                            "
    "    A(int a) : n(a) { }     "
    "    A() : A(2), n(0) { }    "
    "  };                        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InvalidUseOfDelegatedConstructor{}.what());
}


TEST(CompilerErrors, not_data_member) {
  using namespace script;

  const char *source =
    "  class A                   "
    "  {                         "
    "    A(int a) : n(a) { }     "
    "  };                        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::NotDataMember{ "n" }.what());
}

TEST(CompilerErrors, initializing_inherited_data_member) {
  using namespace script;

  const char *source =
    "  class A              "
    "  {                    "
    "    int n;             "
    "    A() = default;     "
    "  };                   "
    "                       "
    "  class B : A          "
    "  {                    "
    "    B() : n(0) { }     "
    "  };                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InheritedDataMember{ "n" }.what());
}

TEST(CompilerErrors, multiple_initializers) {
  using namespace script;

  const char *source =
    "  class A                   "
    "  {                         "
    "    int n;                  "
    "    A() : n(0), n(1) { }    "
    "  };                        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::DataMemberAlreadyHasInitializer{ "n" }.what());
}

TEST(CompilerErrors, could_not_find_delegate_ctor) {
  using namespace script;

  const char *source =
    "  class A                   "
    "  {                         "
    "    int n;                  "
    "                            "
    "    A(int a) : n(a) { }     "
    "    A() : A(2,3) { }        "
    "  };                        ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::NoDelegatingConstructorFound{}.what());
}

TEST(CompilerErrors, no_valid_base_ctor) {
  using namespace script;

  const char *source =
    "  class A              "
    "  {                    "
    "    int n;             "
    "    A() = default;     "
    "  };                   "
    "                       "
    "  class B : A          "
    "  {                    "
    "    B() : A(1) { }     "
    "  };                   ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::CouldNotFindValidBaseConstructor{}.what());
}

TEST(CompilerErrors, init_list_first_array_element) {
  using namespace script;

  const char *source =
    " auto a = [{1, 2}, 3];";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InitializerListAsFirstArrayElement{}.what());
}

TEST(CompilerErrors, return_without_value) {
  using namespace script;

  const char *source =
    " int foo() { return; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ReturnStatementWithoutValue{}.what());
}

TEST(CompilerErrors, return_with_value) {
  using namespace script;

  const char *source =
    " void foo() { return 2; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ReturnStatementWithValue{}.what());
}

TEST(CompilerErrors, ref_not_initialized) {
  using namespace script;

  const char *source =
    " int & a;";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ReferencesMustBeInitialized{}.what());
}

TEST(CompilerErrors, enum_not_initialized) {
  using namespace script;

  const char *source =
    " enum A{}; A a;";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::EnumerationsMustBeInitialized{}.what());
}

TEST(CompilerErrors, funvar_not_initialized) {
  using namespace script;

  const char *source =
    " int(int) func; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::FunctionVariablesMustBeInitialized{}.what());
}

TEST(CompilerErrors, not_default_constructible) {
  using namespace script;

  const char *source =
    " class A {}; A a; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::VariableCannotBeDefaultConstructed{ "A" }.what());
}

TEST(CompilerErrors, invalid_param_count_op_overload) {
  using namespace script;

  const char *source =
    " class A {}; int operator+(const A & a, const A & b, const A & c) { return 0; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InvalidParamCountInOperatorOverload("1", "3").what());
}

TEST(CompilerErrors, data_member_auto) {
  using namespace script;

  const char *source =
    " class A { auto x; }; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::DataMemberCannotBeAuto{}.what());
}

TEST(CompilerErrors, missing_static_data_member_init) {
  using namespace script;

  const char *source =
    " class A { static int x; }; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::MissingStaticInitialization{}.what());
}

TEST(CompilerErrors, invalid_base_class) {
  using namespace script;

  const char *source =
    " class A : B {}; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InvalidBaseClass{}.what());
}

TEST(CompilerErrors, invalid_default_arg) {
  using namespace script;

  const char *source =
    " int sum(int a = 0, int b) { return a + b; } ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::InvalidUseOfDefaultArgument{}.what());
}

TEST(CompilerErrors, array_elem_not_convertible) {
  using namespace script;

  const char *source =
    " class A { A() = default; }; auto a = [1, A{}]; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ArrayElementNotConvertible{}.what());
}

TEST(CompilerErrors, invalid_array_subscript) {
  using namespace script;

  const char *source =
    " int a = 5; int b = a[10]; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ArraySubscriptOnNonObject{}.what());
}

TEST(CompilerErrors, too_many_args_in_init_1) {
  using namespace script;

  const char *source =
    " int a{1, 2}; ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::TooManyArgumentInVariableInitialization{}.what());
}

TEST(CompilerErrors, base_no_copy_ctor) {
  using namespace script;

  const char *source =
    "  class A { };                                  "
    "  class B : A { B(const B &) = default; };      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ParentHasNoCopyConstructor{}.what());
}


TEST(CompilerErrors, base_deleted_move_ctor) {
  using namespace script;

  const char *source =
    "  class A { A(A &&) = delete; };           "
    "  class B : A { B(B &&) = default; };      ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::ParentHasDeletedMoveConstructor{}.what());
}

TEST(CompilerErrors, no_valid_literal_operator) {
  using namespace script;

  const char *source =
    "  auto d = 3km;  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::CouldNotFindValidLiteralOperator{}.what());
}

TEST(CompilerErrors, narrowing_conversion) {
  using namespace script;

  const char *source =
    "  int a{3.14};  ";

  Engine engine;
  engine.setup();

  Script s = engine.newScript(SourceFile::fromString(source));
  bool success = s.compile();
  const auto & errors = s.messages();
  ASSERT_FALSE(success);

  ASSERT_EQ(errors.size(), 1);
  //std::cout << errors.front().message() << std::endl;
  ASSERT_EQ(errors.front().content(), compiler::NarrowingConversionInBraceInitialization("double", "int").what());
}
