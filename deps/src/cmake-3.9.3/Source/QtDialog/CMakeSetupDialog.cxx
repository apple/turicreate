/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "CMakeSetupDialog.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>

#include "AddCacheEntry.h"
#include "FirstConfigure.h"
#include "QCMake.h"
#include "QCMakeCacheView.h"
#include "RegexExplorer.h"
#include "WarningMessagesDialog.h"
#include "cmSystemTools.h"
#include "cmVersion.h"

QCMakeThread::QCMakeThread(QObject* p)
  : QThread(p)
  , CMakeInstance(CM_NULLPTR)
{
}

QCMake* QCMakeThread::cmakeInstance() const
{
  return this->CMakeInstance;
}

void QCMakeThread::run()
{
  this->CMakeInstance = new QCMake;
  // emit that this cmake thread is ready for use
  emit this->cmakeInitialized();
  this->exec();
  delete this->CMakeInstance;
  this->CMakeInstance = CM_NULLPTR;
}

CMakeSetupDialog::CMakeSetupDialog()
  : ExitAfterGenerate(true)
  , CacheModified(false)
  , ConfigureNeeded(true)
  , CurrentState(Interrupting)
{
  QString title = QString(tr("CMake %1"));
  title = title.arg(cmVersion::GetCMakeVersion());
  this->setWindowTitle(title);

  // create the GUI
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("windowState").toByteArray());

  this->AddVariableNames =
    settings.value("AddVariableNames", QStringList("CMAKE_INSTALL_PREFIX"))
      .toStringList();
  this->AddVariableTypes =
    settings.value("AddVariableTypes", QStringList("PATH")).toStringList();

  QWidget* cont = new QWidget(this);
  this->setupUi(cont);
  this->Splitter->setStretchFactor(0, 3);
  this->Splitter->setStretchFactor(1, 1);
  this->setCentralWidget(cont);
  this->ProgressBar->reset();
  this->RemoveEntry->setEnabled(false);
  this->AddEntry->setEnabled(false);

  QByteArray p = settings.value("SplitterSizes").toByteArray();
  this->Splitter->restoreState(p);

  bool groupView = settings.value("GroupView", false).toBool();
  this->setGroupedView(groupView);
  this->groupedCheck->setCheckState(groupView ? Qt::Checked : Qt::Unchecked);

  bool advancedView = settings.value("AdvancedView", false).toBool();
  this->setAdvancedView(advancedView);
  this->advancedCheck->setCheckState(advancedView ? Qt::Checked
                                                  : Qt::Unchecked);

  QMenu* FileMenu = this->menuBar()->addMenu(tr("&File"));
  this->ReloadCacheAction = FileMenu->addAction(tr("&Reload Cache"));
  QObject::connect(this->ReloadCacheAction, SIGNAL(triggered(bool)), this,
                   SLOT(doReloadCache()));
  this->DeleteCacheAction = FileMenu->addAction(tr("&Delete Cache"));
  QObject::connect(this->DeleteCacheAction, SIGNAL(triggered(bool)), this,
                   SLOT(doDeleteCache()));
  this->ExitAction = FileMenu->addAction(tr("E&xit"));
  this->ExitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
  QObject::connect(this->ExitAction, SIGNAL(triggered(bool)), this,
                   SLOT(close()));

  QMenu* ToolsMenu = this->menuBar()->addMenu(tr("&Tools"));
  this->ConfigureAction = ToolsMenu->addAction(tr("&Configure"));
  // prevent merging with Preferences menu item on Mac OS X
  this->ConfigureAction->setMenuRole(QAction::NoRole);
  QObject::connect(this->ConfigureAction, SIGNAL(triggered(bool)), this,
                   SLOT(doConfigure()));
  this->GenerateAction = ToolsMenu->addAction(tr("&Generate"));
  QObject::connect(this->GenerateAction, SIGNAL(triggered(bool)), this,
                   SLOT(doGenerate()));
  QAction* showChangesAction = ToolsMenu->addAction(tr("&Show My Changes"));
  QObject::connect(showChangesAction, SIGNAL(triggered(bool)), this,
                   SLOT(showUserChanges()));
#if defined(Q_WS_MAC) || defined(Q_OS_MAC)
  this->InstallForCommandLineAction =
    ToolsMenu->addAction(tr("&How to Install For Command Line Use"));
  QObject::connect(this->InstallForCommandLineAction, SIGNAL(triggered(bool)),
                   this, SLOT(doInstallForCommandLine()));
