/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMachO.h"

#include "cmsys/FStream.hxx"
#include <algorithm>
#include <stddef.h>
#include <string>
#include <vector>

// Include the Mach-O format information system header.
#include <mach-o/fat.h>
#include <mach-o/loader.h>

/**

  https://developer.apple.com/library/mac/documentation/
          DeveloperTools/Conceptual/MachORuntime/index.html

  A Mach-O file has 3 major regions: header, load commands and segments.
  Data Structures are provided from <mach-o/loader.h> which
  correspond to the file structure.

  The header can be either a struct mach_header or struct mach_header_64.
  One can peek at the first 4 bytes to identify the type of header.

  Following is the load command region which starts with
  struct load_command, and is followed by n number of load commands.

  In the case of a universal binary (an archive of multiple Mach-O files),
  the file begins with a struct fat_header and is followed by multiple
  struct fat_arch instances.  The struct fat_arch indicates the offset
  for each Mach-O file.

  */

namespace {

// peek in the file
template <typename T>
bool peek(cmsys::ifstream& fin, T& v)
{
  std::streampos p = fin.tellg();
  if (!fin.read(reinterpret_cast<char*>(&v), sizeof(T))) {
    return false;
  }
  fin.seekg(p);
  return fin.good();
}

// read from the file and fill a data structure
template <typename T>
bool read(cmsys::ifstream& fin, T& v)
{
  if (!fin.read(reinterpret_cast<char*>(&v), sizeof(T))) {
    return false;
  }
  return true;
}

// read from the file and fill multiple data structures where
// the vector has been resized
template <typename T>
bool read(cmsys::ifstream& fin, std::vector<T>& v)
{
  // nothing to read
  if (v.empty()) {
    return true;
  }
  if (!fin.read(reinterpret_cast<char*>(&v[0]), sizeof(T) * v.size())) {
    return false;
  }
  return true;
}
}

// Contains header and load commands for a single Mach-O file
class cmMachOHeaderAndLoadCommands
{
public:
  // A load_command and its associated data
  struct RawLoadCommand
  {
    uint32_t type(const cmMachOHeaderAndLoadCommands* m) const
    {
      if (this->LoadCommand.size() < sizeof(load_command)) {
        return 0;
      }
      const load_command* cmd =
        reinterpret_cast<const load_command*>(&this->LoadCommand[0]);
      return m->swap(cmd->cmd);
    }
    std::vector<char> LoadCommand;
  };

  cmMachOHeaderAndLoadCommands(bool _swap)
    : Swap(_swap)
  {
  }
  virtual ~cmMachOHeaderAndLoadCommands() {}

  virtual bool read_mach_o(cmsys::ifstream& fin) = 0;

  const std::vector<RawLoadCommand>& load_commands() const
  {
    return this->LoadCommands;
  }

  uint32_t swap(uint32_t v) const
  {
    if (this->Swap) {
      char* c = reinterpret_cast<char*>(&v);
      std::swap(c[0], c[3]);
      std::swap(c[1], c[2]);
    }
    return v;
  }

protected:
  bool read_load_commands(uint32_t ncmds, uint32_t sizeofcmds,
                          cmsys::ifstream& fin);

  bool Swap;
  std::vector<RawLoadCommand> LoadCommands;
};

// Implementation for reading Mach-O header and load commands.
// This is 32 or 64 bit arch specific.
template <class T>
class cmMachOHeaderAndLoadCommandsImpl : public cmMachOHeaderAndLoadCommands
{
public:
  cmMachOHeaderAndLoadCommandsImpl(bool _swap)
    : cmMachOHeaderAndLoadCommands(_swap)
  {
  }
  bool read_mach_o(cmsys::ifstream& fin)
  {
    if (!read(fin, this->Header)) {
      return false;
    }
    this->Header.cputype = swap(this->Header.cputype);
    this->Header.cpusubtype = swap(this->Header.cpusubtype);
    this->Header.filetype = swap(this->Header.filetype);
    this->Header.ncmds = swap(this->Header.ncmds);
    this->Header.sizeofcmds = swap(this->Header.sizeofcmds);
    this->Header.flags = swap(this->Header.flags);

    return read_load_commands(this->Header.ncmds, this->Header.sizeofcmds,
                              fin);
  }

protected:
  T Header;
};

bool cmMachOHeaderAndLoadCommands::read_load_commands(uint32_t ncmds,
                                                      uint32_t sizeofcmds,
                                                      cmsys::ifstream& fin)
{
  uint32_t size_read = 0;
  this->LoadCommands.resize(ncmds);
  for (uint32_t i = 0; i < ncmds; i++) {
    load_command lc;
    if (!peek(fin, lc)) {
      return false;
    }
    lc.cmd = swap(lc.cmd);
    lc.cmdsize = swap(lc.cmdsize);
    size_read += lc.cmdsize;

    RawLoadCommand& c = this->LoadCommands[i];
    c.LoadCommand.resize(lc.cmdsize);
    if (!read(fin, c.LoadCommand)) {
      return false;
    }
  }

  if (size_read != sizeofcmds) {
    this->LoadCommands.clear();
    return false;
  }

  return true;
}

