// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#ifndef LIBSCRIPT_FUNCTION_TEMPLATE_PROCESSOR_H
#define LIBSCRIPT_FUNCTION_TEMPLATE_PROCESSOR_H

#include "script/functiontemplate.h"

#include "script/diagnosticmessage.h"
#include "script/templateargumentprocessor.h"

namespace script
{

class LIBSCRIPT_API FunctionTemplateProcessor
{
public:
  FunctionTemplateProcessor() = default;
  FunctionTemplateProcessor(const FunctionTemplateProcessor & ) = default;
  ~FunctionTemplateProcessor() = default;

  static void remove_duplicates(std::vector<FunctionTemplate> & list);

  void complete(std::vector<Function> & functions, const std::vector<FunctionTemplate> & fts, const std::vector<TemplateArgument> & args, const std::vector<Type> & types);
  
  Function deduce_substitute(const FunctionTemplate & ft, const std::vector<TemplateArgument> & args, const std::vector<Type> & types);
  void instantiate(Function & f);
 
  diagnostic::DiagnosticMessage emitDiagnostic() const;

  FunctionTemplateProcessor & operator=(const FunctionTemplateProcessor & ) = default;
};

} // namespace script

#endif // LIBSCRIPT_FUNCTION_TEMPLATE_H
