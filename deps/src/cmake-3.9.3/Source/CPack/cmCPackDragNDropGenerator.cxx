/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackDragNDropGenerator.h"

#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <iomanip>
#include <map>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>

#ifdef HAVE_CoreServices
// For the old LocaleStringToLangAndRegionCodes() function, to convert
// to the old Script Manager RegionCode values needed for the 'LPic' data
// structure used for generating multi-lingual SLAs.
#include <CoreServices/CoreServices.h>
#endif

static const char* SLAHeader =
  "data 'LPic' (5000) {\n"
  "    $\"0002 0011 0003 0001 0000 0000 0002 0000\"\n"
  "    $\"0008 0003 0000 0001 0004 0000 0004 0005\"\n"
  "    $\"0000 000E 0006 0001 0005 0007 0000 0007\"\n"
  "    $\"0008 0000 0047 0009 0000 0034 000A 0001\"\n"
  "    $\"0035 000B 0001 0020 000C 0000 0011 000D\"\n"
  "    $\"0000 005B 0004 0000 0033 000F 0001 000C\"\n"
  "    $\"0010 0000 000B 000E 0000\"\n"
  "};\n"
  "\n";

static const char* SLASTREnglish =
  "resource 'STR#' (5002, \"English\") {\n"
  "    {\n"
  "        \"English\",\n"
  "        \"Agree\",\n"
  "        \"Disagree\",\n"
  "        \"Print\",\n"
  "        \"Save...\",\n"
  "        \"You agree to the License Agreement terms when you click \"\n"
  "        \"the \\\"Agree\\\" button.\",\n"
  "        \"Software License Agreement\",\n"
  "        \"This text cannot be saved.  This disk may be full or locked, "
  "or the \"\n"
  "        \"file may be locked.\",\n"
  "        \"Unable to print.  Make sure you have selected a printer.\"\n"
  "    }\n"
  "};\n"
  "\n";

cmCPackDragNDropGenerator::cmCPackDragNDropGenerator()
  : singleLicense(false)
{
  // default to one package file for components
  this->componentPackageMethod = ONE_PACKAGE;
}

cmCPackDragNDropGenerator::~cmCPackDragNDropGenerator()
{
}

int cmCPackDragNDropGenerator::InitializeInternal()
{
  // Starting with Xcode 4.3, look in "/Applications/Xcode.app" first:
  //
  std::vector<std::string> paths;
  paths.push_back("/Applications/Xcode.app/Contents/Developer/Tools");
  paths.push_back("/Developer/Tools");

  const std::string hdiutil_path =
    cmSystemTools::FindProgram("hdiutil", std::vector<std::string>(), false);
  if (hdiutil_path.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot locate hdiutil command"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_COMMAND_HDIUTIL", hdiutil_path.c_str());

  const std::string setfile_path =
    cmSystemTools::FindProgram("SetFile", paths, false);
  if (setfile_path.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot locate SetFile command"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_COMMAND_SETFILE", setfile_path.c_str());

  const std::string rez_path = cmSystemTools::FindProgram("Rez", paths, false);
  if (rez_path.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Cannot locate Rez command"
                    << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_COMMAND_REZ", rez_path.c_str());

  if (this->IsSet("CPACK_DMG_SLA_DIR")) {
    slaDirectory = this->GetOption("CPACK_DMG_SLA_DIR");
    if (!slaDirectory.empty() && this->IsSet("CPACK_RESOURCE_FILE_LICENSE")) {
      std::string license_file =
        this->GetOption("CPACK_RESOURCE_FILE_LICENSE");
      if (!license_file.empty() &&
          (license_file.find("CPack.GenericLicense.txt") ==
           std::string::npos)) {
        cmCPackLogger(
          cmCPackLog::LOG_OUTPUT,
          "Both CPACK_DMG_SLA_DIR and CPACK_RESOURCE_FILE_LICENSE specified, "
          "using CPACK_RESOURCE_FILE_LICENSE as a license for all languages."
            << std::endl);
        singleLicense = true;
      }
    }
    if (!this->IsSet("CPACK_DMG_SLA_LANGUAGES")) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "CPACK_DMG_SLA_DIR set but no languages defined "
                    "(set CPACK_DMG_SLA_LANGUAGES)"
                      << std::endl);
      return 0;
    }
    if (!cmSystemTools::FileExists(slaDirectory, false)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_DMG_SLA_DIR does not exist"
                      << std::endl);
      return 0;
    }

    std::vector<std::string> languages;
    cmSystemTools::ExpandListArgument(
      this->GetOption("CPACK_DMG_SLA_LANGUAGES"), languages);
    if (languages.empty()) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "CPACK_DMG_SLA_LANGUAGES set but empty" << std::endl);
      return 0;
    }
    for (size_t i = 0; i < languages.size(); ++i) {
      std::string license = slaDirectory + "/" + languages[i] + ".license.txt";
      if (!singleLicense && !cmSystemTools::FileExists(license)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Missing license file "
                        << languages[i] << ".license.txt" << std::endl);
        return 0;
      }
      std::string menu = slaDirectory + "/" + languages[i] + ".menu.txt";
      if (!cmSystemTools::FileExists(menu)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR, "Missing menu file "
                        << languages[i] << ".menu.txt" << std::endl);
        return 0;
      }
    }
  }

  return this->Superclass::InitializeInternal();
}