#endif
  ToolsMenu->addSeparator();
  ToolsMenu->addAction(tr("Regular Expression Explorer..."), this,
                       SLOT(doRegexExplorerDialog()));
  ToolsMenu->addSeparator();
  ToolsMenu->addAction(tr("&Find in Output..."), this,
                       SLOT(doOutputFindDialog()), QKeySequence::Find);
  ToolsMenu->addAction(tr("Find Next"), this, SLOT(doOutputFindNext()),
                       QKeySequence::FindNext);
  ToolsMenu->addAction(tr("Find Previous"), this, SLOT(doOutputFindPrev()),
                       QKeySequence::FindPrevious);
  ToolsMenu->addAction(tr("Goto Next Error"), this, SLOT(doOutputErrorNext()),
                       QKeySequence(Qt::Key_F8)); // in Visual Studio
  new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Period), this,
                SLOT(doOutputErrorNext())); // in Eclipse

  QMenu* OptionsMenu = this->menuBar()->addMenu(tr("&Options"));
  OptionsMenu->addAction(tr("Warning Messages..."), this,
                         SLOT(doWarningMessagesDialog()));
  this->WarnUninitializedAction =
    OptionsMenu->addAction(tr("&Warn Uninitialized (--warn-uninitialized)"));
  this->WarnUninitializedAction->setCheckable(true);
  this->WarnUnusedAction =
    OptionsMenu->addAction(tr("&Warn Unused (--warn-unused-vars)"));
  this->WarnUnusedAction->setCheckable(true);

  QAction* debugAction = OptionsMenu->addAction(tr("&Debug Output"));
  debugAction->setCheckable(true);
  QObject::connect(debugAction, SIGNAL(toggled(bool)), this,
                   SLOT(setDebugOutput(bool)));

  OptionsMenu->addSeparator();
  QAction* expandAction =
    OptionsMenu->addAction(tr("&Expand Grouped Entries"));
  QObject::connect(expandAction, SIGNAL(triggered(bool)), this->CacheValues,
                   SLOT(expandAll()));
  QAction* collapseAction =
    OptionsMenu->addAction(tr("&Collapse Grouped Entries"));
  QObject::connect(collapseAction, SIGNAL(triggered(bool)), this->CacheValues,
                   SLOT(collapseAll()));

  QMenu* HelpMenu = this->menuBar()->addMenu(tr("&Help"));
  QAction* a = HelpMenu->addAction(tr("About"));
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(doAbout()));
  a = HelpMenu->addAction(tr("Help"));
  QObject::connect(a, SIGNAL(triggered(bool)), this, SLOT(doHelp()));

  this->setAcceptDrops(true);

  // get the saved binary directories
  QStringList buildPaths = this->loadBuildPaths();
  this->BinaryDirectory->addItems(buildPaths);

  this->BinaryDirectory->setCompleter(new QCMakeFileCompleter(this, true));
  this->SourceDirectory->setCompleter(new QCMakeFileCompleter(this, true));

  // fixed pitch font in output window
  QFont outputFont("Courier");
  this->Output->setFont(outputFont);
  this->ErrorFormat.setForeground(QBrush(Qt::red));

  this->Output->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this->Output, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(doOutputContextMenu(const QPoint&)));

  // start the cmake worker thread
  this->CMakeThread = new QCMakeThread(this);
  QObject::connect(this->CMakeThread, SIGNAL(cmakeInitialized()), this,
                   SLOT(initialize()), Qt::QueuedConnection);
  this->CMakeThread->start();

  this->enterState(ReadyConfigure);

  ProgressOffset = 0.0;
  ProgressFactor = 1.0;
}

