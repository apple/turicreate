/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXPatch.h"

#include "cmCPackGenerator.h"

cmWIXPatch::cmWIXPatch(cmCPackLog* logger)
  : Logger(logger)
{
}

bool cmWIXPatch::LoadFragments(std::string const& patchFilePath)
{
  cmWIXPatchParser parser(Fragments, Logger);
  if (!parser.ParseFile(patchFilePath.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Failed parsing XML patch file: '" << patchFilePath << "'"
                                                     << std::endl);
    return false;
  }

  return true;
}

void cmWIXPatch::ApplyFragment(std::string const& id,
                               cmWIXSourceWriter& writer)
{
  cmWIXPatchParser::fragment_map_t::iterator i = Fragments.find(id);
  if (i == Fragments.end())
    return;

  const cmWIXPatchElement& fragment = i->second;
  for (auto const& attr : fragment.attributes) {
    writer.AddAttribute(attr.first, attr.second);
  }
  this->ApplyElementChildren(fragment, writer);

  Fragments.erase(i);
}

void cmWIXPatch::ApplyElementChildren(const cmWIXPatchElement& element,
                                      cmWIXSourceWriter& writer)
{
  for (cmWIXPatchNode* node : element.children) {
    switch (node->type()) {
      case cmWIXPatchNode::ELEMENT:
        ApplyElement(dynamic_cast<const cmWIXPatchElement&>(*node), writer);
        break;
      case cmWIXPatchNode::TEXT:
        writer.AddTextNode(dynamic_cast<const cmWIXPatchText&>(*node).text);
        break;
    }
  }
}

void cmWIXPatch::ApplyElement(const cmWIXPatchElement& element,
                              cmWIXSourceWriter& writer)
{
  writer.BeginElement(element.name);

  for (auto const& attr : element.attributes) {
    writer.AddAttribute(attr.first, attr.second);
  }

  this->ApplyElementChildren(element, writer);

  writer.EndElement(element.name);
}

bool cmWIXPatch::CheckForUnappliedFragments()
{
  std::string fragmentList;
  for (auto const& fragment : Fragments) {
    if (!fragmentList.empty()) {
      fragmentList += ", ";
    }

    fragmentList += "'";
    fragmentList += fragment.first;
    fragmentList += "'";
  }

  if (!fragmentList.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Some XML patch fragments did not have matching IDs: "
                    << fragmentList << std::endl);
    return false;
  }

  return true;
}
