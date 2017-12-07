/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesMainForm_h
#define cmCursesMainForm_h

#include "cmConfigure.h"

#include "cmCursesForm.h"
#include "cmCursesStandardIncludes.h"
#include "cmStateTypes.h"

#include <stddef.h>
#include <string>
#include <vector>

class cmCursesCacheEntryComposite;
class cmake;

/** \class cmCursesMainForm
 * \brief The main page of ccmake
 *
 * cmCursesMainForm is the main page of ccmake.
 */
class cmCursesMainForm : public cmCursesForm
{
  CM_DISABLE_COPY(cmCursesMainForm)

public:
  cmCursesMainForm(std::vector<std::string> const& args, int initwidth);
  ~cmCursesMainForm() CM_OVERRIDE;

  /**
   * Set the widgets which represent the cache entries.
   */
  void InitializeUI();

  /**
   * Handle user input.
   */
  void HandleInput() CM_OVERRIDE;

  /**
   * Display form. Use a window of size width x height, starting
   * at top, left.
   */
  void Render(int left, int top, int width, int height) CM_OVERRIDE;

  /**
   * Returns true if an entry with the given key is in the
   * list of current composites.
   */
  bool LookForCacheEntry(const std::string& key);

  enum
  {
    MIN_WIDTH = 65,
    MIN_HEIGHT = 6,
    IDEAL_WIDTH = 80,
    MAX_WIDTH = 512
  };

  /**
   * This method should normally be called only by the form.  The only
   * exception is during a resize. The optional argument specifies the
   * string to be displayed in the status bar.
   */
  void UpdateStatusBar() CM_OVERRIDE { this->UpdateStatusBar(CM_NULLPTR); }
  virtual void UpdateStatusBar(const char* message);

  /**
   * Display current commands and their keys on the toolbar.  This
   * method should normally called only by the form.  The only
   * exception is during a resize. If the optional argument process is
   * specified and is either 1 (configure) or 2 (generate), then keys
   * will be displayed accordingly.
   */
  void PrintKeys(int process = 0);

  /**
   * During a CMake run, an error handle should add errors
   * to be displayed afterwards.
   */
  void AddError(const char* message, const char* title) CM_OVERRIDE;

  /**
   * Used to do a configure. If argument is specified, it does only the check
   * and not configure.
   */
  int Configure(int noconfigure = 0);

  /**
   * Used to generate
   */
  int Generate();

  /**
   * Used by main program
   */
  int LoadCache(const char* dir);

  /**
   * Progress callback
   */
  static void UpdateProgressOld(const char* msg, float prog, void*);
  static void UpdateProgress(const char* msg, float prog, void*);

protected:
  // Copy the cache values from the user interface to the actual
  // cache.
  void FillCacheManagerFromUI();
  // Fix formatting of values to a consistent form.
  void FixValue(cmStateEnums::CacheEntryType type, const std::string& in,
                std::string& out) const;
  // Re-post the existing fields. Used to toggle between
  // normal and advanced modes. Render() should be called
  // afterwards.
  void RePost();
  // Remove an entry from the interface and the cache.
  void RemoveEntry(const char* value);

  // Jump to the cache entry whose name matches the string.
  void JumpToCacheEntry(const char* str);

  // Copies of cache entries stored in the user interface
  std::vector<cmCursesCacheEntryComposite*>* Entries;
  // Errors produced during last run of cmake
  std::vector<std::string> Errors;
  // Command line argumens to be passed to cmake each time
  // it is run
  std::vector<std::string> Args;
  // Message displayed when user presses 'h'
  // It is: Welcome + info about current entry + common help
  std::vector<std::string> HelpMessage;

  // Common help
  static const char* s_ConstHelpMessage;

  // Fields displayed. Includes labels, new entry markers, entries
  FIELD** Fields;
  // Where is source of current project
  std::string WhereSource;
  // Where is cmake executable
  std::string WhereCMake;
  // Number of entries shown (depends on mode -normal or advanced-)
  size_t NumberOfVisibleEntries;
  bool AdvancedMode;
  // Did the iteration converge (no new entries) ?
  bool OkToGenerate;
  // Number of pages displayed
  int NumberOfPages;

  int InitialWidth;
  cmake* CMakeInstance;

  std::string SearchString;
  std::string OldSearchString;
  bool SearchMode;
};

#endif // cmCursesMainForm_h
