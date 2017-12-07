/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCursesMainForm.h"

#include "cmAlgorithms.h"
#include "cmCursesCacheEntryComposite.h"
#include "cmCursesDummyWidget.h"
#include "cmCursesForm.h"
#include "cmCursesLabelWidget.h"
#include "cmCursesLongMessageForm.h"
#include "cmCursesStandardIncludes.h"
#include "cmCursesStringWidget.h"
#include "cmCursesWidget.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmake.h"

#include <stdio.h>
#include <string.h>

inline int ctrl(int z)
{
  return (z & 037);
}

cmCursesMainForm::cmCursesMainForm(std::vector<std::string> const& args,
                                   int initWidth)
  : Args(args)
  , InitialWidth(initWidth)
{
  this->NumberOfPages = 0;
  this->Fields = CM_NULLPTR;
  this->Entries = CM_NULLPTR;
  this->AdvancedMode = false;
  this->NumberOfVisibleEntries = 0;
  this->OkToGenerate = false;
  this->HelpMessage.push_back(
    "Welcome to ccmake, curses based user interface for CMake.");
  this->HelpMessage.push_back("");
  this->HelpMessage.push_back(s_ConstHelpMessage);
  this->CMakeInstance = new cmake(cmake::RoleProject);
  this->CMakeInstance->SetCMakeEditCommand(
    cmSystemTools::GetCMakeCursesCommand());

  // create the arguments for the cmake object
  std::string whereCMake = cmSystemTools::GetProgramPath(this->Args[0]);
  whereCMake += "/cmake";
  this->Args[0] = whereCMake;
  this->CMakeInstance->SetArgs(this->Args);
  this->SearchString = "";
  this->OldSearchString = "";
  this->SearchMode = false;
}

cmCursesMainForm::~cmCursesMainForm()
{
  if (this->Form) {
    unpost_form(this->Form);
    free_form(this->Form);
    this->Form = CM_NULLPTR;
  }
  delete[] this->Fields;

  // Clean-up composites
  if (this->Entries) {
    cmDeleteAll(*this->Entries);
  }
  delete this->Entries;
  if (this->CMakeInstance) {
    delete this->CMakeInstance;
    this->CMakeInstance = CM_NULLPTR;
  }
}

// See if a cache entry is in the list of entries in the ui.
bool cmCursesMainForm::LookForCacheEntry(const std::string& key)
{
  if (!this->Entries) {
    return false;
  }

  std::vector<cmCursesCacheEntryComposite*>::iterator it;
  for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
    if (key == (*it)->Key) {
      return true;
    }
  }

  return false;
}

// Create new cmCursesCacheEntryComposite entries from the cache
void cmCursesMainForm::InitializeUI()
{
  // Create a vector of cmCursesCacheEntryComposite's
  // which contain labels, entries and new entry markers
  std::vector<cmCursesCacheEntryComposite*>* newEntries =
    new std::vector<cmCursesCacheEntryComposite*>;
  std::vector<std::string> cacheKeys =
    this->CMakeInstance->GetState()->GetCacheEntryKeys();
  newEntries->reserve(cacheKeys.size());

  // Count non-internal and non-static entries
  int count = 0;

  for (std::vector<std::string>::const_iterator it = cacheKeys.begin();
       it != cacheKeys.end(); ++it) {
    cmStateEnums::CacheEntryType t =
      this->CMakeInstance->GetState()->GetCacheEntryType(*it);
    if (t != cmStateEnums::INTERNAL && t != cmStateEnums::STATIC &&
        t != cmStateEnums::UNINITIALIZED) {
      ++count;
    }
  }

  int entrywidth = this->InitialWidth - 35;

  cmCursesCacheEntryComposite* comp;
  if (count == 0) {
    // If cache is empty, display a label saying so and a
    // dummy entry widget (does not respond to input)
    comp = new cmCursesCacheEntryComposite("EMPTY CACHE", 30, 30);
    comp->Entry = new cmCursesDummyWidget(1, 1, 1, 1);
    newEntries->push_back(comp);
  } else {
    // Create the composites.

    // First add entries which are new
    for (std::vector<std::string>::const_iterator it = cacheKeys.begin();
         it != cacheKeys.end(); ++it) {
      std::string key = *it;
      cmStateEnums::CacheEntryType t =
        this->CMakeInstance->GetState()->GetCacheEntryType(*it);
      if (t == cmStateEnums::INTERNAL || t == cmStateEnums::STATIC ||
          t == cmStateEnums::UNINITIALIZED) {
        continue;
      }

      if (!this->LookForCacheEntry(key)) {
        newEntries->push_back(new cmCursesCacheEntryComposite(
          key, this->CMakeInstance, true, 30, entrywidth));
        this->OkToGenerate = false;
      }
    }

    // then add entries which are old
    for (std::vector<std::string>::const_iterator it = cacheKeys.begin();
         it != cacheKeys.end(); ++it) {
      std::string key = *it;
      cmStateEnums::CacheEntryType t =
        this->CMakeInstance->GetState()->GetCacheEntryType(*it);
      if (t == cmStateEnums::INTERNAL || t == cmStateEnums::STATIC ||
          t == cmStateEnums::UNINITIALIZED) {
        continue;
      }

      if (this->LookForCacheEntry(key)) {
        newEntries->push_back(new cmCursesCacheEntryComposite(
          key, this->CMakeInstance, false, 30, entrywidth));
      }
    }
  }

  // Clean old entries
  if (this->Entries) {
    cmDeleteAll(*this->Entries);
  }
  delete this->Entries;
  this->Entries = newEntries;

  // Compute fields from composites
  this->RePost();
}

