
#include "FirstConfigure.h"

#include "Compilers.h"

#include <QComboBox>
#include <QRadioButton>
#include <QSettings>
#include <QVBoxLayout>

StartCompilerSetup::StartCompilerSetup(QWidget* p)
  : QWizardPage(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel(tr("Specify the generator for this project")));
  this->GeneratorOptions = new QComboBox(this);
  l->addWidget(this->GeneratorOptions);

  // Add the ability to specify toolset (-T parameter)
  ToolsetFrame = CreateToolsetWidgets();
  l->addWidget(ToolsetFrame);

  l->addSpacing(6);

  this->CompilerSetupOptions[0] =
    new QRadioButton(tr("Use default native compilers"), this);
  this->CompilerSetupOptions[1] =
    new QRadioButton(tr("Specify native compilers"), this);
  this->CompilerSetupOptions[2] =
    new QRadioButton(tr("Specify toolchain file for cross-compiling"), this);
  this->CompilerSetupOptions[3] =
    new QRadioButton(tr("Specify options for cross-compiling"), this);
  l->addWidget(this->CompilerSetupOptions[0]);
  l->addWidget(this->CompilerSetupOptions[1]);
  l->addWidget(this->CompilerSetupOptions[2]);
  l->addWidget(this->CompilerSetupOptions[3]);

  this->CompilerSetupOptions[0]->setChecked(true);

  QObject::connect(this->CompilerSetupOptions[0], SIGNAL(toggled(bool)), this,
                   SLOT(onSelectionChanged(bool)));
  QObject::connect(this->CompilerSetupOptions[1], SIGNAL(toggled(bool)), this,
                   SLOT(onSelectionChanged(bool)));
  QObject::connect(this->CompilerSetupOptions[2], SIGNAL(toggled(bool)), this,
                   SLOT(onSelectionChanged(bool)));
  QObject::connect(this->CompilerSetupOptions[3], SIGNAL(toggled(bool)), this,
                   SLOT(onSelectionChanged(bool)));
  QObject::connect(GeneratorOptions,
                   SIGNAL(currentIndexChanged(QString const&)), this,
                   SLOT(onGeneratorChanged(QString const&)));
}

QFrame* StartCompilerSetup::CreateToolsetWidgets()
{
  QFrame* frame = new QFrame(this);
  QVBoxLayout* l = new QVBoxLayout(frame);
  l->setContentsMargins(0, 0, 0, 0);

  ToolsetLabel = new QLabel(tr("Optional toolset to use (argument to -T)"));
  l->addWidget(ToolsetLabel);

  Toolset = new QLineEdit(frame);
  l->addWidget(Toolset);

  return frame;
}

StartCompilerSetup::~StartCompilerSetup()
{
}

void StartCompilerSetup::setGenerators(
  std::vector<cmake::GeneratorInfo> const& gens)
{
  this->GeneratorOptions->clear();

  QStringList generator_list;

  std::vector<cmake::GeneratorInfo>::const_iterator it;
  for (it = gens.begin(); it != gens.end(); ++it) {
    generator_list.append(QString::fromLocal8Bit(it->name.c_str()));

    if (it->supportsToolset) {
      this->GeneratorsSupportingToolset.append(
        QString::fromLocal8Bit(it->name.c_str()));
    }
  }

  this->GeneratorOptions->addItems(generator_list);
}

void StartCompilerSetup::setCurrentGenerator(const QString& gen)
{
  int idx = this->GeneratorOptions->findText(gen);
  if (idx != -1) {
    this->GeneratorOptions->setCurrentIndex(idx);
  }
}

QString StartCompilerSetup::getGenerator() const
{
  return this->GeneratorOptions->currentText();
};

QString StartCompilerSetup::getToolset() const
{
  return this->Toolset->text();
};

bool StartCompilerSetup::defaultSetup() const
{
  return this->CompilerSetupOptions[0]->isChecked();
}

bool StartCompilerSetup::compilerSetup() const
{
  return this->CompilerSetupOptions[1]->isChecked();
}

bool StartCompilerSetup::crossCompilerToolChainFile() const
{
  return this->CompilerSetupOptions[2]->isChecked();
}

bool StartCompilerSetup::crossCompilerSetup() const
{
  return this->CompilerSetupOptions[3]->isChecked();
}

void StartCompilerSetup::onSelectionChanged(bool on)
{
  if (on) {
    emit selectionChanged();
  }
}

void StartCompilerSetup::onGeneratorChanged(QString const& name)
{
  if (GeneratorsSupportingToolset.contains(name)) {
    ToolsetFrame->show();
  } else {
    ToolsetFrame->hide();
  }
}