const char* cmCPackDragNDropGenerator::GetOutputExtension()
{
  return ".dmg";
}

int cmCPackDragNDropGenerator::PackageFiles()
{
  // gather which directories to make dmg files for
  // multiple directories occur if packaging components or groups separately

  // monolith
  if (this->Components.empty()) {
    return this->CreateDMG(toplevel, packageFileNames[0]);
  }

  // component install
  std::vector<std::string> package_files;

  std::map<std::string, cmCPackComponent>::iterator compIt;
  for (compIt = this->Components.begin(); compIt != this->Components.end();
       ++compIt) {
    std::string name = GetComponentInstallDirNameSuffix(compIt->first);
    package_files.push_back(name);
  }
  std::sort(package_files.begin(), package_files.end());
  package_files.erase(std::unique(package_files.begin(), package_files.end()),
                      package_files.end());

  // loop to create dmg files
  packageFileNames.clear();
  for (size_t i = 0; i < package_files.size(); i++) {
    std::string full_package_name = std::string(toplevel) + std::string("/");
    if (package_files[i] == "ALL_IN_ONE") {
      full_package_name += this->GetOption("CPACK_PACKAGE_FILE_NAME");
    } else {
      full_package_name += package_files[i];
    }
    full_package_name += std::string(GetOutputExtension());
    packageFileNames.push_back(full_package_name);

    std::string src_dir = toplevel;
    src_dir += "/";
    src_dir += package_files[i];

    if (0 == this->CreateDMG(src_dir, full_package_name)) {
      return 0;
    }
  }
  return 1;
}

bool cmCPackDragNDropGenerator::CopyFile(std::ostringstream& source,
                                         std::ostringstream& target)
{
  if (!cmSystemTools::CopyFileIfDifferent(source.str().c_str(),
                                          target.str().c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error copying "
                    << source.str() << " to " << target.str() << std::endl);

    return false;
  }

  return true;
}

bool cmCPackDragNDropGenerator::CreateEmptyFile(std::ostringstream& target,
                                                size_t size)
{
  cmsys::ofstream fout(target.str().c_str(), std::ios::out | std::ios::binary);
  if (!fout) {
    return false;
  } else {
    // Seek to desired size - 1 byte
    fout.seekp(size - 1, std::ios::beg);
    char byte = 0;
    // Write one byte to ensure file grows
    fout.write(&byte, 1);
  }

  return true;
}

bool cmCPackDragNDropGenerator::RunCommand(std::ostringstream& command,
                                           std::string* output)
{
  int exit_code = 1;

  bool result =
    cmSystemTools::RunSingleCommand(command.str().c_str(), output, output,
                                    &exit_code, 0, this->GeneratorVerbose, 0);

  if (!result || exit_code) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error executing: " << command.str()
                                                             << std::endl);

    return false;
  }

  return true;
}

