/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionDAGChecker.h"

#include "cmAlgorithms.h"
#include "cmGeneratorExpressionContext.h"
#include "cmGeneratorExpressionEvaluator.h"
#include "cmGeneratorTarget.h"
#include "cmLocalGenerator.h"
#include "cmake.h"

#include <sstream>
#include <string.h>
#include <utility>

cmGeneratorExpressionDAGChecker::cmGeneratorExpressionDAGChecker(
  const cmListFileBacktrace& backtrace, const std::string& target,
  const std::string& property, const GeneratorExpressionContent* content,
  cmGeneratorExpressionDAGChecker* parent)
  : Parent(parent)
  , Target(target)
  , Property(property)
  , Content(content)
  , Backtrace(backtrace)
  , TransitivePropertiesOnly(false)
{
  Initialize();
}

cmGeneratorExpressionDAGChecker::cmGeneratorExpressionDAGChecker(
  const std::string& target, const std::string& property,
  const GeneratorExpressionContent* content,
  cmGeneratorExpressionDAGChecker* parent)
  : Parent(parent)
  , Target(target)
  , Property(property)
  , Content(content)
  , Backtrace()
  , TransitivePropertiesOnly(false)
{
  Initialize();
}

void cmGeneratorExpressionDAGChecker::Initialize()
{
  const cmGeneratorExpressionDAGChecker* top = this;
  const cmGeneratorExpressionDAGChecker* p = this->Parent;
  while (p) {
    top = p;
    p = p->Parent;
  }
  this->CheckResult = this->CheckGraph();

#define TEST_TRANSITIVE_PROPERTY_METHOD(METHOD) top->METHOD() ||

  if (CheckResult == DAG &&
      (CM_FOR_EACH_TRANSITIVE_PROPERTY_METHOD(
        TEST_TRANSITIVE_PROPERTY_METHOD) false)) // NOLINT(clang-tidy)
#undef TEST_TRANSITIVE_PROPERTY_METHOD
  {
    std::map<std::string, std::set<std::string> >::const_iterator it =
      top->Seen.find(this->Target);
    if (it != top->Seen.end()) {
      const std::set<std::string>& propSet = it->second;
      if (propSet.find(this->Property) != propSet.end()) {
        this->CheckResult = ALREADY_SEEN;
        return;
      }
    }
    const_cast<cmGeneratorExpressionDAGChecker*>(top)
      ->Seen[this->Target]
      .insert(this->Property);
  }
}

cmGeneratorExpressionDAGChecker::Result
cmGeneratorExpressionDAGChecker::Check() const
{
  return this->CheckResult;
}

void cmGeneratorExpressionDAGChecker::ReportError(
  cmGeneratorExpressionContext* context, const std::string& expr)
{
  if (this->CheckResult == DAG) {
    return;
  }

  context->HadError = true;
  if (context->Quiet) {
    return;
  }

  const cmGeneratorExpressionDAGChecker* parent = this->Parent;

  if (parent && !parent->Parent) {
    std::ostringstream e;
    e << "Error evaluating generator expression:\n"
      << "  " << expr << "\n"
      << "Self reference on target \"" << context->HeadTarget->GetName()
      << "\".\n";
    context->LG->GetCMakeInstance()->IssueMessage(cmake::FATAL_ERROR, e.str(),
                                                  parent->Backtrace);
    return;
  }

  {
    std::ostringstream e;
    /* clang-format off */
  e << "Error evaluating generator expression:\n"
    << "  " << expr << "\n"
    << "Dependency loop found.";
    /* clang-format on */
    context->LG->GetCMakeInstance()->IssueMessage(cmake::FATAL_ERROR, e.str(),
                                                  context->Backtrace);
  }

  int loopStep = 1;
  while (parent) {
    std::ostringstream e;
    e << "Loop step " << loopStep << "\n"
      << "  "
      << (parent->Content ? parent->Content->GetOriginalExpression() : expr)
      << "\n";
    context->LG->GetCMakeInstance()->IssueMessage(cmake::FATAL_ERROR, e.str(),
                                                  parent->Backtrace);
    parent = parent->Parent;
    ++loopStep;
  }
}

