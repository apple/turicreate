/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <set>
#include <iostream>
#include <fstream>
#include <chrono>


#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/atomic.hpp>
#include <core/logging/assertions.hpp>
#include <timer/timer.hpp>

#include <core/random/random.hpp>

#ifdef _WIN32
#include <cross_platform/windows_wrapper.hpp>
#include <Wincrypt.h>
#endif

#include <core/util/syserr_reporting.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi {
  namespace random {

    /**
     * A truely nondeterministic generator
     */
    class nondet_generator {
    public:
      static nondet_generator& global() {
        static nondet_generator global_gen;
        return global_gen;
      }

      nondet_generator() 
       : _random_devices(thread::cpu_count())
      {} 
         
      typedef unsigned int result_type;
      
      static constexpr result_type min() { return std::numeric_limits<unsigned int>::min(); }
      static constexpr result_type max() { return std::numeric_limits<unsigned int>::max(); }

      // read a size_t from the source
      inline result_type operator()() {
        int thread_id = thread::thread_id();

        return _random_devices.at(thread_id)();
      }

      std::vector<std::random_device> _random_devices;
    };
    //nondet_generator global_nondet_rng;

    /** Use the C++11 standard generato to get as close to a true source of randomness as possible. 
     *
     *  Returns a 64 bit truely random seed. 
     */
    uint64_t pure_random_seed() { 
      return hash64_combine(hash64(nondet_generator::global()()), hash64(nondet_generator::global()()));
    }

    /**
     * This class represents a master registery of all active random
     * number generators
     */
    struct source_registry {
      std::set<generator*> generators;
      generator master;
      mutex mut;

      static source_registry& global() {
        static source_registry registry;
        return registry;
      }
      /**
       * Seed all threads using the default seed
       */
      void seed() {
        get_source().seed();
      }

      /**
       * Seed all threads using the default seed
       */
      void nondet_seed() {
        get_source().nondet_seed();
      }


      /**
       * Seed all threads using the default seed
       */
      void time_seed() {
        get_source().time_seed();
      }


      /**
       *  Seed all threads with a fixed number
       */
      void seed(const size_t number) {
        get_source().seed(number);
      }

      /**
       * Register a source with the registry and seed it based on the
       * master.
       */
      void register_generator(generator* tls_ptr) {
        ASSERT_TRUE(tls_ptr != NULL);
        mut.lock();
        generators.insert(tls_ptr);
        tls_ptr->seed(master);
        // std::cerr << "Generator created" << std::endl;
        // __print_back_trace();
        mut.unlock();
      }

      /**
       * Unregister a source from the registry
       */
      void unregister_source(generator* tls_ptr) {
        mut.lock();
        generators.erase(tls_ptr);
        mut.unlock();
      }
    };
    // source_registry registry;








    //////////////////////////////////////////////////////////////
    /// Pthread TLS code

    /**
     * this function is responsible for destroying the random number
     * generators
     */
    void destroy_tls_data(void* ptr) {
      generator* tls_rnd_ptr =
        reinterpret_cast<generator*>(ptr);
      if(tls_rnd_ptr != NULL) {
        /// TOFIX: This has issues... The global may have been destroyed already
        //source_registry::global().unregister_source(tls_rnd_ptr);
        delete tls_rnd_ptr;
      }
    }


    /**
     * Simple struct used to construct the thread local storage at
     * startup.
     */
    struct tls_key_creator {
      pthread_key_t TLS_RANDOM_SOURCE_KEY;
      tls_key_creator() : TLS_RANDOM_SOURCE_KEY(0) {
        pthread_key_create(&TLS_RANDOM_SOURCE_KEY,
                           destroy_tls_data);
      }
    };
    // This function is to be called prior to any access to the random
    // source
    static pthread_key_t get_random_source_key() {
      static const tls_key_creator key;
      return key.TLS_RANDOM_SOURCE_KEY;
    }
    // This forces __init_keys__ to be called prior to main.
    static TURI_ATTRIBUTE_UNUSED pthread_key_t __unused_init_keys__(get_random_source_key());

  // the combination of the two mechanisms above will force the
  // thread local store to be initialized
  // 1: before main
  // 2: before any use of random by global variables.
  // KNOWN_ISSUE: if a global variable (initialized before main)
  //               spawns threads which then call random. Things explode.


    /////////////////////////////////////////////////////////////
    //// Implementation of header functions



    generator& get_source() {
      // get the thread local storage
      generator* tls_rnd_ptr =
        reinterpret_cast<generator*>
        (pthread_getspecific(get_random_source_key()));
      // Create a tls_random_source if none was provided
      if(tls_rnd_ptr == NULL) {
        tls_rnd_ptr = new generator();
        assert(tls_rnd_ptr != NULL);
        // This will seed it with the master rng
        source_registry::global().register_generator(tls_rnd_ptr);
        pthread_setspecific(get_random_source_key(),
                            tls_rnd_ptr);
      }
      // assert(tls_rnd_ptr != NULL);
      return *tls_rnd_ptr;
    } // end of get local random source



    void seed() { source_registry::global().seed();  }

    void nondet_seed() { source_registry::global().nondet_seed(); }

    void time_seed() { source_registry::global().time_seed(); }

    void seed(const size_t seed_value) {
      source_registry::global().seed(seed_value);
    }


    void generator::nondet_seed() {
      // Get the global nondeterministic random number generator.
      nondet_generator& nondet_rnd(nondet_generator::global());
      mut.lock();
      
      // std::cerr << "initializing real rng" << std::endl;
      m_rng.seed(static_cast<uint32_t>(nondet_rnd()));
 
      mut.unlock();
    }


    void pdf2cdf(std::vector<double>& pdf) {
      double Z = 0;
      for(size_t i = 0; i < pdf.size(); ++i) Z += pdf[i];
      for(size_t i = 0; i < pdf.size(); ++i)
        pdf[i] = pdf[i]/Z + ((i>0)? pdf[i-1] : 0);
    } // end of pdf2cdf




  }; // end of namespace random

};// end of namespace turi
