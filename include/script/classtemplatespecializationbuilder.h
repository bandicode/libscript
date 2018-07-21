// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_CLASS_TEMPLATE_SPECIALIZATION_BUILDER_H
#define LIBSCRIPT_CLASS_TEMPLATE_SPECIALIZATION_BUILDER_H

#include "script/classbuilder.h"

#include "script/classtemplate.h"
#include "script/templateargument.h"

namespace script
{

class LIBSCRIPT_API ClassTemplateSpecializationBuilder : public ClassBuilderBase<ClassTemplateSpecializationBuilder>
{
private:
  typedef ClassBuilderBase<ClassTemplateSpecializationBuilder> Base;
  ClassTemplate template_;
  std::vector<TemplateArgument> arguments_;

public:
  ClassTemplateSpecializationBuilder(const ClassTemplate & t, std::vector<TemplateArgument> && targs);

  inline const ClassTemplate & getTemplate() const { return template_; }
  inline const std::vector<TemplateArgument> & arguments() const { return arguments_; }

  Class get();
};

} // namespace script

#endif // LIBSCRIPT_CLASS_TEMPLATE_SPECIALIZATION_BUILDER_H