int StartCompilerSetup::nextId() const
{
  if (compilerSetup()) {
    return NativeSetup;
  }
  if (crossCompilerSetup()) {
    return CrossSetup;
  }
  if (crossCompilerToolChainFile()) {
    return ToolchainSetup;
  }
  return -1;
}

NativeCompilerSetup::NativeCompilerSetup(QWidget* p)
  : QWizardPage(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  QWidget* c = new QWidget(this);
  l->addWidget(c);
  this->setupUi(c);
}

NativeCompilerSetup::~NativeCompilerSetup()
{
}

QString NativeCompilerSetup::getCCompiler() const
{
  return this->CCompiler->text();
}

void NativeCompilerSetup::setCCompiler(const QString& s)
{
  this->CCompiler->setText(s);
}

QString NativeCompilerSetup::getCXXCompiler() const
{
  return this->CXXCompiler->text();
}

void NativeCompilerSetup::setCXXCompiler(const QString& s)
{
  this->CXXCompiler->setText(s);
}

QString NativeCompilerSetup::getFortranCompiler() const
{
  return this->FortranCompiler->text();
}

void NativeCompilerSetup::setFortranCompiler(const QString& s)
{
  this->FortranCompiler->setText(s);
}

CrossCompilerSetup::CrossCompilerSetup(QWidget* p)
  : QWizardPage(p)
{
  this->setupUi(this);
  QWidget::setTabOrder(systemName, systemVersion);
  QWidget::setTabOrder(systemVersion, systemProcessor);
  QWidget::setTabOrder(systemProcessor, CrossCompilers->CCompiler);
  QWidget::setTabOrder(CrossCompilers->CCompiler, CrossCompilers->CXXCompiler);
  QWidget::setTabOrder(CrossCompilers->CXXCompiler,
                       CrossCompilers->FortranCompiler);
  QWidget::setTabOrder(CrossCompilers->FortranCompiler, crossFindRoot);
  QWidget::setTabOrder(crossFindRoot, crossProgramMode);
  QWidget::setTabOrder(crossProgramMode, crossLibraryMode);
  QWidget::setTabOrder(crossLibraryMode, crossIncludeMode);

  // fill in combo boxes
  QStringList modes;
  modes << tr("Search in Target Root, then native system");
  modes << tr("Search only in Target Root");
  modes << tr("Search only in native system");
  crossProgramMode->addItems(modes);
  crossLibraryMode->addItems(modes);
  crossIncludeMode->addItems(modes);
  crossProgramMode->setCurrentIndex(2);
  crossLibraryMode->setCurrentIndex(1);
  crossIncludeMode->setCurrentIndex(1);

  this->registerField("systemName*", this->systemName);
}

CrossCompilerSetup::~CrossCompilerSetup()
{
}

QString CrossCompilerSetup::getCCompiler() const
{
  return this->CrossCompilers->CCompiler->text();
}

void CrossCompilerSetup::setCCompiler(const QString& s)
{
  this->CrossCompilers->CCompiler->setText(s);
}

QString CrossCompilerSetup::getCXXCompiler() const
{
  return this->CrossCompilers->CXXCompiler->text();
}

void CrossCompilerSetup::setCXXCompiler(const QString& s)
{
  this->CrossCompilers->CXXCompiler->setText(s);
}

QString CrossCompilerSetup::getFortranCompiler() const
{
  return this->CrossCompilers->FortranCompiler->text();
}

void CrossCompilerSetup::setFortranCompiler(const QString& s)
{
  this->CrossCompilers->FortranCompiler->setText(s);
}

QString CrossCompilerSetup::getSystem() const
{
  return this->systemName->text();
}

void CrossCompilerSetup::setSystem(const QString& t)
{
  this->systemName->setText(t);
}

QString CrossCompilerSetup::getVersion() const
{
  return this->systemVersion->text();
}

void CrossCompilerSetup::setVersion(const QString& t)
{
  this->systemVersion->setText(t);
}

QString CrossCompilerSetup::getProcessor() const
{
  return this->systemProcessor->text();
}

void CrossCompilerSetup::setProcessor(const QString& t)
{
  this->systemProcessor->setText(t);
}

QString CrossCompilerSetup::getFindRoot() const
{
  return this->crossFindRoot->text();
}

void CrossCompilerSetup::setFindRoot(const QString& t)
{
  this->crossFindRoot->setText(t);
}

int CrossCompilerSetup::getProgramMode() const
{
  return this->crossProgramMode->currentIndex();
}

int CrossCompilerSetup::getLibraryMode() const
{
  return this->crossLibraryMode->currentIndex();
}