int cmCPackDragNDropGenerator::CreateDMG(const std::string& src_dir,
                                         const std::string& output_file)
{
  // Get optional arguments ...
  const std::string cpack_package_icon = this->GetOption("CPACK_PACKAGE_ICON")
    ? this->GetOption("CPACK_PACKAGE_ICON")
    : "";

  const std::string cpack_dmg_volume_name =
    this->GetOption("CPACK_DMG_VOLUME_NAME")
    ? this->GetOption("CPACK_DMG_VOLUME_NAME")
    : this->GetOption("CPACK_PACKAGE_FILE_NAME");

  const std::string cpack_dmg_format = this->GetOption("CPACK_DMG_FORMAT")
    ? this->GetOption("CPACK_DMG_FORMAT")
    : "UDZO";

  // Get optional arguments ...
  std::string cpack_license_file =
    this->GetOption("CPACK_RESOURCE_FILE_LICENSE")
    ? this->GetOption("CPACK_RESOURCE_FILE_LICENSE")
    : "";

  const std::string cpack_dmg_background_image =
    this->GetOption("CPACK_DMG_BACKGROUND_IMAGE")
    ? this->GetOption("CPACK_DMG_BACKGROUND_IMAGE")
    : "";

  const std::string cpack_dmg_ds_store = this->GetOption("CPACK_DMG_DS_STORE")
    ? this->GetOption("CPACK_DMG_DS_STORE")
    : "";

  const std::string cpack_dmg_languages =
    this->GetOption("CPACK_DMG_SLA_LANGUAGES")
    ? this->GetOption("CPACK_DMG_SLA_LANGUAGES")
    : "";

  const std::string cpack_dmg_ds_store_setup_script =
    this->GetOption("CPACK_DMG_DS_STORE_SETUP_SCRIPT")
    ? this->GetOption("CPACK_DMG_DS_STORE_SETUP_SCRIPT")
    : "";

  const bool cpack_dmg_disable_applications_symlink =
    this->IsOn("CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK");

  // only put license on dmg if is user provided
  if (!cpack_license_file.empty() &&
      cpack_license_file.find("CPack.GenericLicense.txt") !=
        std::string::npos) {
    cpack_license_file = "";
  }

  // use sla_dir if both sla_dir and license_file are set
  if (!cpack_license_file.empty() && !slaDirectory.empty() && !singleLicense) {
    cpack_license_file = "";
  }

  // The staging directory contains everything that will end-up inside the
  // final disk image ...
  std::ostringstream staging;
  staging << src_dir;

  // Add a symlink to /Applications so users can drag-and-drop the bundle
  // into it unless this behaviour was disabled
  if (!cpack_dmg_disable_applications_symlink) {
    std::ostringstream application_link;
    application_link << staging.str() << "/Applications";
    cmSystemTools::CreateSymlink("/Applications", application_link.str());
  }

  // Optionally add a custom volume icon ...
  if (!cpack_package_icon.empty()) {
    std::ostringstream package_icon_source;
    package_icon_source << cpack_package_icon;

    std::ostringstream package_icon_destination;
    package_icon_destination << staging.str() << "/.VolumeIcon.icns";

    if (!this->CopyFile(package_icon_source, package_icon_destination)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error copying disk volume icon.  "
                    "Check the value of CPACK_PACKAGE_ICON."
                      << std::endl);

      return 0;
    }
  }

  // Optionally add a custom .DS_Store file
  // (e.g. for setting background/layout) ...
  if (!cpack_dmg_ds_store.empty()) {
    std::ostringstream package_settings_source;
    package_settings_source << cpack_dmg_ds_store;

    std::ostringstream package_settings_destination;
    package_settings_destination << staging.str() << "/.DS_Store";

    if (!this->CopyFile(package_settings_source,
                        package_settings_destination)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error copying disk volume settings file.  "
                    "Check the value of CPACK_DMG_DS_STORE."
                      << std::endl);

      return 0;
    }
  }

  // Optionally add a custom background image ...
  // Make sure the background file type is the same as the custom image
  // and that the file is hidden so it doesn't show up.
  if (!cpack_dmg_background_image.empty()) {
    const std::string extension =
      cmSystemTools::GetFilenameLastExtension(cpack_dmg_background_image);
    std::ostringstream package_background_source;
    package_background_source << cpack_dmg_background_image;

    std::ostringstream package_background_destination;
    package_background_destination << staging.str()
                                   << "/.background/background" << extension;

    if (!this->CopyFile(package_background_source,
                        package_background_destination)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error copying disk volume background image.  "
                    "Check the value of CPACK_DMG_BACKGROUND_IMAGE."
                      << std::endl);

      return 0;
    }
  }

  bool remount_image =
    !cpack_package_icon.empty() || !cpack_dmg_ds_store_setup_script.empty();

  std::string temp_image_format = "UDZO";

  // Create 1 MB dummy padding file in staging area when we need to remount
  // image, so we have enough space for storing changes ...
  if (remount_image) {
    std::ostringstream dummy_padding;
    dummy_padding << staging.str() << "/.dummy-padding-file";
    if (!this->CreateEmptyFile(dummy_padding, 1048576)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error creating dummy padding file."
                      << std::endl);

      return 0;
    }
    temp_image_format = "UDRW";
  }

  // Create a temporary read-write disk image ...
  std::string temp_image = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  temp_image += "/temp.dmg";

  std::ostringstream temp_image_command;
  temp_image_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
  temp_image_command << " create";
  temp_image_command << " -ov";
  temp_image_command << " -srcfolder \"" << staging.str() << "\"";
  temp_image_command << " -volname \"" << cpack_dmg_volume_name << "\"";
  temp_image_command << " -format " << temp_image_format;
  temp_image_command << " \"" << temp_image << "\"";

  if (!this->RunCommand(temp_image_command)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error generating temporary disk image." << std::endl);

    return 0;
  }

  if (remount_image) {
    // Store that we have a failure so that we always unmount the image
    // before we exit.
    bool had_error = false;

    std::ostringstream attach_command;
    attach_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
    attach_command << " attach";
    attach_command << " \"" << temp_image << "\"";

    std::string attach_output;
    if (!this->RunCommand(attach_command, &attach_output)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error attaching temporary disk image." << std::endl);

      return 0;
    }

    cmsys::RegularExpression mountpoint_regex(".*(/Volumes/[^\n]+)\n.*");
    mountpoint_regex.find(attach_output.c_str());
    std::string const temp_mount = mountpoint_regex.match(1);

    // Remove dummy padding file so we have enough space on RW image ...
    std::ostringstream dummy_padding;
    dummy_padding << temp_mount << "/.dummy-padding-file";
    if (!cmSystemTools::RemoveFile(dummy_padding.str())) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error removing dummy padding file."
                      << std::endl);

      had_error = true;
    }

    // Optionally set the custom icon flag for the image ...
    if (!had_error && !cpack_package_icon.empty()) {
      std::ostringstream setfile_command;
      setfile_command << this->GetOption("CPACK_COMMAND_SETFILE");
      setfile_command << " -a C";
      setfile_command << " \"" << temp_mount << "\"";

      if (!this->RunCommand(setfile_command)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Error assigning custom icon to temporary disk image."
                        << std::endl);

        had_error = true;
      }
    }

    // Optionally we can execute a custom apple script to generate
    // the .DS_Store for the volume folder ...
    if (!had_error && !cpack_dmg_ds_store_setup_script.empty()) {
      std::ostringstream setup_script_command;
      setup_script_command << "osascript"
                           << " \"" << cpack_dmg_ds_store_setup_script << "\""
                           << " \"" << cpack_dmg_volume_name << "\"";
      std::string error;
      if (!this->RunCommand(setup_script_command, &error)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Error executing custom script on disk image."
                        << std::endl
                        << error << std::endl);

        had_error = true;
      }
    }

    std::ostringstream detach_command;
    detach_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
    detach_command << " detach";
    detach_command << " \"" << temp_mount << "\"";

    if (!this->RunCommand(detach_command)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error detaching temporary disk image." << std::endl);

      return 0;
    }

    if (had_error) {
      return 0;
    }
  }

  if (!cpack_license_file.empty() || !slaDirectory.empty()) {
    // Use old hardcoded style if sla_dir is not set
    bool oldStyle = slaDirectory.empty();
    std::string sla_r = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
    sla_r += "/sla.r";

    std::vector<std::string> languages;
    if (!oldStyle) {
      cmSystemTools::ExpandListArgument(cpack_dmg_languages, languages);
    }

    cmGeneratedFileStream ofs(sla_r.c_str());
    ofs << "#include <CoreServices/CoreServices.r>\n\n";
    if (oldStyle) {
      ofs << SLAHeader;
      ofs << "\n";
    } else {
      /*
       * LPic Layout
       * (https://github.com/pypt/dmg-add-license/blob/master/main.c)
       * as far as I can tell (no official documentation seems to exist):
       * struct LPic {
       *  uint16_t default_language; // points to a resid, defaulting to 0,
       *                             // which is the first set language
       *  uint16_t length;
       *  struct {
       *    uint16_t language_code;
       *    uint16_t resid;
       *    uint16_t encoding; // Encoding from TextCommon.h,
       *                       // forcing MacRoman (0) for now. Might need to
       *                       // allow overwrite per license by user later
       *  } item[1];
       * }
       */

      // Create vector first for readability, then iterate to write to ofs
      std::vector<uint16_t> header_data;
      header_data.push_back(0);
      header_data.push_back(languages.size());
      for (size_t i = 0; i < languages.size(); ++i) {
        CFStringRef language_cfstring = CFStringCreateWithCString(
          NULL, languages[i].c_str(), kCFStringEncodingUTF8);
        CFStringRef iso_language =
          CFLocaleCreateCanonicalLanguageIdentifierFromString(
            NULL, language_cfstring);
        if (!iso_language) {
          cmCPackLogger(cmCPackLog::LOG_ERROR, languages[i]
                          << " is not a recognized language" << std::endl);
        }
        char* iso_language_cstr = (char*)malloc(65);
        CFStringGetCString(iso_language, iso_language_cstr, 64,
                           kCFStringEncodingMacRoman);
        LangCode lang = 0;
        RegionCode region = 0;
#ifdef HAVE_CoreServices
        OSStatus err =
          LocaleStringToLangAndRegionCodes(iso_language_cstr, &lang, &region);
        if (err != noErr)
#endif
        {
          cmCPackLogger(cmCPackLog::LOG_ERROR,
                        "No language/region code available for "
                          << iso_language_cstr << std::endl);
          free(iso_language_cstr);
          return 0;
        }
#ifdef HAVE_CoreServices
        free(iso_language_cstr);
        header_data.push_back(region);
        header_data.push_back(i);
        header_data.push_back(0);
#endif
      }
      ofs << "data 'LPic' (5000) {\n";
      ofs << std::hex << std::uppercase << std::setfill('0');

      for (size_t i = 0; i < header_data.size(); ++i) {
        if (i % 8 == 0) {
          ofs << "    $\"";
        }

        ofs << std::setw(4) << header_data[i];

        if (i % 8 == 7 || i == header_data.size() - 1) {
          ofs << "\"\n";
        } else {
          ofs << " ";
        }
      }
      ofs << "};\n\n";
      // Reset ofs options
      ofs << std::dec << std::nouppercase << std::setfill(' ');
    }

    bool have_write_license_error = false;
    std::string error;

    if (oldStyle) {
      if (!this->WriteLicense(ofs, 0, "", cpack_license_file, &error)) {
        have_write_license_error = true;
      }
    } else {
      for (size_t i = 0; i < languages.size() && !have_write_license_error;
           ++i) {
        if (singleLicense) {
          if (!this->WriteLicense(ofs, i + 5000, languages[i],
                                  cpack_license_file, &error)) {
            have_write_license_error = true;
          }
        } else {
          if (!this->WriteLicense(ofs, i + 5000, languages[i], "", &error)) {
            have_write_license_error = true;
          }
        }
      }
    }

    ofs.Close();

    if (have_write_license_error) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error writing license file to SLA."
                      << std::endl
                      << error << std::endl);
      return 0;
    }

    if (temp_image_format != "UDZO") {
      temp_image_format = "UDZO";
      // convert to UDZO to enable unflatten/flatten
      std::string temp_udzo = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
      temp_udzo += "/temp-udzo.dmg";

      std::ostringstream udco_image_command;
      udco_image_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
      udco_image_command << " convert \"" << temp_image << "\"";
      udco_image_command << " -format UDZO";
      udco_image_command << " -ov -o \"" << temp_udzo << "\"";

      if (!this->RunCommand(udco_image_command, &error)) {
        cmCPackLogger(cmCPackLog::LOG_ERROR,
                      "Error converting to UDCO dmg for adding SLA."
                        << std::endl
                        << error << std::endl);
        return 0;
      }
      temp_image = temp_udzo;
    }

    // unflatten dmg
    std::ostringstream unflatten_command;
    unflatten_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
    unflatten_command << " unflatten ";
    unflatten_command << "\"" << temp_image << "\"";

    if (!this->RunCommand(unflatten_command, &error)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error unflattening dmg for adding SLA." << std::endl
                                                             << error
                                                             << std::endl);
      return 0;
    }

    // Rez the SLA
    std::ostringstream embed_sla_command;
    embed_sla_command << this->GetOption("CPACK_COMMAND_REZ");
    const char* sysroot = this->GetOption("CPACK_OSX_SYSROOT");
    if (sysroot && sysroot[0] != '\0') {
      embed_sla_command << " -isysroot \"" << sysroot << "\"";
    }
    embed_sla_command << " \"" << sla_r << "\"";
    embed_sla_command << " -a -o ";
    embed_sla_command << "\"" << temp_image << "\"";

    if (!this->RunCommand(embed_sla_command, &error)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR, "Error adding SLA." << std::endl
                                                               << error
                                                               << std::endl);
      return 0;
    }

    // flatten dmg
    std::ostringstream flatten_command;
    flatten_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
    flatten_command << " flatten ";
    flatten_command << "\"" << temp_image << "\"";

    if (!this->RunCommand(flatten_command, &error)) {
      cmCPackLogger(cmCPackLog::LOG_ERROR,
                    "Error flattening dmg for adding SLA." << std::endl
                                                           << error
                                                           << std::endl);
      return 0;
    }
  }

  // Create the final compressed read-only disk image ...
  std::ostringstream final_image_command;
  final_image_command << this->GetOption("CPACK_COMMAND_HDIUTIL");
  final_image_command << " convert \"" << temp_image << "\"";
  final_image_command << " -format ";
  final_image_command << cpack_dmg_format;
  final_image_command << " -imagekey";
  final_image_command << " zlib-level=9";
  final_image_command << " -o \"" << output_file << "\"";

  if (!this->RunCommand(final_image_command)) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error compressing disk image."
                    << std::endl);

    return 0;
  }

  return 1;
}

