// Copyright (C) 2018-2021 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_P_H
#define LIBSCRIPT_FUNCTION_P_H

#include "script/engine.h"
#include "script/functionflags.h"
#include "script/functiontemplate.h"
#include "script/prototypes.h"

namespace script
{

namespace program
{
class Expression;
class Statement;
}

class Class;
class Name;
class SymbolImpl;

typedef std::shared_ptr<program::Expression> DefaultArgument;

class LIBSCRIPT_API FunctionImpl
{
public:
  FunctionImpl(Engine *e, FunctionFlags f = FunctionFlags{});
  virtual ~FunctionImpl();

  virtual const std::string& name() const;
  virtual Name get_name() const;

  Engine *engine;
  std::weak_ptr<SymbolImpl> enclosing_symbol;
  FunctionFlags flags;
  std::shared_ptr<UserData> data;

  virtual bool is_native() const = 0;
  virtual std::shared_ptr<program::Statement> body() const;
  virtual void set_body(std::shared_ptr<program::Statement> b) = 0;

  virtual const Prototype& prototype() const = 0;
  virtual void set_return_type(const Type& t);

  virtual script::Value invoke(script::FunctionCall* c);

  void force_virtual();

  virtual bool is_template_instance() const;
  virtual bool is_instantiation_completed() const;
  virtual void complete_instantiation();
  
  virtual const std::vector<DefaultArgument> & default_arguments() const;
  virtual void set_default_arguments(std::vector<DefaultArgument> defaults);
  virtual void add_default_argument(const DefaultArgument & da);
};

class RegularFunctionImpl : public FunctionImpl
{
public:
  RegularFunctionImpl(std::string name, const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  RegularFunctionImpl(std::string name, DynamicPrototype p, Engine *e, FunctionFlags f = FunctionFlags{});

  std::string mName;
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;
  std::vector<DefaultArgument> mDefaultArguments;
public:
  const std::string& name() const override;
  Name get_name() const override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;

  const Prototype& prototype() const override;
  void set_return_type(const Type& t) override;

  const std::vector<DefaultArgument>& default_arguments() const override;
  void set_default_arguments(std::vector<DefaultArgument> defaults) override;
  void add_default_argument(const DefaultArgument & da) override;
};

class ScriptFunctionImpl : public FunctionImpl
{
public:
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;

public:
  ScriptFunctionImpl(Engine *e);
  ~ScriptFunctionImpl() = default;

  const std::string& name() const override;
  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
  const Prototype& prototype() const override;
};

class ConstructorImpl : public FunctionImpl
{
public:
  ConstructorImpl(const Prototype& p, Engine *e, FunctionFlags f = FunctionFlags{});
  
  DynamicPrototype prototype_;
  std::shared_ptr<program::Statement> program_;
  std::vector<DefaultArgument> mDefaultArguments;
public:
  
  Class getClass() const;

  const std::string& name() const override;
  Name get_name() const override;

  const Prototype& prototype() const override;

  const std::vector<DefaultArgument>& default_arguments() const override;
  void set_default_arguments(std::vector<DefaultArgument> defaults) override;
  void add_default_argument(const DefaultArgument & da) override;

  bool is_default_ctor() const;
  bool is_copy_ctor() const;
  bool is_move_ctor() const;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
};


class DestructorImpl : public FunctionImpl
{
public:
  DestructorPrototype proto_;
  std::shared_ptr<program::Statement> program_;

public:
  DestructorImpl(const Prototype &p, Engine *e, FunctionFlags f = FunctionFlags{});

  const Prototype& prototype() const override;

  bool is_native() const override;
  std::shared_ptr<program::Statement> body() const override;
  void set_body(std::shared_ptr<program::Statement> b) override;
};


class FunctionTemplateInstance : public RegularFunctionImpl
{
public:
  FunctionTemplateInstance(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const std::string & name, const Prototype &p, Engine *e, FunctionFlags f = FunctionFlags{});
  ~FunctionTemplateInstance() = default;

  FunctionTemplate mTemplate;
  std::vector<TemplateArgument> mArgs;

  static std::shared_ptr<FunctionTemplateInstance> create(const FunctionTemplate & ft, const std::vector<TemplateArgument> & targs, const FunctionBuilder & builder);

  bool is_template_instance() const override;
  bool is_instantiation_completed() const override;
  void complete_instantiation() override;
};

} // namespace script


#endif // LIBSCRIPT_FUNCTION_P_H
