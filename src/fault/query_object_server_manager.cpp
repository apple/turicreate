/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <boost/bind.hpp>
#include <fault/query_object_server_manager.hpp>
#include <fault/query_object_client.hpp>
#include <fault/query_object_create_flags.hpp>
#include <fault/query_object_server_common.hpp>
#include <fault/query_object_server_internal_signals.hpp>
#include <fault/message_flags.hpp>

namespace libfault {


/**************************************************************************/
/*                                                                        */
/*                                 Design                                 */
/*                                                                        */
/**************************************************************************/
/**

  The zookeeper K-V store (key-value) is used used to register each object.
  For an object with a key KEY, the following entries are created

  KEY -->  address to reach the master object. This is the address where
           query/update messages can be sent.
  KEY.PUB --> master object publish address. Connecting to this address
              with a SUB socket will receive all update messages.
  KEY.1  --> address of 1st slave. This is the address query messages can
             be sent.
  KEY.2  --> address of 2nd slave. This is the address query messages can
             be sent.

  Master Election
  ---------------
  Master election is performed the following way.



  */

static query_object_server_manager* object_manager = NULL;

/* SIGCHLD handler. */
static void sigchld_callback(int sig) {
  while (1) {
    int ret = waitpid(-1, NULL, WNOHANG);
    if (ret > 0) {
      // keep looping as long as there are processes to kill
      if (object_manager) object_manager->cleanup(ret);
      continue;
    } else {
      break;
    }
  }
}


query_object_server_manager::query_object_server_manager(std::string program,
                                         size_t replica_count,
                                         size_t objectcap)
    :program(program), objectcap(objectcap),
    replica_count(replica_count), auto_created_masters_count(0) {

  if(object_manager != NULL) {
    std::cout << "Only one server manager can be active at any point\n";
    assert(object_manager == NULL);
  }
  object_manager = this;
  std::cout << "Creating Object Server with a maximum object capacity of "
            << objectcap << "\n";
  std::cout << "Each object has up to " << replica_count << " additional replicas\n";
  std::cout << "Using " << program << " as the object server\n";

  // validate the program exists
  int ret = access(program.c_str(), X_OK);
  if (ret < 0) {
    if (errno == EACCES) {
      std::cerr << program << " cannot be executed. Make sure the program permissions are valid\n";
    } else if (errno == EIO || errno == ELOOP || errno == ENAMETOOLONG ||
               errno == ENOENT || errno == ENOTDIR){
      std::cerr << program << " cannot be accessed. Make sure the program exists and is reachable\n";
    }
    else {
      std::cerr << "Unexpected error " << errno << " accessing " << program << "\n";
    }
    exit(0);
  }
  zk_keyval = NULL;
  zk_kv_callback_id = -1;

  // register signal handler
  sigemptyset(&sigchldset);
  sigaddset(&sigchldset, SIGCHLD);
  prev_sighandler = signal(SIGCHLD, sigchld_callback);
  sigprocmask(SIG_UNBLOCK, &sigchldset, NULL);
}

query_object_server_manager::~query_object_server_manager() {
  stop();
  signal(SIGCHLD, prev_sighandler);
}

bool query_object_server_manager::register_zookeeper(std::vector<std::string> zkhosts,
                                             std::string prefix) {
  zk_hosts = zkhosts;
  zk_prefix = prefix;
  zk_keyval = new turi::zookeeper_util::key_value(zkhosts, prefix, "");
  return true;
}


void query_object_server_manager::cleanup(pid_t pid) {
  // since signals are disabled every time locks are acquired
  // it is safe to lock here
  lock.lock();
  for (size_t i = 0; i < objects.size(); ++i) {
    if (objects[i].pid == pid) {
      std::cout << "Cleanup of " << objects[i].objectkey << ":" << objects[i].replicaid << "\n";
      managed_keys.erase(managed_keys.find(objects[i].objectkey));
      managed_zkkeys.erase(get_zk_objectkey_name(objects[i].objectkey,
                                                 objects[i].replicaid));
      if (objects[i].replicaid == 0) --auto_created_masters_count;
      objects.erase(objects.begin() + i);
      break;
    }
  }
  lock.unlock();
}


void query_object_server_manager::lock_and_block_signal() {
  sigprocmask(SIG_BLOCK, &sigchldset, NULL);
  lock.lock();
}

void query_object_server_manager::unlock_and_unblock_signal() {
  lock.unlock();
  sigprocmask(SIG_UNBLOCK, &sigchldset, NULL);
}

void query_object_server_manager::set_all_object_keys(const std::vector<std::string>& master_space) {
  masterspace = master_space;
}


std::vector<std::string> query_object_server_manager::build_arguments() {
  std::vector<std::string> ret;
  // first argument is zkhosts, comma seperated
  {
    std::stringstream strm;
    for (size_t i = 0;i < zk_hosts.size(); ++i) {
      strm << zk_hosts[i];
      if (i != zk_hosts.size() - 1) strm << ",";
    }
    strm.flush();
    ret.push_back(strm.str());
  }

  // 2nd argument is prefix
  ret.push_back(zk_prefix);
  return ret;
}

void query_object_server_manager::start(size_t max_masters) {
  if (zk_kv_callback_id != -1) return;
  // register for a zookeeper callback. Important to do this outside of the lock
  zk_kv_callback_id = zk_keyval->add_callback(
      boost::bind(&query_object_server_manager::keyval_change, this, _1, _2, _3, _4));
  initial_max_masters = max_masters;
  check_managed_objects(max_masters);
}

void query_object_server_manager::stop() {


  if (zk_kv_callback_id == -1) return;
  // unregister the zookeeper callback
  zk_keyval->remove_callback(zk_kv_callback_id);
  zk_kv_callback_id = -1;

  lock_and_block_signal();
  // killall objects
  std::cout << "Deactivating all objects\n";
  for (size_t i = 0;i < objects.size(); ++i) {
    // unregister the object
    const char* stop = "0\n";
    write(objects[i].outfd, stop, strlen(stop));
  }
  unlock_and_unblock_signal();
}



void query_object_server_manager::spawn_object(std::string objectkey, size_t replicaid) {
  std::cout << "Spawning " << objectkey << ": " << replicaid << "\n\n";
  objects.push_back(managed_object());
  managed_object& mo = objects[objects.size() - 1];
  mo.objectkey = objectkey;
  mo.replicaid = replicaid;
  managed_keys.insert(mo.objectkey);
  managed_zkkeys.insert(get_zk_objectkey_name(objectkey, replicaid));
  int fd[2];
  pipe(fd);
  mo.outfd = fd[1];
  mo.pid = fork();
  if (mo.pid == 0) {
    // we are the child
    // connect stdin
    dup2(fd[0], STDIN_FILENO);
    std::vector<std::string> args = build_arguments();
    char objectname[mo.objectkey.length() + 10];
    sprintf(objectname,"%s:%ld", mo.objectkey.c_str(), mo.replicaid);
    execlp(program.c_str(), program.c_str(),
           args[0].c_str(), args[1].c_str(), objectname, NULL);
  }
}


bool query_object_server_manager::some_replica_exists(std::string objectkey)  {
  for (size_t rep = 1; rep <= replica_count; ++rep) {
    if (zk_keyval->get(get_zk_objectkey_name(objectkey, rep)).first) {
      return true;
    }
  }
  return false;
}


void query_object_server_manager::check_managed_objects(size_t max_masters)  {
  if (objects.size() >= objectcap) return;
  lock_and_block_signal();
  // search for masters I can spawn
  if (objects.size() < objectcap && auto_created_masters_count <  max_masters) {
    for (size_t i = 0;i < masterspace.size(); ++i) {
      if (zk_keyval->get(masterspace[i]).first == false) {
        if (!some_replica_exists(masterspace[i])) {
            spawn_object(masterspace[i], 0);
            ++auto_created_masters_count;
            if (objects.size() >= objectcap || auto_created_masters_count >= max_masters) break;
        }
      }
    }
  }
  if (objects.size() < objectcap) {
    // get a list of the current objects I am managing.
    // makes no sense to manage more than one copy of any object
    for (size_t i = 0;i < masterspace.size(); ++i) {
      // I do not already manage something belonging to this object key
      if (managed_keys.find(masterspace[i]) == managed_keys.end()) {
        // master must exist
        if (zk_keyval->get(masterspace[i]).first) {
          for (size_t rep = 1; rep <= replica_count; ++rep) {
            //this particular replica must not exist
            if (zk_keyval->get(get_zk_objectkey_name(masterspace[i], rep)).first == false) {
              spawn_object(masterspace[i], rep);
              break;
            }
          }
        }
      }
      if (objects.size() >= objectcap) break;
    }
  }
  unlock_and_unblock_signal();
}

/**
 * Signals that some sets of keys have changed and we should refresh
 * some values. May be called from a different thread
 */
void query_object_server_manager::keyval_change(turi::zookeeper_util::key_value* unused,
                                        const std::vector<std::string>& newkeys,
                                        const std::vector<std::string>& deletedkeys,
                                        const std::vector<std::string>& modifiedkeys) {
  // TODO: This should do master election
  if (deletedkeys.size() > 0) check_managed_objects();
  lock_and_block_signal();

  // remove myself from newkeys.
  size_t actual_newkeys = 0;
  for (size_t i = 0;i < newkeys.size(); ++i) {
    if (managed_zkkeys.find(newkeys[i]) == managed_zkkeys.end()) {
      ++actual_newkeys;
    }
  }
  unlock_and_unblock_signal();
  if (++actual_newkeys) check_managed_objects(initial_max_masters);
}


bool query_object_server_manager::stop_managing_object(std::string objectkey) {
  // TODO: This should be also block the key from being reintroduced on this
  // machine for a little while
  bool ret = false;
  lock_and_block_signal();
  for (size_t i = 0;i < objects.size(); ++i) {
    if (objectkey == objects[i].objectkey) {
      // unregister the object
      std::cout << "Deactivating object: " << objectkey << "\n";
      const char* stop = QO_SERVER_STOP_STR;
      write(objects[i].outfd, stop, strlen(stop));
      ret = true;
      break;
    }
  }
  unlock_and_unblock_signal();
  return ret;
}


void query_object_server_manager::print_all_object_names() {
  lock_and_block_signal();
  for (size_t i = 0;i < objects.size(); ++i) {
    // unregister the object
    const char* print= QO_SERVER_PRINT_STR;
    write(objects[i].outfd, print, strlen(print));
  }
  unlock_and_unblock_signal();
}


} // namespace libfault
