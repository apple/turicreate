/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <iostream>
#include <boost/program_options.hpp>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <core/logging/logger.hpp>
#include <core/globals/globals.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/s3_api.hpp>

namespace po = boost::program_options;

constexpr size_t BUFFER_SIZE = 4 * 1024 * 1024; /* 4MB */
void print_help(char** argv) {
  std::cout << "Usage: \n";
  std::cout << argv[0] << " cp [src] [dst]\n";
  std::cout << argv[0] << " mkdir [dst]\n";
  std::cout << argv[0] << " ls [path]\n";
  std::cout << argv[0] << " rm [path]    # deletes one file\n";
  std::cout << argv[0] << " rmr [path]    # recursive deletion of a directory\n\n";
  std::cout << "All paths can be local, hdfs, or s3:// paths\n";
  std::cout << "The environment variables AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY will be used if available\n";
  std::cout << "file globs are supported for ls. Not for the rest\n";
}

void include_s3_environment(std::string& path) {
  if (boost::starts_with(path, "s3://")) {
    size_t colon_count = 0;
    for (char c: path) colon_count += (c == ':');
    // we probably already have the secret keys in the URL
    if (colon_count >= 2) return;
    // otherwise, lets try to add the keys
    const char* accesskey = getenv("AWS_ACCESS_KEY_ID");
    const char* secretkey = getenv("AWS_SECRET_ACCESS_KEY");
    if (accesskey != NULL && secretkey != NULL) {
      path = path.substr(5); // remove the s3://
      if (boost::starts_with(path, "/")) path = path.substr(1); // people may accidentally put s3:///
      path = "s3://" + std::string(accesskey) + ":" + std::string(secretkey) + ":" + path;
    }
  }
}

bool file_copy(std::string srcpath, std::string dstpath) {
  try {
    turi::general_ifstream fin(srcpath);
    turi::general_ofstream fout(dstpath);
    char* buffer = new char[BUFFER_SIZE];
    size_t filesize = fin.file_size();
    std::cout << "Copying " << filesize << " bytes\n";
    while(fin.good()) {
      fin.read(buffer, BUFFER_SIZE);
      fout.write(buffer, fin.gcount());
    }
  } catch (std::string s) {
    std::cerr << s << "\n";
    return false;
  }
  return true;
}


bool recursive_copy(std::string srcpath, std::string dstpath) {
  // both src and dst must be directories
  // I am copying the contents of src into the contents of dst
  // create descendent directories
  bool success = turi::fileio::create_directory(dstpath);
  if (!success) {
    std::cerr << "Unable to create directory at "
              << turi::sanitize_url(dstpath) << "\n";
    return false;
  }

  auto srcentries = turi::fileio::get_directory_listing(srcpath);
  for(auto srcfile: srcentries) {
    std::string dstfile = dstpath + "/" + turi::fileio::get_filename(srcfile.first);
    if (srcfile.second == turi::fileio::file_status::REGULAR_FILE) {
      // is a file. copy it
      success = file_copy(srcfile.first, dstfile);
    } else {
      // is a directory. recursively copy it
      success = recursive_copy(srcfile.first, dstfile);
    }
    if (!success) return false;
  }
  return true;
}

/// cp implementation. Behaves like cp.
int cp_impl(std::string srcpath, std::string dstpath) {

  auto src_type = turi::fileio::get_file_status(srcpath);
  auto dst_type = turi::fileio::get_file_status(dstpath);
  if (src_type.first == turi::fileio::file_status::MISSING) {
    // missing source file
    logstream(LOG_ERROR) << '\'' << srcpath << '\'' << " missing. " << __FILE__
                         << " at " << __LINE__ << ". Err: " << src_type.second << std::endl;
    return 1;
  } else if (src_type.first == turi::fileio::file_status::REGULAR_FILE) {
    // source file is a single file
    if (dst_type.first == turi::fileio::file_status::MISSING ||
        dst_type.first == turi::fileio::file_status::REGULAR_FILE) {
      // target is missing, or I am overwriting
      file_copy(srcpath, dstpath);
    } else {
      // if target is a directory... we need to come up with the target filename
      file_copy(srcpath, dstpath + "/" + turi::fileio::get_filename(srcpath));
    }
  } else if (src_type.first == turi::fileio::file_status::DIRECTORY) {
    // copying a directory
    if (dst_type.first == turi::fileio::file_status::REGULAR_FILE) {
      logstream(LOG_ERROR) << '\'' << srcpath << '\'' << " exists. "
                           << "Cannot create target directory." << std::endl;
      return 1;
    } else {
      recursive_copy(srcpath, dstpath + "/" + turi::fileio::get_filename(srcpath));
    }
  }
  return 0;
}