void cmCursesMainForm::RePost()
{
  // Create the fields to be passed to the form.
  if (this->Form) {
    unpost_form(this->Form);
    free_form(this->Form);
    this->Form = CM_NULLPTR;
  }
  delete[] this->Fields;
  if (this->AdvancedMode) {
    this->NumberOfVisibleEntries = this->Entries->size();
  } else {
    // If normal mode, count only non-advanced entries
    this->NumberOfVisibleEntries = 0;
    std::vector<cmCursesCacheEntryComposite*>::iterator it;
    for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
      const char* existingValue =
        this->CMakeInstance->GetState()->GetCacheEntryValue((*it)->GetValue());
      bool advanced =
        this->CMakeInstance->GetState()->GetCacheEntryPropertyAsBool(
          (*it)->GetValue(), "ADVANCED");
      if (!existingValue || (!this->AdvancedMode && advanced)) {
        continue;
      }
      this->NumberOfVisibleEntries++;
    }
  }
  // there is always one even if it is the dummy one
  if (this->NumberOfVisibleEntries == 0) {
    this->NumberOfVisibleEntries = 1;
  }
  // Assign the fields: 3 for each entry: label, new entry marker
  // ('*' or ' ') and entry widget
  this->Fields = new FIELD*[3 * this->NumberOfVisibleEntries + 1];
  size_t cc;
  for (cc = 0; cc < 3 * this->NumberOfVisibleEntries + 1; cc++) {
    this->Fields[cc] = CM_NULLPTR;
  }

  // Assign fields
  int j = 0;
  std::vector<cmCursesCacheEntryComposite*>::iterator it;
  for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
    const char* existingValue =
      this->CMakeInstance->GetState()->GetCacheEntryValue((*it)->GetValue());
    bool advanced =
      this->CMakeInstance->GetState()->GetCacheEntryPropertyAsBool(
        (*it)->GetValue(), "ADVANCED");
    if (!existingValue || (!this->AdvancedMode && advanced)) {
      continue;
    }
    this->Fields[3 * j] = (*it)->Label->Field;
    this->Fields[3 * j + 1] = (*it)->IsNewLabel->Field;
    this->Fields[3 * j + 2] = (*it)->Entry->Field;
    j++;
  }
  // if no cache entries there should still be one dummy field
  if (j == 0) {
    it = this->Entries->begin();
    this->Fields[0] = (*it)->Label->Field;
    this->Fields[1] = (*it)->IsNewLabel->Field;
    this->Fields[2] = (*it)->Entry->Field;
    this->NumberOfVisibleEntries = 1;
  }
  // Has to be null terminated.
  this->Fields[3 * this->NumberOfVisibleEntries] = CM_NULLPTR;
}