void CMakeSetupDialog::initialize()
{
  // now the cmake worker thread is running, lets make our connections to it
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(propertiesChanged(const QCMakePropertyList&)),
                   this->CacheValues->cacheModel(),
                   SLOT(setProperties(const QCMakePropertyList&)));

  QObject::connect(this->ConfigureButton, SIGNAL(clicked(bool)), this,
                   SLOT(doConfigure()));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(configureDone(int)), this, SLOT(exitLoop(int)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(generateDone(int)), this, SLOT(exitLoop(int)));

  QObject::connect(this->GenerateButton, SIGNAL(clicked(bool)), this,
                   SLOT(doGenerate()));
  QObject::connect(this->OpenProjectButton, SIGNAL(clicked(bool)), this,
                   SLOT(doOpenProject()));

  QObject::connect(this->BrowseSourceDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doSourceBrowse()));
  QObject::connect(this->BrowseBinaryDirectoryButton, SIGNAL(clicked(bool)),
                   this, SLOT(doBinaryBrowse()));

  QObject::connect(this->BinaryDirectory, SIGNAL(editTextChanged(QString)),
                   this, SLOT(onBinaryDirectoryChanged(QString)));
  QObject::connect(this->SourceDirectory, SIGNAL(textChanged(QString)), this,
                   SLOT(onSourceDirectoryChanged(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(sourceDirChanged(QString)), this,
                   SLOT(updateSourceDirectory(QString)));
  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(binaryDirChanged(QString)), this,
                   SLOT(updateBinaryDirectory(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(progressChanged(QString, float)), this,
                   SLOT(showProgress(QString, float)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(errorMessage(QString)), this, SLOT(error(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(outputMessage(QString)), this,
                   SLOT(message(QString)));

  QObject::connect(this->groupedCheck, SIGNAL(toggled(bool)), this,
                   SLOT(setGroupedView(bool)));
  QObject::connect(this->advancedCheck, SIGNAL(toggled(bool)), this,
                   SLOT(setAdvancedView(bool)));
  QObject::connect(this->Search, SIGNAL(textChanged(QString)), this,
                   SLOT(setSearchFilter(QString)));

  QObject::connect(this->CMakeThread->cmakeInstance(),
                   SIGNAL(generatorChanged(QString)), this,
                   SLOT(updateGeneratorLabel(QString)));
  this->updateGeneratorLabel(QString());

  QObject::connect(this->CacheValues->cacheModel(),
                   SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
                   SLOT(setCacheModified()));

  QObject::connect(this->CacheValues->selectionModel(),
                   SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
                   this, SLOT(selectionChanged()));
  QObject::connect(this->RemoveEntry, SIGNAL(clicked(bool)), this,
                   SLOT(removeSelectedCacheEntries()));
  QObject::connect(this->AddEntry, SIGNAL(clicked(bool)), this,
                   SLOT(addCacheEntry()));

  QObject::connect(this->WarnUninitializedAction, SIGNAL(triggered(bool)),
                   this->CMakeThread->cmakeInstance(),
                   SLOT(setWarnUninitializedMode(bool)));
  QObject::connect(this->WarnUnusedAction, SIGNAL(triggered(bool)),
                   this->CMakeThread->cmakeInstance(),
                   SLOT(setWarnUnusedMode(bool)));

  if (!this->SourceDirectory->text().isEmpty() ||
      !this->BinaryDirectory->lineEdit()->text().isEmpty()) {
    this->onSourceDirectoryChanged(this->SourceDirectory->text());
    this->onBinaryDirectoryChanged(this->BinaryDirectory->lineEdit()->text());
  } else {
    this->onBinaryDirectoryChanged(this->BinaryDirectory->lineEdit()->text());
  }
}

CMakeSetupDialog::~CMakeSetupDialog()
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  settings.setValue("windowState", QVariant(saveState()));
  settings.setValue("geometry", QVariant(saveGeometry()));
  settings.setValue("SplitterSizes", this->Splitter->saveState());

  // wait for thread to stop
  this->CMakeThread->quit();
  this->CMakeThread->wait();
}

bool CMakeSetupDialog::prepareConfigure()
{
  // make sure build directory exists
  QString bindir = this->CMakeThread->cmakeInstance()->binaryDirectory();
  QDir dir(bindir);
  if (!dir.exists()) {
    QString msg = tr("Build directory does not exist, "
                     "should I create it?\n\n"
                     "Directory: ");
    msg += bindir;
    QString title = tr("Create Directory");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::information(this, title, msg,
                                   QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::No) {
      return false;
    }
    if (!dir.mkpath(".")) {
      QMessageBox::information(
        this, tr("Create Directory Failed"),
        QString(tr("Failed to create directory %1")).arg(dir.path()),
        QMessageBox::Ok);

      return false;
    }
  }

  // if no generator, prompt for it and other setup stuff
  if (this->CMakeThread->cmakeInstance()->generator().isEmpty()) {
    if (!this->setupFirstConfigure()) {
      return false;
    }
  }

  // remember path
  this->addBinaryPath(dir.absolutePath());

  return true;
}

void CMakeSetupDialog::exitLoop(int err)
{
  this->LocalLoop.exit(err);
}

void CMakeSetupDialog::doConfigure()
{
  if (this->CurrentState == Configuring) {
    // stop configure
    doInterrupt();
    return;
  }

  if (!prepareConfigure()) {
    return;
  }

  this->enterState(Configuring);

  bool ret = doConfigureInternal();

  if (ret) {
    this->ConfigureNeeded = false;
  }

  if (ret && !this->CacheValues->cacheModel()->newPropertyCount()) {
    this->enterState(ReadyGenerate);
  } else {
    this->enterState(ReadyConfigure);
    this->CacheValues->scrollToTop();
  }
  this->ProgressBar->reset();
}

bool CMakeSetupDialog::doConfigureInternal()
{
  this->Output->clear();
  this->CacheValues->selectionModel()->clear();

  QMetaObject::invokeMethod(
    this->CMakeThread->cmakeInstance(), "setProperties", Qt::QueuedConnection,
    Q_ARG(QCMakePropertyList, this->CacheValues->cacheModel()->properties()));
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(), "configure",
                            Qt::QueuedConnection);

  int err = this->LocalLoop.exec();

  if (err != 0) {
    QMessageBox::critical(
      this, tr("Error"),
      tr("Error in configuration process, project files may be invalid"),
      QMessageBox::Ok);
  }

  return 0 == err;
}

void CMakeSetupDialog::doInstallForCommandLine()
{
  QString title = tr("How to Install For Command Line Use");
  QString msg = tr("One may add CMake to the PATH:\n"
                   "\n"
                   " PATH=\"%1\":\"$PATH\"\n"
                   "\n"
                   "Or, to install symlinks to '/usr/local/bin', run:\n"
                   "\n"
                   " sudo \"%2\" --install\n"
                   "\n"
                   "Or, to install symlinks to another directory, run:\n"
                   "\n"
                   " sudo \"%3\" --install=/path/to/bin\n");
  msg = msg.arg(
    cmSystemTools::GetFilenamePath(cmSystemTools::GetCMakeCommand()).c_str());
  msg = msg.arg(cmSystemTools::GetCMakeGUICommand().c_str());
  msg = msg.arg(cmSystemTools::GetCMakeGUICommand().c_str());

  QDialog dialog;
  dialog.setWindowTitle(title);
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QLabel* lab = new QLabel(&dialog);
  l->addWidget(lab);
  lab->setText(msg);
  lab->setWordWrap(false);
  lab->setTextInteractionFlags(Qt::TextSelectableByMouse);
  QDialogButtonBox* btns =
    new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  l->addWidget(btns);
  dialog.exec();
}

bool CMakeSetupDialog::doGenerateInternal()
{
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(), "generate",
                            Qt::QueuedConnection);

  int err = this->LocalLoop.exec();

  if (err != 0) {
    QMessageBox::critical(
      this, tr("Error"),
      tr("Error in generation process, project files may be invalid"),
      QMessageBox::Ok);
  }

  return 0 == err;
}

void CMakeSetupDialog::doGenerate()
{
  if (this->CurrentState == Generating) {
    // stop generate
    doInterrupt();
    return;
  }

  // see if we need to configure
  // we'll need to configure if:
  //   the configure step hasn't been done yet
  //   generate was the last step done
  if (this->ConfigureNeeded) {
    if (!prepareConfigure()) {
      return;
    }
  }

  this->enterState(Generating);

  bool config_passed = true;
  if (this->ConfigureNeeded) {
    this->CacheValues->cacheModel()->setShowNewProperties(false);
    this->ProgressFactor = 0.5;
    config_passed = doConfigureInternal();
    this->ProgressOffset = 0.5;
  }

  if (config_passed) {
    doGenerateInternal();
  }

  this->ProgressOffset = 0.0;
  this->ProgressFactor = 1.0;
  this->CacheValues->cacheModel()->setShowNewProperties(true);

  this->enterState(ReadyConfigure);
  this->ProgressBar->reset();

  this->ConfigureNeeded = true;
}

QString CMakeSetupDialog::getProjectFilename()
{
  QStringList nameFilter;
  nameFilter << "*.sln"
             << "*.xcodeproj";
  QDir directory(this->BinaryDirectory->currentText());
  QStringList nlnFile = directory.entryList(nameFilter);

  if (nlnFile.count() == 1) {
    return this->BinaryDirectory->currentText() + "/" + nlnFile.at(0);
  }

  return QString();
}

void CMakeSetupDialog::doOpenProject()
{
  QDesktopServices::openUrl(QUrl::fromLocalFile(this->getProjectFilename()));
}

void CMakeSetupDialog::closeEvent(QCloseEvent* e)
{
  // prompt for close if there are unsaved changes, and we're not busy
  if (this->CacheModified) {
    QString msg = tr("You have changed options but not rebuilt, "
                     "are you sure you want to exit?");
    QString title = tr("Confirm Exit");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::critical(this, title, msg,
                                QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::No) {
      e->ignore();
    }
  }

  // don't close if we're busy, unless the user really wants to
  if (this->CurrentState == Configuring) {
    QString msg =
      tr("You are in the middle of a Configure.\n"
         "If you Exit now the configure information will be lost.\n"
         "Are you sure you want to Exit?");
    QString title = tr("Confirm Exit");
    QMessageBox::StandardButton btn;
    btn = QMessageBox::critical(this, title, msg,
                                QMessageBox::Yes | QMessageBox::No);
    if (btn == QMessageBox::No) {
      e->ignore();
    } else {
      this->doInterrupt();
    }
  }

  // let the generate finish
  if (this->CurrentState == Generating) {
    e->ignore();
  }
}

void CMakeSetupDialog::doHelp()
{
  QString msg = tr(
    "CMake is used to configure and generate build files for "
    "software projects.   The basic steps for configuring a project are as "
    "follows:\r\n\r\n1. Select the source directory for the project.  This "
    "should "
    "contain the CMakeLists.txt files for the project.\r\n\r\n2. Select the "
    "build "
    "directory for the project.   This is the directory where the project "
    "will be "
    "built.  It can be the same or a different directory than the source "
    "directory.   For easy clean up, a separate build directory is "
    "recommended. "
    "CMake will create the directory if it does not exist.\r\n\r\n3. Once the "
    "source and binary directories are selected, it is time to press the "
    "Configure button.  This will cause CMake to read all of the input files "
    "and "
    "discover all the variables used by the project.   The first time a "
    "variable "
    "is displayed it will be in Red.   Users should inspect red variables "
    "making "
    "sure the values are correct.   For some projects the Configure process "
    "can "
    "be iterative, so continue to press the Configure button until there are "
    "no "
    "longer red entries.\r\n\r\n4. Once there are no longer red entries, you "
    "should click the Generate button.  This will write the build files to "
    "the build "
    "directory.");

  QDialog dialog;
  QFontMetrics met(this->font());
  int msgWidth = met.width(msg);
  dialog.setMinimumSize(msgWidth / 15, 20);
  dialog.setWindowTitle(tr("Help"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QLabel* lab = new QLabel(&dialog);
  lab->setText(msg);
  lab->setWordWrap(true);
  QDialogButtonBox* btns =
    new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  l->addWidget(lab);
  l->addWidget(btns);
  dialog.exec();
}

void CMakeSetupDialog::doInterrupt()
{
  this->enterState(Interrupting);
  this->CMakeThread->cmakeInstance()->interrupt();
}

void CMakeSetupDialog::doSourceBrowse()
{
  QString dir = QFileDialog::getExistingDirectory(
    this, tr("Enter Path to Source"), this->SourceDirectory->text(),
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!dir.isEmpty()) {
    this->setSourceDirectory(dir);
  }
}

void CMakeSetupDialog::updateSourceDirectory(const QString& dir)
{
  if (this->SourceDirectory->text() != dir) {
    this->SourceDirectory->blockSignals(true);
    this->SourceDirectory->setText(dir);
    this->SourceDirectory->blockSignals(false);
  }
}

void CMakeSetupDialog::updateBinaryDirectory(const QString& dir)
{
  if (this->BinaryDirectory->currentText() != dir) {
    this->BinaryDirectory->blockSignals(true);
    this->BinaryDirectory->setEditText(dir);
    this->BinaryDirectory->blockSignals(false);
  }
  if (!this->getProjectFilename().isEmpty()) {
    this->OpenProjectButton->setEnabled(true);
  } else {
    this->OpenProjectButton->setEnabled(false);
  }
}

void CMakeSetupDialog::doBinaryBrowse()
{
  QString dir = QFileDialog::getExistingDirectory(
    this, tr("Enter Path to Build"), this->BinaryDirectory->currentText(),
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  if (!dir.isEmpty() && dir != this->BinaryDirectory->currentText()) {
    this->setBinaryDirectory(dir);
  }
}

void CMakeSetupDialog::setBinaryDirectory(const QString& dir)
{
  this->BinaryDirectory->setEditText(dir);
}

void CMakeSetupDialog::onSourceDirectoryChanged(const QString& dir)
{
  this->Output->clear();
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
                            "setSourceDirectory", Qt::QueuedConnection,
                            Q_ARG(QString, dir));
}

void CMakeSetupDialog::onBinaryDirectoryChanged(const QString& dir)
{
  QString title = QString(tr("CMake %1 - %2"));
  title = title.arg(cmVersion::GetCMakeVersion());
  title = title.arg(dir);
  this->setWindowTitle(title);

  this->CacheModified = false;
  this->CacheValues->cacheModel()->clear();
  qobject_cast<QCMakeCacheModelDelegate*>(this->CacheValues->itemDelegate())
    ->clearChanges();
  this->Output->clear();
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
                            "setBinaryDirectory", Qt::QueuedConnection,
                            Q_ARG(QString, dir));
}

void CMakeSetupDialog::setSourceDirectory(const QString& dir)
{
  this->SourceDirectory->setText(dir);
}

void CMakeSetupDialog::showProgress(const QString& /*msg*/, float percent)
{
  percent = (percent * ProgressFactor) + ProgressOffset;
  this->ProgressBar->setValue(qRound(percent * 100));
}

void CMakeSetupDialog::error(const QString& msg)
{
  this->Output->setCurrentCharFormat(this->ErrorFormat);
  // QTextEdit will terminate the msg with a ParagraphSeparator, but it also
  // replaces
  // all newlines with ParagraphSeparators. By replacing the newlines by
  // ourself, one
  // error msg will be one paragraph.
  QString paragraph(msg);
  paragraph.replace(QLatin1Char('\n'), QChar::LineSeparator);
  this->Output->append(paragraph);
}

void CMakeSetupDialog::message(const QString& msg)
{
  this->Output->setCurrentCharFormat(this->MessageFormat);
  this->Output->append(msg);
}

void CMakeSetupDialog::setEnabledState(bool enabled)
{
  // disable parts of the GUI during configure/generate
  this->CacheValues->cacheModel()->setEditEnabled(enabled);
  this->SourceDirectory->setEnabled(enabled);
  this->BrowseSourceDirectoryButton->setEnabled(enabled);
  this->BinaryDirectory->setEnabled(enabled);
  this->BrowseBinaryDirectoryButton->setEnabled(enabled);
  this->ReloadCacheAction->setEnabled(enabled);
  this->DeleteCacheAction->setEnabled(enabled);
  this->ExitAction->setEnabled(enabled);
  this->ConfigureAction->setEnabled(enabled);
  this->AddEntry->setEnabled(enabled);
  this->RemoveEntry->setEnabled(false); // let selection re-enable it
}

bool CMakeSetupDialog::setupFirstConfigure()
{
  FirstConfigure dialog;

  // initialize dialog and restore saved settings

  // add generators
  dialog.setGenerators(
    this->CMakeThread->cmakeInstance()->availableGenerators());

  // restore from settings
  dialog.loadFromSettings();

  if (dialog.exec() == QDialog::Accepted) {
    dialog.saveToSettings();
    this->CMakeThread->cmakeInstance()->setGenerator(dialog.getGenerator());
    this->CMakeThread->cmakeInstance()->setToolset(dialog.getToolset());

    QCMakeCacheModel* m = this->CacheValues->cacheModel();

    if (dialog.compilerSetup()) {
      QString fortranCompiler = dialog.getFortranCompiler();
      if (!fortranCompiler.isEmpty()) {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_Fortran_COMPILER",
                          "Fortran compiler.", fortranCompiler, false);
      }
      QString cxxCompiler = dialog.getCXXCompiler();
      if (!cxxCompiler.isEmpty()) {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_CXX_COMPILER",
                          "CXX compiler.", cxxCompiler, false);
      }

      QString cCompiler = dialog.getCCompiler();
      if (!cCompiler.isEmpty()) {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_C_COMPILER",
                          "C compiler.", cCompiler, false);
      }
    } else if (dialog.crossCompilerSetup()) {
      QString fortranCompiler = dialog.getFortranCompiler();
      if (!fortranCompiler.isEmpty()) {
        m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_Fortran_COMPILER",
                          "Fortran compiler.", fortranCompiler, false);
      }

      QString mode = dialog.getCrossIncludeMode();
      m->insertProperty(QCMakeProperty::STRING,
                        "CMAKE_FIND_ROOT_PATH_MODE_INCLUDE",
                        tr("CMake Find Include Mode"), mode, false);
      mode = dialog.getCrossLibraryMode();
      m->insertProperty(QCMakeProperty::STRING,
                        "CMAKE_FIND_ROOT_PATH_MODE_LIBRARY",
                        tr("CMake Find Library Mode"), mode, false);
      mode = dialog.getCrossProgramMode();
      m->insertProperty(QCMakeProperty::STRING,
                        "CMAKE_FIND_ROOT_PATH_MODE_PROGRAM",
                        tr("CMake Find Program Mode"), mode, false);

      QString rootPath = dialog.getCrossRoot();
      m->insertProperty(QCMakeProperty::PATH, "CMAKE_FIND_ROOT_PATH",
                        tr("CMake Find Root Path"), rootPath, false);

      QString systemName = dialog.getSystemName();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_SYSTEM_NAME",
                        tr("CMake System Name"), systemName, false);
      QString systemVersion = dialog.getSystemVersion();
      m->insertProperty(QCMakeProperty::STRING, "CMAKE_SYSTEM_VERSION",
                        tr("CMake System Version"), systemVersion, false);
      QString cxxCompiler = dialog.getCXXCompiler();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_CXX_COMPILER",
                        tr("CXX compiler."), cxxCompiler, false);
      QString cCompiler = dialog.getCCompiler();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_C_COMPILER",
                        tr("C compiler."), cCompiler, false);
    } else if (dialog.crossCompilerToolChainFile()) {
      QString toolchainFile = dialog.getCrossCompilerToolChainFile();
      m->insertProperty(QCMakeProperty::FILEPATH, "CMAKE_TOOLCHAIN_FILE",
                        tr("Cross Compile ToolChain File"), toolchainFile,
                        false);
    }
    return true;
  }

  return false;
}