class cmMachOInternal
{
public:
  cmMachOInternal(const char* fname);
  ~cmMachOInternal();

  // read a Mach-O file
  bool read_mach_o(uint32_t file_offset);

  // the file we are reading
  cmsys::ifstream Fin;

  // The archs in the universal binary
  // If the binary is not a universal binary, this will be empty.
  std::vector<fat_arch> FatArchs;

  // the error message while parsing
  std::string ErrorMessage;

  // the list of Mach-O's
  std::vector<cmMachOHeaderAndLoadCommands*> MachOList;
};

cmMachOInternal::cmMachOInternal(const char* fname)
  : Fin(fname)
{
  // Quit now if the file could not be opened.
  if (!this->Fin || !this->Fin.get()) {
    this->ErrorMessage = "Error opening input file.";
    return;
  }

  if (!this->Fin.seekg(0)) {
    this->ErrorMessage = "Error seeking to beginning of file.";
    return;
  }

  // Read the binary identification block.
  uint32_t magic = 0;
  if (!peek(this->Fin, magic)) {
    this->ErrorMessage = "Error reading Mach-O identification.";
    return;
  }

  // Verify the binary identification.
  if (!(magic == MH_CIGAM || magic == MH_MAGIC || magic == MH_CIGAM_64 ||
        magic == MH_MAGIC_64 || magic == FAT_CIGAM || magic == FAT_MAGIC)) {
    this->ErrorMessage = "File does not have a valid Mach-O identification.";
    return;
  }

  if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
    // this is a universal binary
    fat_header header;
    if (!read(this->Fin, header)) {
      this->ErrorMessage = "Error reading fat header.";
      return;
    }

    // read fat_archs
    this->FatArchs.resize(OSSwapBigToHostInt32(header.nfat_arch));
    if (!read(this->Fin, this->FatArchs)) {
      this->ErrorMessage = "Error reading fat header archs.";
      return;
    }

    // parse each Mach-O file
    for (size_t i = 0; i < this->FatArchs.size(); i++) {
      const fat_arch& arch = this->FatArchs[i];
      if (!this->read_mach_o(OSSwapBigToHostInt32(arch.offset))) {
        return;
      }
    }
  } else {
    // parse Mach-O file at the beginning of the file
    this->read_mach_o(0);
  }
}

cmMachOInternal::~cmMachOInternal()
{
  for (size_t i = 0; i < this->MachOList.size(); i++) {
    delete this->MachOList[i];
  }
}

bool cmMachOInternal::read_mach_o(uint32_t file_offset)
{
  if (!this->Fin.seekg(file_offset)) {
    this->ErrorMessage = "Failed to locate Mach-O content.";
    return false;
  }

  uint32_t magic;
  if (!peek(this->Fin, magic)) {
    this->ErrorMessage = "Error reading Mach-O identification.";
    return false;
  }

  cmMachOHeaderAndLoadCommands* f = NULL;
  if (magic == MH_CIGAM || magic == MH_MAGIC) {
    bool swap = false;
    if (magic == MH_CIGAM) {
      swap = true;
    }
    f = new cmMachOHeaderAndLoadCommandsImpl<mach_header>(swap);
  } else if (magic == MH_CIGAM_64 || magic == MH_MAGIC_64) {
    bool swap = false;
    if (magic == MH_CIGAM_64) {
      swap = true;
    }
    f = new cmMachOHeaderAndLoadCommandsImpl<mach_header_64>(swap);
  }

  if (f && f->read_mach_o(this->Fin)) {
    this->MachOList.push_back(f);
  } else {
    delete f;
    this->ErrorMessage = "Failed to read Mach-O header.";
    return false;
  }

  return true;
}

//============================================================================
// External class implementation.

cmMachO::cmMachO(const char* fname)
  : Internal(0)
{
  this->Internal = new cmMachOInternal(fname);
}

cmMachO::~cmMachO()
{
  delete this->Internal;
}

std::string const& cmMachO::GetErrorMessage() const
{
  return this->Internal->ErrorMessage;
}

bool cmMachO::Valid() const
{
  return !this->Internal->MachOList.empty();
}

bool cmMachO::GetInstallName(std::string& install_name)
{
  if (this->Internal->MachOList.empty()) {
    return false;
  }

  // grab the first Mach-O and get the install name from that one
  cmMachOHeaderAndLoadCommands* macho = this->Internal->MachOList[0];
  for (size_t i = 0; i < macho->load_commands().size(); i++) {
    const cmMachOHeaderAndLoadCommands::RawLoadCommand& cmd =
      macho->load_commands()[i];
    uint32_t lc_cmd = cmd.type(macho);
    if (lc_cmd == LC_ID_DYLIB || lc_cmd == LC_LOAD_WEAK_DYLIB ||
        lc_cmd == LC_LOAD_DYLIB) {
      if (sizeof(dylib_command) < cmd.LoadCommand.size()) {
        uint32_t namelen = cmd.LoadCommand.size() - sizeof(dylib_command);
        install_name.assign(&cmd.LoadCommand[sizeof(dylib_command)], namelen);
        return true;
      }
    }
  }

  return false;
}

void cmMachO::PrintInfo(std::ostream& /*os*/) const
{
}