/// glob free ls
int simple_ls_impl(std::string url) {
  try {
    auto response = turi::fileio::get_directory_listing(url);
    for(auto entry: response) {
      std::cout << turi::sanitize_url(entry.first);
      if (entry.second == turi::fileio::file_status::DIRECTORY) {
        std::cout << "/";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << response.size() << " entries found\n";
  } catch (std::string s) {
    std::cerr << s << "\n";
    return 1;
  }
  return 0;
}


std::string glob_to_regex(std::string glob) {
  // this is horribly incomplete. But works sufficiently
  boost::replace_all(glob, "/", "\\/");
  boost::replace_all(glob, "?", ".");
  boost::replace_all(glob, "*", ".*");
  return glob;
}

/// enumerates contents of "url", testing the rest against the glob
int glob_ls_impl(std::string url, std::string glob) {
  try {
    size_t ctr = 0;
    auto response = turi::fileio::get_directory_listing(url);
    std::regex glob_regex(glob_to_regex(glob));
    for(auto entry: response) {
      if (std::regex_match(turi::fileio::get_filename(entry.first), glob_regex)) {
        std::cout << turi::sanitize_url(entry.first);
        ++ctr;
        if (entry.second == turi::fileio::file_status::DIRECTORY) {
          std::cout << "/";
        }
        std::cout << "\n";
      }
    }
    std::cout << "\n";
    std::cout << ctr << " entries found\n";
  } catch (std::string s) {
    std::cerr << s << "\n";
    return 1;
  }
  std::cout << "\n";
  return 0;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    print_help(argv);
    return 0;
  }
  turi::globals::initialize_globals_from_environment(argv[0]);
  std::string command = argv[1];
  if (command == "cp" && argc == 4) {
    std::string srcpath = argv[2];
    std::string dstpath = argv[3];
    // if srcpath or dstpath have a trailing slash, drop it
    if (boost::ends_with(srcpath, "/")) srcpath = srcpath.substr(0, srcpath.length() - 1);
    if (boost::ends_with(dstpath, "/")) dstpath = dstpath.substr(0, dstpath.length() - 1);
    include_s3_environment(srcpath);
    include_s3_environment(dstpath);
    return cp_impl(srcpath, dstpath);

  } else if (command == "mkdir" && argc == 3) {
    std::string dstpath = argv[2];
    bool success = turi::fileio::create_directory(dstpath);
    if (!success) {
      std::cerr << "Unable to create directory at "
                << turi::sanitize_url(dstpath) << "\n";
      return 1;
    }
  } else if (command == "rm" && argc == 3) {
    std::string dstpath = argv[2];
    include_s3_environment(dstpath);
    bool success = turi::fileio::delete_path(dstpath);
    if (!success) {
      std::cerr << "Unable to delete path at "
                << turi::sanitize_url(dstpath) << "\n";
      return 1;
    }
  } else if (command == "rmr" && argc == 3) {
    std::string dstpath = argv[2];
    include_s3_environment(dstpath);
    bool success = turi::fileio::delete_path_recursive(dstpath);
    if (!success) {
      std::cerr << "Unable to recursively delete path at "
                << turi::sanitize_url(dstpath) << "\n";
      return 1;
    }
  } else if (command == "ls" && argc == 3) {
    std::string url = argv[2];
    include_s3_environment(url);
    std::string filename = turi::fileio::get_filename(url);
    // is there globs?
    size_t globchars = 0;
    for (char c: filename) globchars += (c == '*' || c == '?');

    if (globchars == 0) {
      return simple_ls_impl(url);
    } else {
      return glob_ls_impl(turi::fileio::get_dirname(url), filename);
    }

  } else if (command == "--help") {
    print_help(argv);
  } else {
    std::cout << "Invalid command\n";
    return 1;
  }
  return 0;
}