void CMakeSetupDialog::updateGeneratorLabel(const QString& gen)
{
  QString str = tr("Current Generator: ");
  if (gen.isEmpty()) {
    str += tr("None");
  } else {
    str += gen;
  }
  this->Generator->setText(str);
}

void CMakeSetupDialog::doReloadCache()
{
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(), "reloadCache",
                            Qt::QueuedConnection);
}

void CMakeSetupDialog::doDeleteCache()
{
  QString title = tr("Delete Cache");
  QString msg = tr("Are you sure you want to delete the cache?");
  QMessageBox::StandardButton btn;
  btn = QMessageBox::information(this, title, msg,
                                 QMessageBox::Yes | QMessageBox::No);
  if (btn == QMessageBox::No) {
    return;
  }
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(), "deleteCache",
                            Qt::QueuedConnection);
}

void CMakeSetupDialog::doAbout()
{
  QString msg = tr(
    "CMake %1 (cmake.org).\n"
    "CMake suite maintained and supported by Kitware (kitware.com/cmake).\n"
    "Distributed under terms of the BSD 3-Clause License.\n"
    "\n"
    "CMake GUI maintained by csimsoft,\n"
    "built using Qt %2 (qt-project.org).\n"
#ifdef USE_LGPL
    "\n"
    "The Qt Toolkit is Copyright (C) Digia Plc and/or its subsidiary(-ies).\n"
    "Qt is licensed under terms of the GNU LGPLv" USE_LGPL ", available at:\n"
    " \"%3\""
#endif
    );
  msg = msg.arg(cmVersion::GetCMakeVersion());
  msg = msg.arg(qVersion());
#ifdef USE_LGPL
  std::string lgpl =
    cmSystemTools::GetCMakeRoot() + "/Licenses/LGPLv" USE_LGPL ".txt";
  msg = msg.arg(lgpl.c_str());
#endif

  QDialog dialog;
  dialog.setWindowTitle(tr("About"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QLabel* lab = new QLabel(&dialog);
  l->addWidget(lab);
  lab->setText(msg);
  lab->setWordWrap(true);
  QDialogButtonBox* btns =
    new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  l->addWidget(btns);
  dialog.exec();
}

void CMakeSetupDialog::setExitAfterGenerate(bool b)
{
  this->ExitAfterGenerate = b;
}

void CMakeSetupDialog::addBinaryPath(const QString& path)
{
  QString cleanpath = QDir::cleanPath(path);

  // update UI
  this->BinaryDirectory->blockSignals(true);
  int idx = this->BinaryDirectory->findText(cleanpath);
  if (idx != -1) {
    this->BinaryDirectory->removeItem(idx);
  }
  this->BinaryDirectory->insertItem(0, cleanpath);
  this->BinaryDirectory->setCurrentIndex(0);
  this->BinaryDirectory->blockSignals(false);

  // save to registry
  QStringList buildPaths = this->loadBuildPaths();
  buildPaths.removeAll(cleanpath);
  buildPaths.prepend(cleanpath);
  this->saveBuildPaths(buildPaths);
}

void CMakeSetupDialog::dragEnterEvent(QDragEnterEvent* e)
{
  if (!(this->CurrentState == ReadyConfigure ||
        this->CurrentState == ReadyGenerate)) {
    e->ignore();
    return;
  }

  const QMimeData* dat = e->mimeData();
  QList<QUrl> urls = dat->urls();
  QString file = urls.count() ? urls[0].toLocalFile() : QString();
  if (!file.isEmpty() &&
      (file.endsWith("CMakeCache.txt", Qt::CaseInsensitive) ||
       file.endsWith("CMakeLists.txt", Qt::CaseInsensitive))) {
    e->accept();
  } else {
    e->ignore();
  }
}

void CMakeSetupDialog::dropEvent(QDropEvent* e)
{
  if (!(this->CurrentState == ReadyConfigure ||
        this->CurrentState == ReadyGenerate)) {
    return;
  }

  const QMimeData* dat = e->mimeData();
  QList<QUrl> urls = dat->urls();
  QString file = urls.count() ? urls[0].toLocalFile() : QString();
  if (file.endsWith("CMakeCache.txt", Qt::CaseInsensitive)) {
    QFileInfo info(file);
    if (this->CMakeThread->cmakeInstance()->binaryDirectory() !=
        info.absolutePath()) {
      this->setBinaryDirectory(info.absolutePath());
    }
  } else if (file.endsWith("CMakeLists.txt", Qt::CaseInsensitive)) {
    QFileInfo info(file);
    if (this->CMakeThread->cmakeInstance()->binaryDirectory() !=
        info.absolutePath()) {
      this->setSourceDirectory(info.absolutePath());
      this->setBinaryDirectory(info.absolutePath());
    }
  }
}

QStringList CMakeSetupDialog::loadBuildPaths()
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");

  QStringList buildPaths;
  for (int i = 0; i < 10; i++) {
    QString p = settings.value(QString("WhereBuild%1").arg(i)).toString();
    if (!p.isEmpty()) {
      buildPaths.append(p);
    }
  }
  return buildPaths;
}

void CMakeSetupDialog::saveBuildPaths(const QStringList& paths)
{
  QSettings settings;
  settings.beginGroup("Settings/StartPath");

  int num = paths.count();
  if (num > 10) {
    num = 10;
  }

  for (int i = 0; i < num; i++) {
    settings.setValue(QString("WhereBuild%1").arg(i), paths[i]);
  }
}

void CMakeSetupDialog::setCacheModified()
{
  this->CacheModified = true;
  this->ConfigureNeeded = true;
  this->enterState(ReadyConfigure);
}

void CMakeSetupDialog::removeSelectedCacheEntries()
{
  QModelIndexList idxs = this->CacheValues->selectionModel()->selectedRows();
  QList<QPersistentModelIndex> pidxs;
  foreach (QModelIndex const& i, idxs) {
    pidxs.append(i);
  }
  foreach (QPersistentModelIndex const& pi, pidxs) {
    this->CacheValues->model()->removeRow(pi.row(), pi.parent());
  }
}

void CMakeSetupDialog::selectionChanged()
{
  QModelIndexList idxs = this->CacheValues->selectionModel()->selectedRows();
  if (idxs.count() && (this->CurrentState == ReadyConfigure ||
                       this->CurrentState == ReadyGenerate)) {
    this->RemoveEntry->setEnabled(true);
  } else {
    this->RemoveEntry->setEnabled(false);
  }
}

void CMakeSetupDialog::enterState(CMakeSetupDialog::State s)
{
  if (s == this->CurrentState) {
    return;
  }

  this->CurrentState = s;

  if (s == Interrupting) {
    this->ConfigureButton->setEnabled(false);
    this->GenerateButton->setEnabled(false);
    this->OpenProjectButton->setEnabled(false);
  } else if (s == Configuring) {
    this->setEnabledState(false);
    this->GenerateButton->setEnabled(false);
    this->GenerateAction->setEnabled(false);
    this->OpenProjectButton->setEnabled(false);
    this->ConfigureButton->setText(tr("&Stop"));
  } else if (s == Generating) {
    this->CacheModified = false;
    this->setEnabledState(false);
    this->ConfigureButton->setEnabled(false);
    this->GenerateAction->setEnabled(false);
    this->OpenProjectButton->setEnabled(false);
    this->GenerateButton->setText(tr("&Stop"));
  } else if (s == ReadyConfigure) {
    this->setEnabledState(true);
    this->GenerateButton->setEnabled(true);
    this->GenerateAction->setEnabled(true);
    this->ConfigureButton->setEnabled(true);
    if (!this->getProjectFilename().isEmpty()) {
      this->OpenProjectButton->setEnabled(true);
    }
    this->ConfigureButton->setText(tr("&Configure"));
    this->GenerateButton->setText(tr("&Generate"));
  } else if (s == ReadyGenerate) {
    this->setEnabledState(true);
    this->GenerateButton->setEnabled(true);
    this->GenerateAction->setEnabled(true);
    this->ConfigureButton->setEnabled(true);
    if (!this->getProjectFilename().isEmpty()) {
      this->OpenProjectButton->setEnabled(true);
    }
    this->ConfigureButton->setText(tr("&Configure"));
    this->GenerateButton->setText(tr("&Generate"));
  }
}

void CMakeSetupDialog::addCacheEntry()
{
  QDialog dialog(this);
  dialog.resize(400, 200);
  dialog.setWindowTitle(tr("Add Cache Entry"));
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  AddCacheEntry* w =
    new AddCacheEntry(&dialog, this->AddVariableNames, this->AddVariableTypes);
  QDialogButtonBox* btns = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(accepted()), &dialog, SLOT(accept()));
  QObject::connect(btns, SIGNAL(rejected()), &dialog, SLOT(reject()));
  l->addWidget(w);
  l->addStretch();
  l->addWidget(btns);
  if (QDialog::Accepted == dialog.exec()) {
    QCMakeCacheModel* m = this->CacheValues->cacheModel();
    m->insertProperty(w->type(), w->name(), w->description(), w->value(),
                      false);

    // only add variable names to the completion which are new
    if (!this->AddVariableNames.contains(w->name())) {
      this->AddVariableNames << w->name();
      this->AddVariableTypes << w->typeString();
      // limit to at most 100 completion items
      if (this->AddVariableNames.size() > 100) {
        this->AddVariableNames.removeFirst();
        this->AddVariableTypes.removeFirst();
      }
      // make sure CMAKE_INSTALL_PREFIX is always there
      if (!this->AddVariableNames.contains("CMAKE_INSTALL_PREFIX")) {
        this->AddVariableNames << "CMAKE_INSTALL_PREFIX";
        this->AddVariableTypes << "PATH";
      }
      QSettings settings;
      settings.beginGroup("Settings/StartPath");
      settings.setValue("AddVariableNames", this->AddVariableNames);
      settings.setValue("AddVariableTypes", this->AddVariableTypes);
    }
  }
}