bool cmCPackDragNDropGenerator::SupportsComponentInstallation() const
{
  return true;
}

std::string cmCPackDragNDropGenerator::GetComponentInstallDirNameSuffix(
  const std::string& componentName)
{
  // we want to group components together that go in the same dmg package
  std::string package_file_name = this->GetOption("CPACK_PACKAGE_FILE_NAME");

  // we have 3 mutually exclusive modes to work in
  // 1. all components in one package
  // 2. each group goes in its own package with left over
  //    components in their own package
  // 3. ignore groups - if grouping is defined, it is ignored
  //    and each component goes in its own package

  if (this->componentPackageMethod == ONE_PACKAGE) {
    return "ALL_IN_ONE";
  }

  if (this->componentPackageMethod == ONE_PACKAGE_PER_GROUP) {
    // We have to find the name of the COMPONENT GROUP
    // the current COMPONENT belongs to.
    std::string groupVar =
      "CPACK_COMPONENT_" + cmSystemTools::UpperCase(componentName) + "_GROUP";
    const char* _groupName = GetOption(groupVar);
    if (_groupName) {
      std::string groupName = _groupName;

      groupName =
        GetComponentPackageFileName(package_file_name, groupName, true);
      return groupName;
    }
  }

  return GetComponentPackageFileName(package_file_name, componentName, false);
}