cmGeneratorExpressionDAGChecker::Result
cmGeneratorExpressionDAGChecker::CheckGraph() const
{
  const cmGeneratorExpressionDAGChecker* parent = this->Parent;
  while (parent) {
    if (this->Target == parent->Target && this->Property == parent->Property) {
      return (parent == this->Parent) ? SELF_REFERENCE : CYCLIC_REFERENCE;
    }
    parent = parent->Parent;
  }
  return DAG;
}

bool cmGeneratorExpressionDAGChecker::GetTransitivePropertiesOnly()
{
  const cmGeneratorExpressionDAGChecker* top = this;
  const cmGeneratorExpressionDAGChecker* parent = this->Parent;
  while (parent) {
    top = parent;
    parent = parent->Parent;
  }

  return top->TransitivePropertiesOnly;
}

bool cmGeneratorExpressionDAGChecker::EvaluatingLinkLibraries(const char* tgt)
{
  const cmGeneratorExpressionDAGChecker* top = this;
  const cmGeneratorExpressionDAGChecker* parent = this->Parent;
  while (parent) {
    top = parent;
    parent = parent->Parent;
  }

  const char* prop = top->Property.c_str();

  if (tgt) {
    return top->Target == tgt && strcmp(prop, "LINK_LIBRARIES") == 0;
  }

  return (strcmp(prop, "LINK_LIBRARIES") == 0 ||
          strcmp(prop, "LINK_INTERFACE_LIBRARIES") == 0 ||
          strcmp(prop, "IMPORTED_LINK_INTERFACE_LIBRARIES") == 0 ||
          cmHasLiteralPrefix(prop, "LINK_INTERFACE_LIBRARIES_") ||
          cmHasLiteralPrefix(prop, "IMPORTED_LINK_INTERFACE_LIBRARIES_")) ||
    strcmp(prop, "INTERFACE_LINK_LIBRARIES") == 0;
}

std::string cmGeneratorExpressionDAGChecker::TopTarget() const
{
  const cmGeneratorExpressionDAGChecker* top = this;
  const cmGeneratorExpressionDAGChecker* parent = this->Parent;
  while (parent) {
    top = parent;
    parent = parent->Parent;
  }
  return top->Target;
}

enum TransitiveProperty
{
#define DEFINE_ENUM_ENTRY(NAME) NAME,
  CM_FOR_EACH_TRANSITIVE_PROPERTY_NAME(DEFINE_ENUM_ENTRY)
#undef DEFINE_ENUM_ENTRY
    TransitivePropertyTerminal
};

template <TransitiveProperty>
bool additionalTest(const char* const /*unused*/)
{
  return false;
}

template <>
bool additionalTest<COMPILE_DEFINITIONS>(const char* const prop)
{
  return cmHasLiteralPrefix(prop, "COMPILE_DEFINITIONS_");
}

#define DEFINE_TRANSITIVE_PROPERTY_METHOD(METHOD, PROPERTY)                   \
  bool cmGeneratorExpressionDAGChecker::METHOD() const                        \
  {                                                                           \
    const char* const prop = this->Property.c_str();                          \
    if (strcmp(prop, #PROPERTY) == 0 ||                                       \
        strcmp(prop, "INTERFACE_" #PROPERTY) == 0) {                          \
      return true;                                                            \
    }                                                                         \
    return additionalTest<PROPERTY>(prop);                                    \
  }

CM_FOR_EACH_TRANSITIVE_PROPERTY(DEFINE_TRANSITIVE_PROPERTY_METHOD)

#undef DEFINE_TRANSITIVE_PROPERTY_METHOD