void CMakeSetupDialog::startSearch()
{
  this->Search->setFocus(Qt::OtherFocusReason);
  this->Search->selectAll();
}

void CMakeSetupDialog::setDebugOutput(bool flag)
{
  QMetaObject::invokeMethod(this->CMakeThread->cmakeInstance(),
                            "setDebugOutput", Qt::QueuedConnection,
                            Q_ARG(bool, flag));
}

void CMakeSetupDialog::setGroupedView(bool v)
{
  this->CacheValues->cacheModel()->setViewType(v ? QCMakeCacheModel::GroupView
                                                 : QCMakeCacheModel::FlatView);
  this->CacheValues->setRootIsDecorated(v);

  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  settings.setValue("GroupView", v);
}

void CMakeSetupDialog::setAdvancedView(bool v)
{
  this->CacheValues->setShowAdvanced(v);
  QSettings settings;
  settings.beginGroup("Settings/StartPath");
  settings.setValue("AdvancedView", v);
}

void CMakeSetupDialog::showUserChanges()
{
  QSet<QCMakeProperty> changes =
    qobject_cast<QCMakeCacheModelDelegate*>(this->CacheValues->itemDelegate())
      ->changes();

  QDialog dialog(this);
  dialog.setWindowTitle(tr("My Changes"));
  dialog.resize(600, 400);
  QVBoxLayout* l = new QVBoxLayout(&dialog);
  QTextEdit* textedit = new QTextEdit(&dialog);
  textedit->setReadOnly(true);
  l->addWidget(textedit);
  QDialogButtonBox* btns =
    new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
  QObject::connect(btns, SIGNAL(rejected()), &dialog, SLOT(accept()));
  l->addWidget(btns);

  QString command;
  QString cache;

  foreach (QCMakeProperty const& prop, changes) {
    QString type;
    switch (prop.Type) {
      case QCMakeProperty::BOOL:
        type = "BOOL";
        break;
      case QCMakeProperty::PATH:
        type = "PATH";
        break;
      case QCMakeProperty::FILEPATH:
        type = "FILEPATH";
        break;
      case QCMakeProperty::STRING:
        type = "STRING";
        break;
    }
    QString value;
    if (prop.Type == QCMakeProperty::BOOL) {
      value = prop.Value.toBool() ? "1" : "0";
    } else {
      value = prop.Value.toString();
    }

    QString const line = QString("%1:%2=").arg(prop.Key, type);
    command += QString("-D%1\"%2\" ").arg(line, value);
    cache += QString("%1%2\n").arg(line, value);
  }

  textedit->append(tr("Commandline options:"));
  textedit->append(command);
  textedit->append("\n");
  textedit->append(tr("Cache file:"));
  textedit->append(cache);

  dialog.exec();
}

