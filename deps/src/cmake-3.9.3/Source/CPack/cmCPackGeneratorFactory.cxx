/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackGeneratorFactory.h"

#include "cmConfigure.h"
#include <ostream>
#include <utility>

#include "IFW/cmCPackIFWGenerator.h"
#include "cmAlgorithms.h"
#include "cmCPack7zGenerator.h"
#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmCPackNSISGenerator.h"
#include "cmCPackSTGZGenerator.h"
#include "cmCPackTGZGenerator.h"
#include "cmCPackTXZGenerator.h"
#include "cmCPackTarBZip2Generator.h"
#include "cmCPackTarCompressGenerator.h"
#include "cmCPackZIPGenerator.h"

#ifdef __APPLE__
#include "cmCPackBundleGenerator.h"
#include "cmCPackDragNDropGenerator.h"
#include "cmCPackOSXX11Generator.h"
#include "cmCPackPackageMakerGenerator.h"
#include "cmCPackProductBuildGenerator.h"
#endif

#ifdef __CYGWIN__
#include "cmCPackCygwinBinaryGenerator.h"
#include "cmCPackCygwinSourceGenerator.h"
#endif

#if !defined(_WIN32) && !defined(__QNXNTO__) && !defined(__BEOS__) &&         \
  !defined(__HAIKU__)
#include "cmCPackDebGenerator.h"
#include "cmCPackRPMGenerator.h"
#endif

#ifdef _WIN32
#include "WiX/cmCPackWIXGenerator.h"
#endif

cmCPackGeneratorFactory::cmCPackGeneratorFactory()
{
  if (cmCPackTGZGenerator::CanGenerate()) {
    this->RegisterGenerator("TGZ", "Tar GZip compression",
                            cmCPackTGZGenerator::CreateGenerator);
  }
  if (cmCPackTXZGenerator::CanGenerate()) {
    this->RegisterGenerator("TXZ", "Tar XZ compression",
                            cmCPackTXZGenerator::CreateGenerator);
  }
  if (cmCPackSTGZGenerator::CanGenerate()) {
    this->RegisterGenerator("STGZ", "Self extracting Tar GZip compression",
                            cmCPackSTGZGenerator::CreateGenerator);
  }
  if (cmCPackNSISGenerator::CanGenerate()) {
    this->RegisterGenerator("NSIS", "Null Soft Installer",
                            cmCPackNSISGenerator::CreateGenerator);
    this->RegisterGenerator("NSIS64", "Null Soft Installer (64-bit)",
                            cmCPackNSISGenerator::CreateGenerator64);
  }
  if (cmCPackIFWGenerator::CanGenerate()) {
    this->RegisterGenerator("IFW", "Qt Installer Framework",
                            cmCPackIFWGenerator::CreateGenerator);
  }
#ifdef __CYGWIN__
  if (cmCPackCygwinBinaryGenerator::CanGenerate()) {
    this->RegisterGenerator("CygwinBinary", "Cygwin Binary Installer",
                            cmCPackCygwinBinaryGenerator::CreateGenerator);
  }
  if (cmCPackCygwinSourceGenerator::CanGenerate()) {
    this->RegisterGenerator("CygwinSource", "Cygwin Source Installer",
                            cmCPackCygwinSourceGenerator::CreateGenerator);
  }
#endif

  if (cmCPackZIPGenerator::CanGenerate()) {
    this->RegisterGenerator("ZIP", "ZIP file format",
                            cmCPackZIPGenerator::CreateGenerator);
  }
  if (cmCPack7zGenerator::CanGenerate()) {
    this->RegisterGenerator("7Z", "7-Zip file format",
                            cmCPack7zGenerator::CreateGenerator);
  }
#ifdef _WIN32
  if (cmCPackWIXGenerator::CanGenerate()) {
    this->RegisterGenerator("WIX", "MSI file format via WiX tools",
                            cmCPackWIXGenerator::CreateGenerator);
  }
#endif
  if (cmCPackTarBZip2Generator::CanGenerate()) {
    this->RegisterGenerator("TBZ2", "Tar BZip2 compression",
                            cmCPackTarBZip2Generator::CreateGenerator);
  }
  if (cmCPackTarCompressGenerator::CanGenerate()) {
    this->RegisterGenerator("TZ", "Tar Compress compression",
                            cmCPackTarCompressGenerator::CreateGenerator);
  }
#ifdef __APPLE__
  if (cmCPackDragNDropGenerator::CanGenerate()) {
    this->RegisterGenerator("DragNDrop", "Mac OSX Drag And Drop",
                            cmCPackDragNDropGenerator::CreateGenerator);
  }
  if (cmCPackBundleGenerator::CanGenerate()) {
    this->RegisterGenerator("Bundle", "Mac OSX bundle",
                            cmCPackBundleGenerator::CreateGenerator);
  }
  if (cmCPackPackageMakerGenerator::CanGenerate()) {
    this->RegisterGenerator("PackageMaker", "Mac OSX Package Maker installer",
                            cmCPackPackageMakerGenerator::CreateGenerator);
  }
  if (cmCPackOSXX11Generator::CanGenerate()) {
    this->RegisterGenerator("OSXX11", "Mac OSX X11 bundle",
                            cmCPackOSXX11Generator::CreateGenerator);
  }
  if (cmCPackProductBuildGenerator::CanGenerate()) {
    this->RegisterGenerator("productbuild", "Mac OSX pkg",
                            cmCPackProductBuildGenerator::CreateGenerator);
  }
#endif
#if !defined(_WIN32) && !defined(__QNXNTO__) && !defined(__BEOS__) &&         \
  !defined(__HAIKU__)
  if (cmCPackDebGenerator::CanGenerate()) {
    this->RegisterGenerator("DEB", "Debian packages",
                            cmCPackDebGenerator::CreateGenerator);
  }
  if (cmCPackRPMGenerator::CanGenerate()) {
    this->RegisterGenerator("RPM", "RPM packages",
                            cmCPackRPMGenerator::CreateGenerator);
  }
#endif
}

cmCPackGeneratorFactory::~cmCPackGeneratorFactory()
{
  cmDeleteAll(this->Generators);
}

cmCPackGenerator* cmCPackGeneratorFactory::NewGenerator(
  const std::string& name)
{
  cmCPackGenerator* gen = this->NewGeneratorInternal(name);
  if (!gen) {
    return CM_NULLPTR;
  }
  this->Generators.push_back(gen);
  gen->SetLogger(this->Logger);
  return gen;
}

cmCPackGenerator* cmCPackGeneratorFactory::NewGeneratorInternal(
  const std::string& name)
{
  cmCPackGeneratorFactory::t_GeneratorCreatorsMap::iterator it =
    this->GeneratorCreators.find(name);
  if (it == this->GeneratorCreators.end()) {
    return CM_NULLPTR;
  }
  return (it->second)();
}

void cmCPackGeneratorFactory::RegisterGenerator(
  const std::string& name, const char* generatorDescription,
  CreateGeneratorCall* createGenerator)
{
  if (!createGenerator) {
    cmCPack_Log(this->Logger, cmCPackLog::LOG_ERROR,
                "Cannot register generator" << std::endl);
    return;
  }
  this->GeneratorCreators[name] = createGenerator;
  this->GeneratorDescriptions[name] = generatorDescription;
}