void cmCursesMainForm::Render(int left, int top, int width, int height)
{

  if (this->Form) {
    FIELD* currentField = current_field(this->Form);
    cmCursesWidget* cw =
      reinterpret_cast<cmCursesWidget*>(field_userptr(currentField));
    // If in edit mode, get out of it
    if (cw->GetType() == cmStateEnums::STRING ||
        cw->GetType() == cmStateEnums::PATH ||
        cw->GetType() == cmStateEnums::FILEPATH) {
      cmCursesStringWidget* sw = static_cast<cmCursesStringWidget*>(cw);
      sw->SetInEdit(false);
    }
    // Delete the previous form
    unpost_form(this->Form);
    free_form(this->Form);
    this->Form = CM_NULLPTR;
  }

  // Wrong window size
  if (width < cmCursesMainForm::MIN_WIDTH || width < this->InitialWidth ||
      height < cmCursesMainForm::MIN_HEIGHT) {
    return;
  }

  // Leave room for toolbar
  height -= 7;

  if (this->AdvancedMode) {
    this->NumberOfVisibleEntries = this->Entries->size();
  } else {
    // If normal, display only non-advanced entries
    this->NumberOfVisibleEntries = 0;
    std::vector<cmCursesCacheEntryComposite*>::iterator it;
    for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
      const char* existingValue =
        this->CMakeInstance->GetState()->GetCacheEntryValue((*it)->GetValue());
      bool advanced =
        this->CMakeInstance->GetState()->GetCacheEntryPropertyAsBool(
          (*it)->GetValue(), "ADVANCED");
      if (!existingValue || (!this->AdvancedMode && advanced)) {
        continue;
      }
      this->NumberOfVisibleEntries++;
    }
  }

  // Re-adjust the fields according to their place
  this->NumberOfPages = 1;
  if (height > 0) {
    bool isNewPage;
    int i = 0;
    std::vector<cmCursesCacheEntryComposite*>::iterator it;
    for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
      const char* existingValue =
        this->CMakeInstance->GetState()->GetCacheEntryValue((*it)->GetValue());
      bool advanced =
        this->CMakeInstance->GetState()->GetCacheEntryPropertyAsBool(
          (*it)->GetValue(), "ADVANCED");
      if (!existingValue || (!this->AdvancedMode && advanced)) {
        continue;
      }
      int row = (i % height) + 1;
      int page = (i / height) + 1;
      isNewPage = (page > 1) && (row == 1);

      if (isNewPage) {
        this->NumberOfPages++;
      }
      (*it)->Label->Move(left, top + row - 1, isNewPage);
      (*it)->IsNewLabel->Move(left + 32, top + row - 1, false);
      (*it)->Entry->Move(left + 33, top + row - 1, false);
      (*it)->Entry->SetPage(this->NumberOfPages);
      i++;
    }
  }

  // Post the form
  this->Form = new_form(this->Fields);
  post_form(this->Form);
  // Update toolbar
  this->UpdateStatusBar();
  this->PrintKeys();

  touchwin(stdscr);
  refresh();
}

void cmCursesMainForm::PrintKeys(int process /* = 0 */)
{
  int x, y;
  getmaxyx(stdscr, y, x);
  if (x < cmCursesMainForm::MIN_WIDTH || x < this->InitialWidth ||
      y < cmCursesMainForm::MIN_HEIGHT) {
    return;
  }

  // Give the current widget (if it exists), a chance to print keys
  cmCursesWidget* cw = CM_NULLPTR;
  if (this->Form) {
    FIELD* currentField = current_field(this->Form);
    cw = reinterpret_cast<cmCursesWidget*>(field_userptr(currentField));
  }

  char fmt_s[] = "%s";
  if (cw == CM_NULLPTR || !cw->PrintKeys()) {
    char firstLine[512] = "";
    char secondLine[512] = "";
    char thirdLine[512] = "";
    if (process) {
      const char* clearLine =
        "                                                                    ";
      strcpy(firstLine, clearLine);
      strcpy(secondLine, clearLine);
      strcpy(thirdLine, clearLine);
    } else {
      if (this->OkToGenerate) {
        sprintf(firstLine,
                "Press [c] to configure       Press [g] to generate and exit");
      } else {
        sprintf(firstLine,
                "Press [c] to configure                                     ");
      }
      {
        const char* toggleKeyInstruction =
          "Press [t] to toggle advanced mode (Currently %s)";
        sprintf(thirdLine, toggleKeyInstruction,
                this->AdvancedMode ? "On" : "Off");
      }
      sprintf(secondLine, "Press [h] for help           "
                          "Press [q] to quit without generating");
    }

    curses_move(y - 4, 0);
    char fmt[512] =
      "Press [enter] to edit option Press [d] to delete an entry";
    if (process) {
      strcpy(fmt, "                           ");
    }
    printw(fmt_s, fmt);
    curses_move(y - 3, 0);
    printw(fmt_s, firstLine);
    curses_move(y - 2, 0);
    printw(fmt_s, secondLine);
    curses_move(y - 1, 0);
    printw(fmt_s, thirdLine);
  }

  if (cw) {
    char pageLine[512] = "";
    sprintf(pageLine, "Page %d of %d", cw->GetPage(), this->NumberOfPages);
    curses_move(0, 65 - static_cast<unsigned int>(strlen(pageLine)) - 1);
    printw(fmt_s, pageLine);
  }

  pos_form_cursor(this->Form);
}

