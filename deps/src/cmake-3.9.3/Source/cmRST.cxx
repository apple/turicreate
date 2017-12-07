/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmRST.h"

#include "cmAlgorithms.h"
#include "cmSystemTools.h"
#include "cmVersion.h"

#include "cmsys/FStream.hxx"
#include <algorithm>
#include <ctype.h>
#include <iterator>
#include <stddef.h>
#include <utility>

cmRST::cmRST(std::ostream& os, std::string const& docroot)
  : OS(os)
  , DocRoot(docroot)
  , IncludeDepth(0)
  , OutputLinePending(false)
  , LastLineEndedInColonColon(false)
  , Markup(MarkupNone)
  , Directive(DirectiveNone)
  , CMakeDirective("^.. (cmake:)?("
                   "command|variable"
                   ")::[ \t]+([^ \t\n]+)$")
  , CMakeModuleDirective("^.. cmake-module::[ \t]+([^ \t\n]+)$")
  , ParsedLiteralDirective("^.. parsed-literal::[ \t]*(.*)$")
  , CodeBlockDirective("^.. code-block::[ \t]*(.*)$")
  , ReplaceDirective("^.. (\\|[^|]+\\|) replace::[ \t]*(.*)$")
  , IncludeDirective("^.. include::[ \t]+([^ \t\n]+)$")
  , TocTreeDirective("^.. toctree::[ \t]*(.*)$")
  , ProductionListDirective("^.. productionlist::[ \t]*(.*)$")
  , NoteDirective("^.. note::[ \t]*(.*)$")
  , ModuleRST("^#\\[(=*)\\[\\.rst:$")
  , CMakeRole("(:cmake)?:("
              "command|generator|variable|module|policy|"
              "prop_cache|prop_dir|prop_gbl|prop_inst|prop_sf|"
              "prop_test|prop_tgt|"
              "manual"
              "):`(<*([^`<]|[^` \t]<)*)([ \t]+<[^`]*>)?`")
  , Substitution("(^|[^A-Za-z0-9_])"
                 "((\\|[^| \t\r\n]([^|\r\n]*[^| \t\r\n])?\\|)(__|_|))"
                 "([^A-Za-z0-9_]|$)")
  , TocTreeLink("^.*[ \t]+<([^>]+)>$")
{
  this->Replace["|release|"] = cmVersion::GetCMakeVersion();
}

bool cmRST::ProcessFile(std::string const& fname, bool isModule)
{
  cmsys::ifstream fin(fname.c_str());
  if (fin) {
    this->DocDir = cmSystemTools::GetFilenamePath(fname);
    if (isModule) {
      this->ProcessModule(fin);
    } else {
      this->ProcessRST(fin);
    }
    this->OutputLinePending = true;
    return true;
  }
  return false;
}

void cmRST::ProcessRST(std::istream& is)
{
  std::string line;
  while (cmSystemTools::GetLineFromStream(is, line)) {
    this->ProcessLine(line);
  }
  this->Reset();
}

void cmRST::ProcessModule(std::istream& is)
{
  std::string line;
  std::string rst;
  while (cmSystemTools::GetLineFromStream(is, line)) {
    if (!rst.empty() && rst != "#") {
      // Bracket mode: check for end bracket
      std::string::size_type pos = line.find(rst);
      if (pos == std::string::npos) {
        this->ProcessLine(line);
      } else {
        if (line[0] != '#') {
          this->ProcessLine(line.substr(0, pos));
        }
        rst = "";
        this->Reset();
        this->OutputLinePending = true;
      }
    } else {
      // Line mode: check for .rst start (bracket or line)
      if (rst == "#") {
        if (line == "#") {
          this->ProcessLine("");
          continue;
        }
        if (line.substr(0, 2) == "# ") {
          this->ProcessLine(line.substr(2));
          continue;
        }
        rst = "";
        this->Reset();
        this->OutputLinePending = true;
      }
      if (line == "#.rst:") {
        rst = "#";
      } else if (this->ModuleRST.find(line)) {
        rst = "]" + this->ModuleRST.match(1) + "]";
      }
    }
  }
  if (rst == "#") {
    this->Reset();
  }
}

void cmRST::Reset()
{
  if (!this->MarkupLines.empty()) {
    this->UnindentLines(this->MarkupLines);
  }
  switch (this->Directive) {
    case DirectiveNone:
      break;
    case DirectiveParsedLiteral:
      this->ProcessDirectiveParsedLiteral();
      break;
    case DirectiveLiteralBlock:
      this->ProcessDirectiveLiteralBlock();
      break;
    case DirectiveCodeBlock:
      this->ProcessDirectiveCodeBlock();
      break;
    case DirectiveReplace:
      this->ProcessDirectiveReplace();
      break;
    case DirectiveTocTree:
      this->ProcessDirectiveTocTree();
      break;
  }
  this->Markup = MarkupNone;
  this->Directive = DirectiveNone;
  this->MarkupLines.clear();
}

