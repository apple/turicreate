/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "QCMake.h"

#include <QCoreApplication>
#include <QDir>

#include "cmExternalMakefileProjectGenerator.h"
#include "cmState.h"
#include "cmSystemTools.h"

#ifdef Q_OS_WIN
#include "qt_windows.h" // For SetErrorMode
#endif

QCMake::QCMake(QObject* p)
  : QObject(p)
{
  this->WarnUninitializedMode = false;
  this->WarnUnusedMode = false;
  qRegisterMetaType<QCMakeProperty>();
  qRegisterMetaType<QCMakePropertyList>();

  cmSystemTools::DisableRunCommandOutput();
  cmSystemTools::SetRunCommandHideConsole(true);
  cmSystemTools::SetMessageCallback(QCMake::messageCallback, this);
  cmSystemTools::SetStdoutCallback(QCMake::stdoutCallback, this);
  cmSystemTools::SetStderrCallback(QCMake::stderrCallback, this);

  this->CMakeInstance = new cmake(cmake::RoleProject);
  this->CMakeInstance->SetCMakeEditCommand(
    cmSystemTools::GetCMakeGUICommand());
  this->CMakeInstance->SetProgressCallback(QCMake::progressCallback, this);

  cmSystemTools::SetInterruptCallback(QCMake::interruptCallback, this);

  std::vector<cmake::GeneratorInfo> generators;
  this->CMakeInstance->GetRegisteredGenerators(generators);

  std::vector<cmake::GeneratorInfo>::const_iterator it;
  for (it = generators.begin(); it != generators.end(); ++it) {
    // Skip the generator "KDevelop3", since there is also
    // "KDevelop3 - Unix Makefiles", which is the full and official name.
    // The short name is actually only still there since this was the name
    // in CMake 2.4, to keep "command line argument compatibility", but
    // this is not necessary in the GUI.
    if (it->name == "KDevelop3") {
      continue;
    }

    this->AvailableGenerators.push_back(*it);
  }
}

QCMake::~QCMake()
{
  delete this->CMakeInstance;
  // cmDynamicLoader::FlushCache();
}

void QCMake::loadCache(const QString& dir)
{
  this->setBinaryDirectory(dir);
}

void QCMake::setSourceDirectory(const QString& _dir)
{
  QString dir = QString::fromLocal8Bit(
    cmSystemTools::GetActualCaseForPath(_dir.toLocal8Bit().data()).c_str());
  if (this->SourceDirectory != dir) {
    this->SourceDirectory = QDir::fromNativeSeparators(dir);
    emit this->sourceDirChanged(this->SourceDirectory);
  }
}

void QCMake::setBinaryDirectory(const QString& _dir)
{
  QString dir = QString::fromLocal8Bit(
    cmSystemTools::GetActualCaseForPath(_dir.toLocal8Bit().data()).c_str());
  if (this->BinaryDirectory != dir) {
    this->BinaryDirectory = QDir::fromNativeSeparators(dir);
    emit this->binaryDirChanged(this->BinaryDirectory);
    cmState* state = this->CMakeInstance->GetState();
    this->setGenerator(QString());
    this->setToolset(QString());
    if (!this->CMakeInstance->LoadCache(
          this->BinaryDirectory.toLocal8Bit().data())) {
      QDir testDir(this->BinaryDirectory);
      if (testDir.exists("CMakeCache.txt")) {
        cmSystemTools::Error(
          "There is a CMakeCache.txt file for the current binary "
          "tree but cmake does not have permission to read it.  "
          "Please check the permissions of the directory you are trying to "
          "run CMake on.");
      }
    }

    QCMakePropertyList props = this->properties();
    emit this->propertiesChanged(props);
    const char* homeDir = state->GetCacheEntryValue("CMAKE_HOME_DIRECTORY");
    if (homeDir) {
      setSourceDirectory(QString::fromLocal8Bit(homeDir));
    }
    const char* gen = state->GetCacheEntryValue("CMAKE_GENERATOR");
    if (gen) {
      const char* extraGen =
        state->GetInitializedCacheValue("CMAKE_EXTRA_GENERATOR");
      std::string curGen =
        cmExternalMakefileProjectGenerator::CreateFullGeneratorName(
          gen, extraGen ? extraGen : "");
      this->setGenerator(QString::fromLocal8Bit(curGen.c_str()));
    }

    const char* toolset = state->GetCacheEntryValue("CMAKE_GENERATOR_TOOLSET");
    if (toolset) {
      this->setToolset(QString::fromLocal8Bit(toolset));
    }
  }
}