// Print the key of the current entry and the CMake version
// on the status bar. Designed for a width of 80 chars.
void cmCursesMainForm::UpdateStatusBar(const char* message)
{
  int x, y;
  getmaxyx(stdscr, y, x);
  // If window size is too small, display error and return
  if (x < cmCursesMainForm::MIN_WIDTH || x < this->InitialWidth ||
      y < cmCursesMainForm::MIN_HEIGHT) {
    curses_clear();
    curses_move(0, 0);
    char fmt[] = "Window is too small. A size of at least %dx%d is required.";
    printw(fmt, (cmCursesMainForm::MIN_WIDTH < this->InitialWidth
                   ? this->InitialWidth
                   : cmCursesMainForm::MIN_WIDTH),
           cmCursesMainForm::MIN_HEIGHT);
    touchwin(stdscr);
    wrefresh(stdscr);
    return;
  }

  // Get the key of the current entry
  FIELD* cur = current_field(this->Form);
  int findex = field_index(cur);
  cmCursesWidget* lbl = CM_NULLPTR;
  if (findex >= 0) {
    lbl = reinterpret_cast<cmCursesWidget*>(
      field_userptr(this->Fields[findex - 2]));
  }
  char help[128] = "";
  const char* curField = "";
  if (lbl) {
    curField = lbl->GetValue();

    // Get the help string of the current entry
    // and add it to the help string
    const char* existingValue =
      this->CMakeInstance->GetState()->GetCacheEntryValue(curField);
    if (existingValue) {
      const char* hs = this->CMakeInstance->GetState()->GetCacheEntryProperty(
        curField, "HELPSTRING");
      if (hs) {
        strncpy(help, hs, 127);
        help[127] = '\0';
      } else {
        help[0] = 0;
      }
    } else {
      sprintf(help, " ");
    }
  }

  // Join the key, help string and pad with spaces
  // (or truncate) as necessary
  char bar[cmCursesMainForm::MAX_WIDTH];
  size_t i, curFieldLen = strlen(curField);
  size_t helpLen = strlen(help);

  size_t width;
  if (x < cmCursesMainForm::MAX_WIDTH) {
    width = x;
  } else {
    width = cmCursesMainForm::MAX_WIDTH;
  }

  if (message) {
    curField = message;
    curFieldLen = strlen(message);
    if (curFieldLen < width) {
      strcpy(bar, curField);
      for (i = curFieldLen; i < width; ++i) {
        bar[i] = ' ';
      }
    } else {
      strncpy(bar, curField, width);
    }
  } else {
    if (curFieldLen >= width) {
      strncpy(bar, curField, width);
    } else {
      strcpy(bar, curField);
      bar[curFieldLen] = ':';
      bar[curFieldLen + 1] = ' ';
      if (curFieldLen + helpLen + 2 >= width) {
        strncpy(bar + curFieldLen + 2, help, width - curFieldLen - 2);
      } else {
        strcpy(bar + curFieldLen + 2, help);
        for (i = curFieldLen + helpLen + 2; i < width; ++i) {
          bar[i] = ' ';
        }
      }
    }
  }

  bar[width] = '\0';

  // Display CMake version info on the next line
  // We want to display this on the right
  char version[cmCursesMainForm::MAX_WIDTH];
  char vertmp[128];
  sprintf(vertmp, "CMake Version %s", cmVersion::GetCMakeVersion());
  size_t sideSpace = (width - strlen(vertmp));
  for (i = 0; i < sideSpace; i++) {
    version[i] = ' ';
  }
  sprintf(version + sideSpace, "%s", vertmp);
  version[width] = '\0';

  // Now print both lines
  char fmt_s[] = "%s";
  curses_move(y - 5, 0);
  attron(A_STANDOUT);
  printw(fmt_s, bar);
  attroff(A_STANDOUT);
  curses_move(y - 4, 0);
  printw(fmt_s, version);
  pos_form_cursor(this->Form);
}

void cmCursesMainForm::UpdateProgress(const char* msg, float prog, void* vp)
{
  cmCursesMainForm* cm = static_cast<cmCursesMainForm*>(vp);
  if (!cm) {
    return;
  }
  char tmp[1024];
  const char* cmsg = tmp;
  if (prog >= 0) {
    sprintf(tmp, "%s %i%%", msg, (int)(100 * prog));
  } else {
    cmsg = msg;
  }
  cm->UpdateStatusBar(cmsg);
  cm->PrintKeys(1);
  curses_move(1, 1);
  touchwin(stdscr);
  refresh();
}