void CMakeSetupDialog::setSearchFilter(const QString& str)
{
  this->CacheValues->selectionModel()->clear();
  this->CacheValues->setSearchFilter(str);
}

void CMakeSetupDialog::doOutputContextMenu(const QPoint& pt)
{
  QMenu* menu = this->Output->createStandardContextMenu();

  menu->addSeparator();
  menu->addAction(tr("Find..."), this, SLOT(doOutputFindDialog()),
                  QKeySequence::Find);
  menu->addAction(tr("Find Next"), this, SLOT(doOutputFindNext()),
                  QKeySequence::FindNext);
  menu->addAction(tr("Find Previous"), this, SLOT(doOutputFindPrev()),
                  QKeySequence::FindPrevious);
  menu->addSeparator();
  menu->addAction(tr("Goto Next Error"), this, SLOT(doOutputErrorNext()),
                  QKeySequence(Qt::Key_F8));

  menu->exec(this->Output->mapToGlobal(pt));
  delete menu;
}

void CMakeSetupDialog::doOutputFindDialog()
{
  QStringList strings(this->FindHistory);

  QString selection = this->Output->textCursor().selectedText();
  if (!selection.isEmpty() && !selection.contains(QChar::ParagraphSeparator) &&
      !selection.contains(QChar::LineSeparator)) {
    strings.push_front(selection);
  }

  bool ok;
  QString search = QInputDialog::getItem(this, tr("Find in Output"),
                                         tr("Find:"), strings, 0, true, &ok);
  if (ok && !search.isEmpty()) {
    if (!this->FindHistory.contains(search)) {
      this->FindHistory.push_front(search);
    }
    doOutputFindNext();
  }
}