void QCMake::setGenerator(const QString& gen)
{
  if (this->Generator != gen) {
    this->Generator = gen;
    emit this->generatorChanged(this->Generator);
  }
}

void QCMake::setToolset(const QString& toolset)
{
  if (this->Toolset != toolset) {
    this->Toolset = toolset;
    emit this->toolsetChanged(this->Toolset);
  }
}

void QCMake::configure()
{
#ifdef Q_OS_WIN
  UINT lastErrorMode = SetErrorMode(0);
#endif

  this->CMakeInstance->SetHomeDirectory(
    this->SourceDirectory.toLocal8Bit().data());
  this->CMakeInstance->SetHomeOutputDirectory(
    this->BinaryDirectory.toLocal8Bit().data());
  this->CMakeInstance->SetGlobalGenerator(
    this->CMakeInstance->CreateGlobalGenerator(
      this->Generator.toLocal8Bit().data()));
  this->CMakeInstance->SetGeneratorPlatform("");
  this->CMakeInstance->SetGeneratorToolset(this->Toolset.toLocal8Bit().data());
  this->CMakeInstance->LoadCache();
  this->CMakeInstance->SetWarnUninitialized(this->WarnUninitializedMode);
  this->CMakeInstance->SetWarnUnused(this->WarnUnusedMode);
  this->CMakeInstance->PreLoadCMakeFiles();

  InterruptFlag = 0;
  cmSystemTools::ResetErrorOccuredFlag();

  int err = this->CMakeInstance->Configure();

#ifdef Q_OS_WIN
  SetErrorMode(lastErrorMode);
#endif

  emit this->propertiesChanged(this->properties());
  emit this->configureDone(err);
}

void QCMake::generate()
{
#ifdef Q_OS_WIN
  UINT lastErrorMode = SetErrorMode(0);
#endif

  InterruptFlag = 0;
  cmSystemTools::ResetErrorOccuredFlag();

  int err = this->CMakeInstance->Generate();

#ifdef Q_OS_WIN
  SetErrorMode(lastErrorMode);
#endif

  emit this->generateDone(err);
}