int cmCursesMainForm::Configure(int noconfigure)
{
  int xi, yi;
  getmaxyx(stdscr, yi, xi);

  curses_move(1, 1);
  this->UpdateStatusBar("Configuring, please wait...");
  this->PrintKeys(1);
  touchwin(stdscr);
  refresh();
  this->CMakeInstance->SetProgressCallback(cmCursesMainForm::UpdateProgress,
                                           this);

  // always save the current gui values to disk
  this->FillCacheManagerFromUI();
  this->CMakeInstance->SaveCache(
    this->CMakeInstance->GetHomeOutputDirectory());
  this->LoadCache(CM_NULLPTR);

  // Get rid of previous errors
  this->Errors = std::vector<std::string>();

  // run the generate process
  this->OkToGenerate = true;
  int retVal;
  if (noconfigure) {
    retVal = this->CMakeInstance->DoPreConfigureChecks();
    this->OkToGenerate = false;
    if (retVal > 0) {
      retVal = 0;
    }
  } else {
    retVal = this->CMakeInstance->Configure();
  }
  this->CMakeInstance->SetProgressCallback(CM_NULLPTR, CM_NULLPTR);

  keypad(stdscr, true); /* Use key symbols as KEY_DOWN */

  if (retVal != 0 || !this->Errors.empty()) {
    // see if there was an error
    if (cmSystemTools::GetErrorOccuredFlag()) {
      this->OkToGenerate = false;
    }
    int xx, yy;
    getmaxyx(stdscr, yy, xx);
    cmCursesLongMessageForm* msgs = new cmCursesLongMessageForm(
      this->Errors, cmSystemTools::GetErrorOccuredFlag()
        ? "Errors occurred during the last pass."
        : "CMake produced the following output.");
    // reset error condition
    cmSystemTools::ResetErrorOccuredFlag();
    CurrentForm = msgs;
    msgs->Render(1, 1, xx, yy);
    msgs->HandleInput();
    // If they typed the wrong source directory, we report
    // an error and exit
    if (retVal == -2) {
      return retVal;
    }
    CurrentForm = this;
    this->Render(1, 1, xx, yy);
  }

  this->InitializeUI();
  this->Render(1, 1, xi, yi);

  return 0;
}

int cmCursesMainForm::Generate()
{
  int xi, yi;
  getmaxyx(stdscr, yi, xi);

  curses_move(1, 1);
  this->UpdateStatusBar("Generating, please wait...");
  this->PrintKeys(1);
  touchwin(stdscr);
  refresh();
  this->CMakeInstance->SetProgressCallback(cmCursesMainForm::UpdateProgress,
                                           this);

  // Get rid of previous errors
  this->Errors = std::vector<std::string>();

  // run the generate process
  int retVal = this->CMakeInstance->Generate();

  this->CMakeInstance->SetProgressCallback(CM_NULLPTR, CM_NULLPTR);
  keypad(stdscr, true); /* Use key symbols as KEY_DOWN */

  if (retVal != 0 || !this->Errors.empty()) {
    // see if there was an error
    if (cmSystemTools::GetErrorOccuredFlag()) {
      this->OkToGenerate = false;
    }
    // reset error condition
    cmSystemTools::ResetErrorOccuredFlag();
    int xx, yy;
    getmaxyx(stdscr, yy, xx);
    const char* title = "Messages during last pass.";
    if (cmSystemTools::GetErrorOccuredFlag()) {
      title = "Errors occurred during the last pass.";
    }
    cmCursesLongMessageForm* msgs =
      new cmCursesLongMessageForm(this->Errors, title);
    CurrentForm = msgs;
    msgs->Render(1, 1, xx, yy);
    msgs->HandleInput();
    // If they typed the wrong source directory, we report
    // an error and exit
    if (retVal == -2) {
      return retVal;
    }
    CurrentForm = this;
    this->Render(1, 1, xx, yy);
  }

  this->InitializeUI();
  this->Render(1, 1, xi, yi);

  return 0;
}

void cmCursesMainForm::AddError(const char* message, const char* /*unused*/)
{
  this->Errors.push_back(message);
}

void cmCursesMainForm::RemoveEntry(const char* value)
{
  if (!value) {
    return;
  }

  std::vector<cmCursesCacheEntryComposite*>::iterator it;
  for (it = this->Entries->begin(); it != this->Entries->end(); ++it) {
    const char* val = (*it)->GetValue();
    if (val && !strcmp(value, val)) {
      this->CMakeInstance->UnwatchUnusedCli(value);
      this->Entries->erase(it);
      break;
    }
  }
}

// copy from the list box to the cache manager
void cmCursesMainForm::FillCacheManagerFromUI()
{
  size_t size = this->Entries->size();
  for (size_t i = 0; i < size; i++) {
    std::string cacheKey = (*this->Entries)[i]->Key;
    const char* existingValue =
      this->CMakeInstance->GetState()->GetCacheEntryValue(cacheKey);
    if (existingValue) {
      std::string oldValue = existingValue;
      std::string newValue = (*this->Entries)[i]->Entry->GetValue();
      std::string fixedOldValue;
      std::string fixedNewValue;
      cmStateEnums::CacheEntryType t =
        this->CMakeInstance->GetState()->GetCacheEntryType(cacheKey);
      this->FixValue(t, oldValue, fixedOldValue);
      this->FixValue(t, newValue, fixedNewValue);

      if (!(fixedOldValue == fixedNewValue)) {
        // The user has changed the value.  Mark it as modified.
        this->CMakeInstance->GetState()->SetCacheEntryBoolProperty(
          cacheKey, "MODIFIED", true);
        this->CMakeInstance->GetState()->SetCacheEntryValue(cacheKey,
                                                            fixedNewValue);
      }
    }
  }
}