void CMakeSetupDialog::doRegexExplorerDialog()
{
  RegexExplorer dialog(this);
  dialog.exec();
}

void CMakeSetupDialog::doOutputFindPrev()
{
  doOutputFindNext(false);
}

void CMakeSetupDialog::doOutputFindNext(bool directionForward)
{
  if (this->FindHistory.isEmpty()) {
    doOutputFindDialog(); // will re-call this function again
    return;
  }

  QString search = this->FindHistory.front();

  QTextCursor textCursor = this->Output->textCursor();
  QTextDocument* document = this->Output->document();
  QTextDocument::FindFlags flags;
  if (!directionForward) {
    flags |= QTextDocument::FindBackward;
  }

  textCursor = document->find(search, textCursor, flags);

  if (textCursor.isNull()) {
    // first search found nothing, wrap around and search again
    textCursor = this->Output->textCursor();
    textCursor.movePosition(directionForward ? QTextCursor::Start
                                             : QTextCursor::End);
    textCursor = document->find(search, textCursor, flags);
  }

  if (textCursor.hasSelection()) {
    this->Output->setTextCursor(textCursor);
  }
}

void CMakeSetupDialog::doOutputErrorNext()
{
  QTextCursor textCursor = this->Output->textCursor();
  bool atEnd = false;

  // move cursor out of current error-block
  if (textCursor.blockCharFormat() == this->ErrorFormat) {
    atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
  }

  // move cursor to next error-block
  while (textCursor.blockCharFormat() != this->ErrorFormat && !atEnd) {
    atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
  }

  if (atEnd) {
    // first search found nothing, wrap around and search again
    atEnd = !textCursor.movePosition(QTextCursor::Start);

    // move cursor to next error-block
    while (textCursor.blockCharFormat() != this->ErrorFormat && !atEnd) {
      atEnd = !textCursor.movePosition(QTextCursor::NextBlock);
    }
  }

  if (!atEnd) {
    textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

    QTextCharFormat selectionFormat;
    selectionFormat.setBackground(Qt::yellow);
    QTextEdit::ExtraSelection extraSelection = { textCursor, selectionFormat };
    this->Output->setExtraSelections(QList<QTextEdit::ExtraSelection>()
                                     << extraSelection);

    // make the whole error-block visible
    this->Output->setTextCursor(textCursor);

    // remove the selection to see the extraSelection
    textCursor.setPosition(textCursor.anchor());
    this->Output->setTextCursor(textCursor);
  }
}

void CMakeSetupDialog::doWarningMessagesDialog()
{
  WarningMessagesDialog dialog(this, this->CMakeThread->cmakeInstance());
  dialog.exec();
}