int CrossCompilerSetup::getIncludeMode() const
{
  return this->crossIncludeMode->currentIndex();
}

void CrossCompilerSetup::setProgramMode(int m)
{
  this->crossProgramMode->setCurrentIndex(m);
}

void CrossCompilerSetup::setLibraryMode(int m)
{
  this->crossLibraryMode->setCurrentIndex(m);
}

void CrossCompilerSetup::setIncludeMode(int m)
{
  this->crossIncludeMode->setCurrentIndex(m);
}

ToolchainCompilerSetup::ToolchainCompilerSetup(QWidget* p)
  : QWizardPage(p)
{
  QVBoxLayout* l = new QVBoxLayout(this);
  l->addWidget(new QLabel(tr("Specify the Toolchain file")));
  this->ToolchainFile = new QCMakeFilePathEditor(this);
  l->addWidget(this->ToolchainFile);
}

ToolchainCompilerSetup::~ToolchainCompilerSetup()
{
}

QString ToolchainCompilerSetup::toolchainFile() const
{
  return this->ToolchainFile->text();
}

void ToolchainCompilerSetup::setToolchainFile(const QString& t)
{
  this->ToolchainFile->setText(t);
}

FirstConfigure::FirstConfigure()
{
  // this->setOption(QWizard::HaveFinishButtonOnEarlyPages, true);
  this->mStartCompilerSetupPage = new StartCompilerSetup(this);
  this->setPage(Start, this->mStartCompilerSetupPage);
  QObject::connect(this->mStartCompilerSetupPage, SIGNAL(selectionChanged()),
                   this, SLOT(restart()));

  this->mNativeCompilerSetupPage = new NativeCompilerSetup(this);
  this->setPage(NativeSetup, this->mNativeCompilerSetupPage);

  this->mCrossCompilerSetupPage = new CrossCompilerSetup(this);
  this->setPage(CrossSetup, this->mCrossCompilerSetupPage);

  this->mToolchainCompilerSetupPage = new ToolchainCompilerSetup(this);
  this->setPage(ToolchainSetup, this->mToolchainCompilerSetupPage);
}

FirstConfigure::~FirstConfigure()
{
}

void FirstConfigure::setGenerators(
  std::vector<cmake::GeneratorInfo> const& gens)
{
  this->mStartCompilerSetupPage->setGenerators(gens);
}

QString FirstConfigure::getGenerator() const
{
  return this->mStartCompilerSetupPage->getGenerator();
}

QString FirstConfigure::getToolset() const
{
  return this->mStartCompilerSetupPage->getToolset();
}

void FirstConfigure::loadFromSettings()
{
  QSettings settings;
  // restore generator
  settings.beginGroup("Settings/StartPath");
  QString lastGen = settings.value("LastGenerator").toString();
  this->mStartCompilerSetupPage->setCurrentGenerator(lastGen);
  settings.endGroup();

  // restore compiler setup
  settings.beginGroup("Settings/Compiler");
  this->mNativeCompilerSetupPage->setCCompiler(
    settings.value("CCompiler").toString());
  this->mNativeCompilerSetupPage->setCXXCompiler(
    settings.value("CXXCompiler").toString());
  this->mNativeCompilerSetupPage->setFortranCompiler(
    settings.value("FortranCompiler").toString());
  settings.endGroup();

  // restore cross compiler setup
  settings.beginGroup("Settings/CrossCompiler");
  this->mCrossCompilerSetupPage->setCCompiler(
    settings.value("CCompiler").toString());
  this->mCrossCompilerSetupPage->setCXXCompiler(
    settings.value("CXXCompiler").toString());
  this->mCrossCompilerSetupPage->setFortranCompiler(
    settings.value("FortranCompiler").toString());
  this->mToolchainCompilerSetupPage->setToolchainFile(
    settings.value("ToolChainFile").toString());
  this->mCrossCompilerSetupPage->setSystem(
    settings.value("SystemName").toString());
  this->mCrossCompilerSetupPage->setVersion(
    settings.value("SystemVersion").toString());
  this->mCrossCompilerSetupPage->setProcessor(
    settings.value("SystemProcessor").toString());
  this->mCrossCompilerSetupPage->setFindRoot(
    settings.value("FindRoot").toString());
  this->mCrossCompilerSetupPage->setProgramMode(
    settings.value("ProgramMode", 0).toInt());
  this->mCrossCompilerSetupPage->setLibraryMode(
    settings.value("LibraryMode", 0).toInt());
  this->mCrossCompilerSetupPage->setIncludeMode(
    settings.value("IncludeMode", 0).toInt());
  settings.endGroup();
}