void cmCursesMainForm::FixValue(cmStateEnums::CacheEntryType type,
                                const std::string& in, std::string& out) const
{
  out = in.substr(0, in.find_last_not_of(' ') + 1);
  if (type == cmStateEnums::PATH || type == cmStateEnums::FILEPATH) {
    cmSystemTools::ConvertToUnixSlashes(out);
  }
  if (type == cmStateEnums::BOOL) {
    if (cmSystemTools::IsOff(out.c_str())) {
      out = "OFF";
    } else {
      out = "ON";
    }
  }
}

void cmCursesMainForm::HandleInput()
{
  int x = 0, y = 0;

  if (!this->Form) {
    return;
  }

  FIELD* currentField;
  cmCursesWidget* currentWidget;

  char debugMessage[128];

  for (;;) {
    this->UpdateStatusBar();
    this->PrintKeys();
    if (this->SearchMode) {
      std::string searchstr = "Search: " + this->SearchString;
      this->UpdateStatusBar(searchstr.c_str());
      this->PrintKeys(1);
      curses_move(y - 5, static_cast<unsigned int>(searchstr.size()));
      // curses_move(1,1);
      touchwin(stdscr);
      refresh();
    }
    int key = getch();

    getmaxyx(stdscr, y, x);
    // If window too small, handle 'q' only
    if (x < cmCursesMainForm::MIN_WIDTH || y < cmCursesMainForm::MIN_HEIGHT) {
      // quit
      if (key == 'q') {
        break;
      }
      continue;
    }

    currentField = current_field(this->Form);
    currentWidget =
      reinterpret_cast<cmCursesWidget*>(field_userptr(currentField));

    bool widgetHandled = false;

    if (this->SearchMode) {
      if (key == 10 || key == KEY_ENTER) {
        this->SearchMode = false;
        if (!this->SearchString.empty()) {
          this->JumpToCacheEntry(this->SearchString.c_str());
          this->OldSearchString = this->SearchString;
        }
        this->SearchString = "";
      }
      /*
      else if ( key == KEY_ESCAPE )
        {
        this->SearchMode = false;
        }
      */
      else if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') ||
               (key >= '0' && key <= '9') || (key == '_')) {
        if (this->SearchString.size() <
            static_cast<std::string::size_type>(x - 10)) {
          this->SearchString += static_cast<char>(key);
        }
      } else if (key == ctrl('h') || key == KEY_BACKSPACE || key == KEY_DC) {
        if (!this->SearchString.empty()) {
          this->SearchString.resize(this->SearchString.size() - 1);
        }
      }
    } else if (currentWidget && !this->SearchMode) {
      // Ask the current widget if it wants to handle input
      widgetHandled = currentWidget->HandleInput(key, this, stdscr);
      if (widgetHandled) {
        this->OkToGenerate = false;
        this->UpdateStatusBar();
        this->PrintKeys();
      }
    }
    if ((!currentWidget || !widgetHandled) && !this->SearchMode) {
      // If the current widget does not want to handle input,
      // we handle it.
      sprintf(debugMessage, "Main form handling input, key: %d", key);
      cmCursesForm::LogMessage(debugMessage);
      // quit
      if (key == 'q') {
        break;
      }
      // if not end of page, next field otherwise next page
      // each entry consists of fields: label, isnew, value
      // therefore, the label field for the prev. entry is index-5
      // and the label field for the next entry is index+1
      // (index always corresponds to the value field)
      // scroll down with arrow down, ctrl+n (emacs binding), or j (vim
      // binding)
      if (key == KEY_DOWN || key == ctrl('n') || key == 'j') {
        FIELD* cur = current_field(this->Form);
        size_t findex = field_index(cur);
        if (findex == 3 * this->NumberOfVisibleEntries - 1) {
          continue;
        }
        if (new_page(this->Fields[findex + 1])) {
          form_driver(this->Form, REQ_NEXT_PAGE);
        } else {
          form_driver(this->Form, REQ_NEXT_FIELD);
        }
      }
      // if not beginning of page, previous field, otherwise previous page
      // each entry consists of fields: label, isnew, value
      // therefore, the label field for the prev. entry is index-5
      // and the label field for the next entry is index+1
      // (index always corresponds to the value field)
      // scroll down with arrow up, ctrl+p (emacs binding), or k (vim binding)
      else if (key == KEY_UP || key == ctrl('p') || key == 'k') {
        FIELD* cur = current_field(this->Form);
        int findex = field_index(cur);
        if (findex == 2) {
          continue;
        }
        if (new_page(this->Fields[findex - 2])) {
          form_driver(this->Form, REQ_PREV_PAGE);
          set_current_field(this->Form, this->Fields[findex - 3]);
        } else {
          form_driver(this->Form, REQ_PREV_FIELD);
        }
      }
      // pg down
      else if (key == KEY_NPAGE || key == ctrl('d')) {
        form_driver(this->Form, REQ_NEXT_PAGE);
      }
      // pg up
      else if (key == KEY_PPAGE || key == ctrl('u')) {
        form_driver(this->Form, REQ_PREV_PAGE);
      }
      // configure
      else if (key == 'c') {
        this->Configure();
      }
      // display help
      else if (key == 'h') {
        getmaxyx(stdscr, y, x);

        FIELD* cur = current_field(this->Form);
        int findex = field_index(cur);
        cmCursesWidget* lbl = reinterpret_cast<cmCursesWidget*>(
          field_userptr(this->Fields[findex - 2]));
        const char* curField = lbl->GetValue();
        const char* helpString = CM_NULLPTR;

        const char* existingValue =
          this->CMakeInstance->GetState()->GetCacheEntryValue(curField);
        if (existingValue) {
          helpString = this->CMakeInstance->GetState()->GetCacheEntryProperty(
            curField, "HELPSTRING");
        }
        if (helpString) {
          char* message = new char
            [strlen(curField) + strlen(helpString) +
             strlen(
               "Current option is: \n Help string for this option is: \n") +
             10];
          sprintf(
            message,
            "Current option is: %s\nHelp string for this option is: %s\n",
            curField, helpString);
          this->HelpMessage[1] = message;
          delete[] message;
        } else {
          this->HelpMessage[1] = "";
        }

        cmCursesLongMessageForm* msgs =
          new cmCursesLongMessageForm(this->HelpMessage, "Help.");
        CurrentForm = msgs;
        msgs->Render(1, 1, x, y);
        msgs->HandleInput();
        CurrentForm = this;
        this->Render(1, 1, x, y);
        set_current_field(this->Form, cur);
      }
      // display last errors
      else if (key == 'l') {
        getmaxyx(stdscr, y, x);
        cmCursesLongMessageForm* msgs = new cmCursesLongMessageForm(
          this->Errors, "Errors occurred during the last pass.");
        CurrentForm = msgs;
        msgs->Render(1, 1, x, y);
        msgs->HandleInput();
        CurrentForm = this;
        this->Render(1, 1, x, y);
      } else if (key == '/') {
        this->SearchMode = true;
        this->UpdateStatusBar("Search");
        this->PrintKeys(1);
        touchwin(stdscr);
        refresh();
      } else if (key == 'n') {
        if (!this->OldSearchString.empty()) {
          this->JumpToCacheEntry(this->OldSearchString.c_str());
        }
      }
      // switch advanced on/off
      else if (key == 't') {
        if (this->AdvancedMode) {
          this->AdvancedMode = false;
        } else {
          this->AdvancedMode = true;
        }
        getmaxyx(stdscr, y, x);
        this->RePost();
        this->Render(1, 1, x, y);
      }
      // generate and exit
      else if (key == 'g') {
        if (this->OkToGenerate) {
          this->Generate();
          break;
        }
      }
      // delete cache entry
      else if (key == 'd' && this->NumberOfVisibleEntries) {
        this->OkToGenerate = false;
        FIELD* cur = current_field(this->Form);
        size_t findex = field_index(cur);

        // make the next or prev. current field after deletion
        // each entry consists of fields: label, isnew, value
        // therefore, the label field for the prev. entry is findex-5
        // and the label field for the next entry is findex+1
        // (findex always corresponds to the value field)
        FIELD* nextCur;
        if (findex == 2) {
          nextCur = CM_NULLPTR;
        } else if (findex == 3 * this->NumberOfVisibleEntries - 1) {
          nextCur = this->Fields[findex - 5];
        } else {
          nextCur = this->Fields[findex + 1];
        }

        // Get the label widget
        // each entry consists of fields: label, isnew, value
        // therefore, the label field for the is findex-2
        // (findex always corresponds to the value field)
        cmCursesWidget* lbl = reinterpret_cast<cmCursesWidget*>(
          field_userptr(this->Fields[findex - 2]));
        if (lbl) {
          this->CMakeInstance->GetState()->RemoveCacheEntry(lbl->GetValue());

          std::string nextVal;
          if (nextCur) {
            nextVal =
              (reinterpret_cast<cmCursesWidget*>(field_userptr(nextCur))
                 ->GetValue());
          }

          getmaxyx(stdscr, y, x);
          this->RemoveEntry(lbl->GetValue());
          this->RePost();
          this->Render(1, 1, x, y);

          if (nextCur) {
            // make the next or prev. current field after deletion
            nextCur = CM_NULLPTR;
            std::vector<cmCursesCacheEntryComposite*>::iterator it;
            for (it = this->Entries->begin(); it != this->Entries->end();
                 ++it) {
              if (nextVal == (*it)->Key) {
                nextCur = (*it)->Entry->Field;
              }
            }

            if (nextCur) {
              set_current_field(this->Form, nextCur);
            }
          }
        }
      }
    }

    touchwin(stdscr);
    wrefresh(stdscr);
  }
}