void cmRST::ProcessLine(std::string const& line)
{
  bool lastLineEndedInColonColon = this->LastLineEndedInColonColon;
  this->LastLineEndedInColonColon = false;

  // A line starting in .. is an explicit markup start.
  if (line == ".." || (line.size() >= 3 && line[0] == '.' && line[1] == '.' &&
                       isspace(line[2]))) {
    this->Reset();
    this->Markup =
      (line.find_first_not_of(" \t", 2) == std::string::npos ? MarkupEmpty
                                                             : MarkupNormal);
    if (this->CMakeDirective.find(line)) {
      // Output cmake domain directives and their content normally.
      this->NormalLine(line);
    } else if (this->CMakeModuleDirective.find(line)) {
      // Process cmake-module directive: scan .cmake file comments.
      std::string file = this->CMakeModuleDirective.match(1);
      if (file.empty() || !this->ProcessInclude(file, IncludeModule)) {
        this->NormalLine(line);
      }
    } else if (this->ParsedLiteralDirective.find(line)) {
      // Record the literal lines to output after whole block.
      this->Directive = DirectiveParsedLiteral;
      this->MarkupLines.push_back(this->ParsedLiteralDirective.match(1));
    } else if (this->CodeBlockDirective.find(line)) {
      // Record the literal lines to output after whole block.
      // Ignore the language spec and record the opening line as blank.
      this->Directive = DirectiveCodeBlock;
      this->MarkupLines.push_back("");
    } else if (this->ReplaceDirective.find(line)) {
      // Record the replace directive content.
      this->Directive = DirectiveReplace;
      this->ReplaceName = this->ReplaceDirective.match(1);
      this->MarkupLines.push_back(this->ReplaceDirective.match(2));
    } else if (this->IncludeDirective.find(line)) {
      // Process the include directive or output the directive and its
      // content normally if it fails.
      std::string file = this->IncludeDirective.match(1);
      if (file.empty() || !this->ProcessInclude(file, IncludeNormal)) {
        this->NormalLine(line);
      }
    } else if (this->TocTreeDirective.find(line)) {
      // Record the toctree entries to process after whole block.
      this->Directive = DirectiveTocTree;
      this->MarkupLines.push_back(this->TocTreeDirective.match(1));
    } else if (this->ProductionListDirective.find(line)) {
      // Output productionlist directives and their content normally.
      this->NormalLine(line);
    } else if (this->NoteDirective.find(line)) {
      // Output note directives and their content normally.
      this->NormalLine(line);
    }
  }
  // An explicit markup start followed nothing but whitespace and a
  // blank line does not consume any indented text following.
  else if (this->Markup == MarkupEmpty && line.empty()) {
    this->NormalLine(line);
  }
  // Indented lines following an explicit markup start are explicit markup.
  else if (this->Markup && (line.empty() || isspace(line[0]))) {
    this->Markup = MarkupNormal;
    // Record markup lines if the start line was recorded.
    if (!this->MarkupLines.empty()) {
      this->MarkupLines.push_back(line);
    }
  }
  // A blank line following a paragraph ending in "::" starts a literal block.
  else if (lastLineEndedInColonColon && line.empty()) {
    // Record the literal lines to output after whole block.
    this->Markup = MarkupNormal;
    this->Directive = DirectiveLiteralBlock;
    this->MarkupLines.push_back("");
    this->OutputLine("", false);
  }
  // Print non-markup lines.
  else {
    this->NormalLine(line);
    this->LastLineEndedInColonColon =
      (line.size() >= 2 && line[line.size() - 2] == ':' &&
       line[line.size() - 1] == ':');
  }
}

void cmRST::NormalLine(std::string const& line)
{
  this->Reset();
  this->OutputLine(line, true);
}

void cmRST::OutputLine(std::string const& line_in, bool inlineMarkup)
{
  if (this->OutputLinePending) {
    this->OS << "\n";
    this->OutputLinePending = false;
  }
  if (inlineMarkup) {
    std::string line = this->ReplaceSubstitutions(line_in);
    std::string::size_type pos = 0;
    while (this->CMakeRole.find(line.c_str() + pos)) {
      this->OS << line.substr(pos, this->CMakeRole.start());
      std::string text = this->CMakeRole.match(3);
      // If a command reference has no explicit target and
      // no explicit "(...)" then add "()" to the text.
      if (this->CMakeRole.match(2) == "command" &&
          this->CMakeRole.match(5).empty() &&
          text.find_first_of("()") == std::string::npos) {
        text += "()";
      }
      this->OS << "``" << text << "``";
      pos += this->CMakeRole.end();
    }
    this->OS << line.substr(pos) << "\n";
  } else {
    this->OS << line_in << "\n";
  }
}