void FirstConfigure::saveToSettings()
{
  QSettings settings;

  // save generator
  settings.beginGroup("Settings/StartPath");
  QString lastGen = this->mStartCompilerSetupPage->getGenerator();
  settings.setValue("LastGenerator", lastGen);
  settings.endGroup();

  // save compiler setup
  settings.beginGroup("Settings/Compiler");
  settings.setValue("CCompiler",
                    this->mNativeCompilerSetupPage->getCCompiler());
  settings.setValue("CXXCompiler",
                    this->mNativeCompilerSetupPage->getCXXCompiler());
  settings.setValue("FortranCompiler",
                    this->mNativeCompilerSetupPage->getFortranCompiler());
  settings.endGroup();

  // save cross compiler setup
  settings.beginGroup("Settings/CrossCompiler");
  settings.setValue("CCompiler",
                    this->mCrossCompilerSetupPage->getCCompiler());
  settings.setValue("CXXCompiler",
                    this->mCrossCompilerSetupPage->getCXXCompiler());
  settings.setValue("FortranCompiler",
                    this->mCrossCompilerSetupPage->getFortranCompiler());
  settings.setValue("ToolChainFile", this->getCrossCompilerToolChainFile());
  settings.setValue("SystemName", this->mCrossCompilerSetupPage->getSystem());
  settings.setValue("SystemVersion",
                    this->mCrossCompilerSetupPage->getVersion());
  settings.setValue("SystemProcessor",
                    this->mCrossCompilerSetupPage->getProcessor());
  settings.setValue("FindRoot", this->mCrossCompilerSetupPage->getFindRoot());
  settings.setValue("ProgramMode",
                    this->mCrossCompilerSetupPage->getProgramMode());
  settings.setValue("LibraryMode",
                    this->mCrossCompilerSetupPage->getLibraryMode());
  settings.setValue("IncludeMode",
                    this->mCrossCompilerSetupPage->getIncludeMode());
  settings.endGroup();
}

bool FirstConfigure::defaultSetup() const
{
  return this->mStartCompilerSetupPage->defaultSetup();
}

bool FirstConfigure::compilerSetup() const
{
  return this->mStartCompilerSetupPage->compilerSetup();
}

bool FirstConfigure::crossCompilerSetup() const
{
  return this->mStartCompilerSetupPage->crossCompilerSetup();
}

bool FirstConfigure::crossCompilerToolChainFile() const
{
  return this->mStartCompilerSetupPage->crossCompilerToolChainFile();
}

QString FirstConfigure::getCrossCompilerToolChainFile() const
{
  return this->mToolchainCompilerSetupPage->toolchainFile();
}

QString FirstConfigure::getSystemName() const
{
  return this->mCrossCompilerSetupPage->getSystem();
}

QString FirstConfigure::getCCompiler() const
{
  if (this->compilerSetup()) {
    return this->mNativeCompilerSetupPage->getCCompiler();
  }
  if (this->crossCompilerSetup()) {
    return this->mCrossCompilerSetupPage->getCCompiler();
  }
  return QString();
}

QString FirstConfigure::getCXXCompiler() const
{
  if (this->compilerSetup()) {
    return this->mNativeCompilerSetupPage->getCXXCompiler();
  }
  if (this->crossCompilerSetup()) {
    return this->mCrossCompilerSetupPage->getCXXCompiler();
  }
  return QString();
}

QString FirstConfigure::getFortranCompiler() const
{
  if (this->compilerSetup()) {
    return this->mNativeCompilerSetupPage->getFortranCompiler();
  }
  if (this->crossCompilerSetup()) {
    return this->mCrossCompilerSetupPage->getFortranCompiler();
  }
  return QString();
}

QString FirstConfigure::getSystemVersion() const
{
  return this->mCrossCompilerSetupPage->getVersion();
}

QString FirstConfigure::getSystemProcessor() const
{
  return this->mCrossCompilerSetupPage->getProcessor();
}

QString FirstConfigure::getCrossRoot() const
{
  return this->mCrossCompilerSetupPage->getFindRoot();
}

const QString CrossModes[] = { "BOTH", "ONLY", "NEVER" };

QString FirstConfigure::getCrossProgramMode() const
{
  return CrossModes[this->mCrossCompilerSetupPage->getProgramMode()];
}

QString FirstConfigure::getCrossLibraryMode() const
{
  return CrossModes[this->mCrossCompilerSetupPage->getLibraryMode()];
}

QString FirstConfigure::getCrossIncludeMode() const
{
  return CrossModes[this->mCrossCompilerSetupPage->getIncludeMode()];
}