int cmCursesMainForm::LoadCache(const char* /*unused*/)

{
  int r = this->CMakeInstance->LoadCache();
  if (r < 0) {
    return r;
  }
  this->CMakeInstance->SetCacheArgs(this->Args);
  this->CMakeInstance->PreLoadCMakeFiles();
  return r;
}

void cmCursesMainForm::JumpToCacheEntry(const char* astr)
{
  std::string str;
  if (astr) {
    str = cmSystemTools::LowerCase(astr);
  }

  if (str.empty()) {
    return;
  }
  FIELD* cur = current_field(this->Form);
  int start_index = field_index(cur);
  int findex = start_index;
  for (;;) {
    if (!str.empty()) {
      cmCursesWidget* lbl = CM_NULLPTR;
      if (findex >= 0) {
        lbl = reinterpret_cast<cmCursesWidget*>(
          field_userptr(this->Fields[findex - 2]));
      }
      if (lbl) {
        const char* curField = lbl->GetValue();
        if (curField) {
          std::string cfld = cmSystemTools::LowerCase(curField);
          if (cfld.find(str) != std::string::npos && findex != start_index) {
            break;
          }
        }
      }
    }
    if (size_t(findex) >= 3 * this->NumberOfVisibleEntries - 1) {
      set_current_field(this->Form, this->Fields[2]);
    } else if (new_page(this->Fields[findex + 1])) {
      form_driver(this->Form, REQ_NEXT_PAGE);
    } else {
      form_driver(this->Form, REQ_NEXT_FIELD);
    }
    /*
    char buffer[1024];
    sprintf(buffer, "Line: %d != %d / %d\n", findex, idx,
    this->NumberOfVisibleEntries);
    touchwin(stdscr);
    refresh();
    this->UpdateStatusBar( buffer );
    usleep(100000);
    */
    cur = current_field(this->Form);
    findex = field_index(cur);
    if (findex == start_index) {
      break;
    }
  }
}