void QCMake::setProperties(const QCMakePropertyList& newProps)
{
  QCMakePropertyList props = newProps;

  QStringList toremove;

  // set the value of properties
  cmState* state = this->CMakeInstance->GetState();
  std::vector<std::string> cacheKeys = state->GetCacheEntryKeys();
  for (std::vector<std::string>::const_iterator it = cacheKeys.begin();
       it != cacheKeys.end(); ++it) {
    cmStateEnums::CacheEntryType t = state->GetCacheEntryType(*it);
    if (t == cmStateEnums::INTERNAL || t == cmStateEnums::STATIC) {
      continue;
    }

    QCMakeProperty prop;
    prop.Key = QString::fromLocal8Bit(it->c_str());
    int idx = props.indexOf(prop);
    if (idx == -1) {
      toremove.append(QString::fromLocal8Bit(it->c_str()));
    } else {
      prop = props[idx];
      if (prop.Value.type() == QVariant::Bool) {
        state->SetCacheEntryValue(*it, prop.Value.toBool() ? "ON" : "OFF");
      } else {
        state->SetCacheEntryValue(*it,
                                  prop.Value.toString().toLocal8Bit().data());
      }
      props.removeAt(idx);
    }
  }

  // remove some properites
  foreach (QString const& s, toremove) {
    this->CMakeInstance->UnwatchUnusedCli(s.toLocal8Bit().data());

    state->RemoveCacheEntry(s.toLocal8Bit().data());
  }

  // add some new properites
  foreach (QCMakeProperty const& s, props) {
    this->CMakeInstance->WatchUnusedCli(s.Key.toLocal8Bit().data());

    if (s.Type == QCMakeProperty::BOOL) {
      this->CMakeInstance->AddCacheEntry(
        s.Key.toLocal8Bit().data(), s.Value.toBool() ? "ON" : "OFF",
        s.Help.toLocal8Bit().data(), cmStateEnums::BOOL);
    } else if (s.Type == QCMakeProperty::STRING) {
      this->CMakeInstance->AddCacheEntry(
        s.Key.toLocal8Bit().data(), s.Value.toString().toLocal8Bit().data(),
        s.Help.toLocal8Bit().data(), cmStateEnums::STRING);
    } else if (s.Type == QCMakeProperty::PATH) {
      this->CMakeInstance->AddCacheEntry(
        s.Key.toLocal8Bit().data(), s.Value.toString().toLocal8Bit().data(),
        s.Help.toLocal8Bit().data(), cmStateEnums::PATH);
    } else if (s.Type == QCMakeProperty::FILEPATH) {
      this->CMakeInstance->AddCacheEntry(
        s.Key.toLocal8Bit().data(), s.Value.toString().toLocal8Bit().data(),
        s.Help.toLocal8Bit().data(), cmStateEnums::FILEPATH);
    }
  }

  this->CMakeInstance->SaveCache(this->BinaryDirectory.toLocal8Bit().data());
}

QCMakePropertyList QCMake::properties() const
{
  QCMakePropertyList ret;

  cmState* state = this->CMakeInstance->GetState();
  std::vector<std::string> cacheKeys = state->GetCacheEntryKeys();
  for (std::vector<std::string>::const_iterator i = cacheKeys.begin();
       i != cacheKeys.end(); ++i) {
    cmStateEnums::CacheEntryType t = state->GetCacheEntryType(*i);
    if (t == cmStateEnums::INTERNAL || t == cmStateEnums::STATIC ||
        t == cmStateEnums::UNINITIALIZED) {
      continue;
    }

    const char* cachedValue = state->GetCacheEntryValue(*i);

    QCMakeProperty prop;
    prop.Key = QString::fromLocal8Bit(i->c_str());
    prop.Help =
      QString::fromLocal8Bit(state->GetCacheEntryProperty(*i, "HELPSTRING"));
    prop.Value = QString::fromLocal8Bit(cachedValue);
    prop.Advanced = state->GetCacheEntryPropertyAsBool(*i, "ADVANCED");
    if (t == cmStateEnums::BOOL) {
      prop.Type = QCMakeProperty::BOOL;
      prop.Value = cmSystemTools::IsOn(cachedValue);
    } else if (t == cmStateEnums::PATH) {
      prop.Type = QCMakeProperty::PATH;
    } else if (t == cmStateEnums::FILEPATH) {
      prop.Type = QCMakeProperty::FILEPATH;
    } else if (t == cmStateEnums::STRING) {
      prop.Type = QCMakeProperty::STRING;
      const char* stringsProperty =
        state->GetCacheEntryProperty(*i, "STRINGS");
      if (stringsProperty) {
        prop.Strings = QString::fromLocal8Bit(stringsProperty).split(";");
      }
    }

    ret.append(prop);
  }

  return ret;
}

void QCMake::interrupt()
{
  this->InterruptFlag.ref();
}

bool QCMake::interruptCallback(void* cd)
{
  QCMake* self = reinterpret_cast<QCMake*>(cd);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  return self->InterruptFlag;
#else
  return self->InterruptFlag.load();
#endif
}