std::string cmRST::ReplaceSubstitutions(std::string const& line)
{
  std::string out;
  std::string::size_type pos = 0;
  while (this->Substitution.find(line.c_str() + pos)) {
    std::string::size_type start = this->Substitution.start(2);
    std::string::size_type end = this->Substitution.end(2);
    std::string substitute = this->Substitution.match(3);
    std::map<std::string, std::string>::iterator replace =
      this->Replace.find(substitute);
    if (replace != this->Replace.end()) {
      std::pair<std::set<std::string>::iterator, bool> replaced =
        this->Replaced.insert(substitute);
      if (replaced.second) {
        substitute = this->ReplaceSubstitutions(replace->second);
        this->Replaced.erase(replaced.first);
      }
    }
    out += line.substr(pos, start);
    out += substitute;
    pos += end;
  }
  out += line.substr(pos);
  return out;
}

void cmRST::OutputMarkupLines(bool inlineMarkup)
{
  for (std::vector<std::string>::iterator i = this->MarkupLines.begin();
       i != this->MarkupLines.end(); ++i) {
    std::string line = *i;
    if (!line.empty()) {
      line = " " + line;
    }
    this->OutputLine(line, inlineMarkup);
  }
  this->OutputLinePending = true;
}

bool cmRST::ProcessInclude(std::string file, IncludeType type)
{
  bool found = false;
  if (this->IncludeDepth < 10) {
    cmRST r(this->OS, this->DocRoot);
    r.IncludeDepth = this->IncludeDepth + 1;
    r.OutputLinePending = this->OutputLinePending;
    if (type != IncludeTocTree) {
      r.Replace = this->Replace;
    }
    if (file[0] == '/') {
      file = this->DocRoot + file;
    } else {
      file = this->DocDir + "/" + file;
    }
    found = r.ProcessFile(file, type == IncludeModule);
    if (type != IncludeTocTree) {
      this->Replace = r.Replace;
    }
    this->OutputLinePending = r.OutputLinePending;
  }
  return found;
}

void cmRST::ProcessDirectiveParsedLiteral()
{
  this->OutputMarkupLines(true);
}

void cmRST::ProcessDirectiveLiteralBlock()
{
  this->OutputMarkupLines(false);
}

void cmRST::ProcessDirectiveCodeBlock()
{
  this->OutputMarkupLines(false);
}

void cmRST::ProcessDirectiveReplace()
{
  // Record markup lines as replacement text.
  std::string& replacement = this->Replace[this->ReplaceName];
  replacement += cmJoin(this->MarkupLines, " ");
  this->ReplaceName = "";
}

void cmRST::ProcessDirectiveTocTree()
{
  // Process documents referenced by toctree directive.
  for (std::vector<std::string>::iterator i = this->MarkupLines.begin();
       i != this->MarkupLines.end(); ++i) {
    std::string const& line = *i;
    if (!line.empty() && line[0] != ':') {
      if (this->TocTreeLink.find(line)) {
        std::string const& link = this->TocTreeLink.match(1);
        this->ProcessInclude(link + ".rst", IncludeTocTree);
      } else {
        this->ProcessInclude(line + ".rst", IncludeTocTree);
      }
    }
  }
}

void cmRST::UnindentLines(std::vector<std::string>& lines)
{
  // Remove the common indentation from the second and later lines.
  std::string indentText;
  std::string::size_type indentEnd = 0;
  bool first = true;
  for (size_t i = 1; i < lines.size(); ++i) {
    std::string const& line = lines[i];

    // Do not consider empty lines.
    if (line.empty()) {
      continue;
    }

    // Record indentation on first non-empty line.
    if (first) {
      first = false;
      indentEnd = line.find_first_not_of(" \t");
      indentText = line.substr(0, indentEnd);
      continue;
    }

    // Truncate indentation to match that on this line.
    indentEnd = std::min(indentEnd, line.size());
    for (std::string::size_type j = 0; j != indentEnd; ++j) {
      if (line[j] != indentText[j]) {
        indentEnd = j;
        break;
      }
    }
  }

  // Update second and later lines.
  for (size_t i = 1; i < lines.size(); ++i) {
    std::string& line = lines[i];
    if (!line.empty()) {
      line = line.substr(indentEnd);
    }
  }

  std::vector<std::string>::const_iterator it = lines.begin();
  size_t leadingEmpty = std::distance(it, cmFindNot(lines, std::string()));

  std::vector<std::string>::const_reverse_iterator rit = lines.rbegin();
  size_t trailingEmpty =
    std::distance(rit, cmFindNot(cmReverseRange(lines), std::string()));

  std::vector<std::string>::iterator contentEnd = cmRotate(
    lines.begin(), lines.begin() + leadingEmpty, lines.end() - trailingEmpty);
  lines.erase(contentEnd, lines.end());
}