const char* cmCursesMainForm::s_ConstHelpMessage =
  "CMake is used to configure and generate build files for software projects. "
  "The basic steps for configuring a project with ccmake are as follows:\n\n"
  "1. Run ccmake in the directory where you want the object and executable "
  "files to be placed (build directory). If the source directory is not the "
  "same as this build directory, you have to specify it as an argument on the "
  "command line.\n\n"
  "2. When ccmake is run, it will read the configuration files and display "
  "the current build options. "
  "If you have run CMake before and have updated the configuration files "
  "since then, any new entries will be displayed on top and will be marked "
  "with a *. "
  "On the other hand, the first time you run ccmake, all build options will "
  "be new and will be marked as such. "
  "At this point, you can modify any options (see keys below) you want to "
  "change. "
  "When you are satisfied with your changes, press 'c' to have CMake process "
  "the configuration files. "
  "Please note that changing some options may cause new ones to appear. These "
  "will be shown on top and will be marked with *. "
  "Repeat this procedure until you are satisfied with all the options and "
  "there are no new entries. "
  "At this point, a new command will appear: G)enerate and Exit. You can now "
  "hit 'g' to have CMake generate all the build files (i.e. makefiles or "
  "project files) and exit. "
  "At any point during the process, you can exit ccmake with 'q'. However, "
  "this will not generate/change any build files.\n\n"
  "ccmake KEYS:\n\n"
  "Navigation: "
  "You can use the arrow keys and page up, down to navigate the options. "
  "Alternatively, you can use the following keys: \n"
  " C-n or j : next option\n"
  " C-p or k : previous options\n"
  " C-d : down one page\n"
  " C-u : up one page\n\n"
  "Editing options: "
  "To change an option  press enter or return. If the current options is a "
  "boolean, this will toggle its value. "
  "Otherwise, ccmake will enter edit mode. Alternatively, you can toggle "
  "a bool variable by pressing space, and enter edit mode with i."
  "In this mode you can edit an option using arrow keys and backspace. "
  "Alternatively, you can use the following keys:\n"
  " C-b : back one character\n"
  " C-f : forward one character\n"
  " C-a : go to the beginning of the field\n"
  " C-e : go to the end of the field\n"
  " C-d : delete previous character\n"
  " C-k : kill the rest of the field\n"
  " Esc : Restore field (discard last changes)\n"
  " Enter : Leave edit mode\n"
  "Commands:\n"
  " q : quit ccmake without generating build files\n"
  " h : help, shows this screen\n"
  " c : process the configuration files with the current options\n"
  " g : generate build files and exit, only available when there are no "
  "new options and no errors have been detected during last configuration.\n"
  " l : shows last errors\n"
  " d : delete an option\n"
  " t : toggles advanced mode. In normal mode, only the most important "
  "options are shown. In advanced mode, all options are shown. We recommend "
  "using normal mode unless you are an expert.\n"
  " / : search for a variable name.\n";