void QCMake::progressCallback(const char* msg, float percent, void* cd)
{
  QCMake* self = reinterpret_cast<QCMake*>(cd);
  if (percent >= 0) {
    emit self->progressChanged(QString::fromLocal8Bit(msg), percent);
  } else {
    emit self->outputMessage(QString::fromLocal8Bit(msg));
  }
  QCoreApplication::processEvents();
}

void QCMake::messageCallback(const char* msg, const char* /*title*/,
                             bool& /*stop*/, void* cd)
{
  QCMake* self = reinterpret_cast<QCMake*>(cd);
  emit self->errorMessage(QString::fromLocal8Bit(msg));
  QCoreApplication::processEvents();
}

void QCMake::stdoutCallback(const char* msg, size_t len, void* cd)
{
  QCMake* self = reinterpret_cast<QCMake*>(cd);
  emit self->outputMessage(QString::fromLocal8Bit(msg, int(len)));
  QCoreApplication::processEvents();
}

void QCMake::stderrCallback(const char* msg, size_t len, void* cd)
{
  QCMake* self = reinterpret_cast<QCMake*>(cd);
  emit self->outputMessage(QString::fromLocal8Bit(msg, int(len)));
  QCoreApplication::processEvents();
}

QString QCMake::binaryDirectory() const
{
  return this->BinaryDirectory;
}

QString QCMake::sourceDirectory() const
{
  return this->SourceDirectory;
}

QString QCMake::generator() const
{
  return this->Generator;
}

std::vector<cmake::GeneratorInfo> const& QCMake::availableGenerators() const
{
  return AvailableGenerators;
}

void QCMake::deleteCache()
{
  // delete cache
  this->CMakeInstance->DeleteCache(this->BinaryDirectory.toLocal8Bit().data());
  // reload to make our cache empty
  this->CMakeInstance->LoadCache(this->BinaryDirectory.toLocal8Bit().data());
  // emit no generator and no properties
  this->setGenerator(QString());
  this->setToolset(QString());
  QCMakePropertyList props = this->properties();
  emit this->propertiesChanged(props);
}

void QCMake::reloadCache()
{
  // emit that the cache was cleaned out
  QCMakePropertyList props;
  emit this->propertiesChanged(props);
  // reload
  this->CMakeInstance->LoadCache(this->BinaryDirectory.toLocal8Bit().data());
  // emit new cache properties
  props = this->properties();
  emit this->propertiesChanged(props);
}

void QCMake::setDebugOutput(bool flag)
{
  if (flag != this->CMakeInstance->GetDebugOutput()) {
    this->CMakeInstance->SetDebugOutputOn(flag);
    emit this->debugOutputChanged(flag);
  }
}

bool QCMake::getDebugOutput() const
{
  return this->CMakeInstance->GetDebugOutput();
}

bool QCMake::getSuppressDevWarnings()
{
  return this->CMakeInstance->GetSuppressDevWarnings();
}

void QCMake::setSuppressDevWarnings(bool value)
{
  this->CMakeInstance->SetSuppressDevWarnings(value);
}

bool QCMake::getSuppressDeprecatedWarnings()
{
  return this->CMakeInstance->GetSuppressDeprecatedWarnings();
}

void QCMake::setSuppressDeprecatedWarnings(bool value)
{
  this->CMakeInstance->SetSuppressDeprecatedWarnings(value);
}

bool QCMake::getDevWarningsAsErrors()
{
  return this->CMakeInstance->GetDevWarningsAsErrors();
}

void QCMake::setDevWarningsAsErrors(bool value)
{
  this->CMakeInstance->SetDevWarningsAsErrors(value);
}

bool QCMake::getDeprecatedWarningsAsErrors()
{
  return this->CMakeInstance->GetDeprecatedWarningsAsErrors();
}

void QCMake::setDeprecatedWarningsAsErrors(bool value)
{
  this->CMakeInstance->SetDeprecatedWarningsAsErrors(value);
}

void QCMake::setWarnUninitializedMode(bool value)
{
  this->WarnUninitializedMode = value;
}

void QCMake::setWarnUnusedMode(bool value)
{
  this->WarnUnusedMode = value;
}