bool cmCPackDragNDropGenerator::WriteLicense(
  cmGeneratedFileStream& outputStream, int licenseNumber,
  std::string licenseLanguage, std::string licenseFile, std::string* error)
{
  if (!licenseFile.empty() && !singleLicense) {
    licenseNumber = 5002;
    licenseLanguage = "English";
  }

  // License header
  outputStream << "data 'TEXT' (" << licenseNumber << ", \"" << licenseLanguage
               << "\") {\n";
  // License body
  std::string actual_license = !licenseFile.empty()
    ? licenseFile
    : (slaDirectory + "/" + licenseLanguage + ".license.txt");
  cmsys::ifstream license_ifs;
  license_ifs.open(actual_license.c_str());
  if (license_ifs.is_open()) {
    while (license_ifs.good()) {
      std::string line;
      std::getline(license_ifs, line);
      if (!line.empty()) {
        EscapeQuotesAndBackslashes(line);
        std::vector<std::string> lines;
        if (!this->BreakLongLine(line, lines, error)) {
          return false;
        }
        for (size_t i = 0; i < lines.size(); ++i) {
          outputStream << "        \"" << lines[i] << "\"\n";
        }
      }
      outputStream << "        \"\\n\"\n";
    }
    license_ifs.close();
  }

  // End of License
  outputStream << "};\n\n";
  if (!licenseFile.empty() && !singleLicense) {
    outputStream << SLASTREnglish;
  } else {
    // Menu header
    outputStream << "resource 'STR#' (" << licenseNumber << ", \""
                 << licenseLanguage << "\") {\n";
    outputStream << "    {\n";

    // Menu body
    cmsys::ifstream menu_ifs;
    menu_ifs.open(
      (slaDirectory + "/" + licenseLanguage + ".menu.txt").c_str());
    if (menu_ifs.is_open()) {
      size_t lines_written = 0;
      while (menu_ifs.good()) {
        // Lines written from original file, not from broken up lines
        std::string line;
        std::getline(menu_ifs, line);
        if (!line.empty()) {
          EscapeQuotesAndBackslashes(line);
          std::vector<std::string> lines;
          if (!this->BreakLongLine(line, lines, error)) {
            return false;
          }
          for (size_t i = 0; i < lines.size(); ++i) {
            std::string comma;
            // We need a comma after every complete string,
            // but not on the very last line
            if (lines_written != 8 && i == lines.size() - 1) {
              comma = ",";
            } else {
              comma = "";
            }
            outputStream << "        \"" << lines[i] << "\"" << comma << "\n";
          }
          ++lines_written;
        }
      }
      menu_ifs.close();
    }

    // End of menu
    outputStream << "    }\n";
    outputStream << "};\n";
    outputStream << "\n";
  }

  return true;
}

bool cmCPackDragNDropGenerator::BreakLongLine(const std::string& line,
                                              std::vector<std::string>& lines,
                                              std::string* error)
{
  const size_t max_line_length = 512;
  for (size_t i = 0; i < line.size(); i += max_line_length) {
    size_t line_length = max_line_length;
    if (i + line_length > line.size()) {
      line_length = line.size() - i;
    } else
      while (line_length > 0 && line[i + line_length - 1] != ' ') {
        line_length = line_length - 1;
      }

    if (line_length == 0) {
      *error = "Please make sure there are no words "
               "(or character sequences not broken up by spaces or newlines) "
               "in your license file which are more than 512 characters long.";
      return false;
    }
    lines.push_back(line.substr(i, line_length));
  }
  return true;
}

void cmCPackDragNDropGenerator::EscapeQuotesAndBackslashes(std::string& line)
{
  std::string::size_type backslash_pos = line.find('\\');
  while (backslash_pos != std::string::npos) {
    line.replace(backslash_pos, 1, "\\\\");
    backslash_pos = line.find('\\', backslash_pos + 2);
  }

  std::string::size_type quote_pos = line.find('\"');
  while (quote_pos != std::string::npos) {
    line.replace(quote_pos, 1, "\\\"");
    quote_pos = line.find('\"', quote_pos + 2);
  }
}
