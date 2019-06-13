/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FAST_MULTINOMIAL_HPP
#define TURI_FAST_MULTINOMIAL_HPP

#include <vector>
#include <algorithm>


#include <boost/integer.hpp>
#include <boost/random.hpp>

#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/atomic.hpp>

#include <core/generics/float_selector.hpp>




namespace turi {


  /// \ingroup util_internal
  class fast_multinomial {
    // system word length float
    typedef float_selector<sizeof(size_t)>::float_type float_t;

    //! First leaf index
    size_t first_leaf_index;

    //! The number of assignments to the multinomial
    size_t num_asg;

    //! The tree datastructure
    std::vector<float_t> tree;

    //! the number of positive probability elements
    atomic<size_t> num_support;




    // Helper routines
    // ========================================================>

    //! Compute the next power of 2
    //     size_t next_powerof2(size_t val) {
    //       size_t powof2 = 1;
    //       while(powof2 < val) powof2 = powof2 * 2;
    //       return powof2;
    //     }

    //! Clever next power of two bit magic
    uint64_t next_powerof2(uint64_t val) {
      --val;
      val = val | (val >> 1);
      val = val | (val >> 2);
      val = val | (val >> 4);
      val = val | (val >> 8);
      val = val | (val >> 16);
      val = val | (val >> 32);
      return val + 1;
    }




    //! Returns the index of the left child of the supplied index.
    size_t left_child(size_t i) const { return 2 * i + 1; }

    //! Returns the index of the right child of the supplied index.
    size_t right_child(size_t i) const { return 2 * i + 2; }

    //! Returns the index of the parent of the supplied index.
    size_t parent(size_t i) const { return (i-1) / 2; }

    //! returns the sibling of
    size_t sibling(size_t i) const {
      // the binary here is equivalent to (+1) if leaf is odd and (-1) if
      // leaf is even
      return i + (i & 1)*2 - 1;
    }

    //! get the tree location of the assignment
    size_t tree_loc_from_asg(size_t asg) const {
      size_t loc = asg + first_leaf_index;
      assert(loc < tree.size());
      assert(is_leaf(loc));
      return loc;
    }

    //! determine the assignment from the location in the tree
    size_t asg_from_tree_loc(size_t i) const {
      assert(is_leaf(i));
      size_t asg = i - first_leaf_index;
      assert(asg < num_asg);
      return asg;
    }


    //! Returns true if the index corresponds to a leaf
    bool is_leaf(size_t i) const {
      return i >= first_leaf_index;
      //      return left_child(i) > tree.size();
    }

    //! Returns true if the location is the root
    bool is_root(size_t i) const { return i == 0; }


    /// Returns the index of a leaf sampled proportionate to its
    /// priority.  Returns false on failure
    bool try_sample(size_t& asg, size_t cpuid) {
      size_t loc = 0;
      while ( !is_leaf(loc) ) {
        // get the left and right priorities
        float_t left_p = tree.at(left_child(loc));
        float_t right_p = tree.at(right_child(loc));
        // if both are zero, the sample has failed. Return
        if (left_p + right_p == 0) return false;
        else if (right_p == 0) loc = left_child(loc);
        else if (left_p == 0)  loc = right_child(loc);
        else {
          // pick from a bernoulli trial
          float_t childsum = left_p + right_p;
          float_t rndnumber = turi::random::uniform<float_t>(0,1);
          if((childsum * rndnumber)  < left_p)
            loc = left_child(loc);
          else
            loc = right_child(loc);
        }
      }
      assert(is_leaf(loc));
      asg = asg_from_tree_loc(loc);
      assert(asg < num_asg);
      return true;
    } // end of sample index


    /// Propagates a cumulative sum update up the tree.
    void propagate_change(size_t loc) {
      // Loop while the location is not the root each time moving up
      // the tree
      for( ; !is_root(loc); loc = parent(loc) ) {
        // Get the sibbling of this location
        size_t sibling_loc = sibling(loc);
        assert(sibling_loc < tree.size());
        // Get the parent
        size_t parent_loc = parent(loc);
        assert(parent_loc < tree.size());
        // Assert that the sibling is infact the sibling
        assert(parent_loc == parent(sibling_loc));
        // Get the priority of this location and the sibling
        // and the parent
        volatile float_t* sibling1 = &(tree[loc]);
        volatile float_t* sibling2 = &(tree[sibling_loc]);
        volatile float_t* parent = &(tree[parent_loc]);

        // write to the parent. Use a concurrent write mechanism
        // size_t spin_count = 0;
        //float_t old_value = *parent;
        //        float_t new_value = *sibling1 + *sibling2;
        //        while(!atomic_compare_and_swap(tree[parent_loc], old_value, new_value)) {
        //          old_value = *parent;
        //          new_value = *sibling1 + *sibling2;
        // if(++spin_count % 10 == 0) {
        //   std::cerr << "Propagate_change: " << spin_count << std::endl;
        // }
        //      }
        while(true) {
          float_t sum = (*sibling1) + (*sibling2);
          (*parent) = sum;
          __asm("mfence");
          float_t sum2 = (*sibling1) + (*sibling2);
          float_t parentval = (*parent);
          if (sum2 == parentval) break;
        }
        // If the update was successful accomplished by anothe thread
        // than return
        //if(old_value == new_value) return;
      } // end of for loop
    } // end of propagate change


  public:

    /** initialize a fast multinomail */
    fast_multinomial(size_t num_asg,
                     size_t ncpus) :
      first_leaf_index(0),
      num_asg(num_asg) {
      // // initialize the generators
      // for(size_t i = 0; i < rngs.size(); ++i) {
      //   rngs[i].seed(rand());
      //   distributions.push_back(distribution_type(rngs[i]));
      // }
      // Determine the size of the tree
      first_leaf_index = next_powerof2(num_asg) - 1;
      size_t tree_size = first_leaf_index + next_powerof2(num_asg);
      tree.resize(tree_size, 0.0);
    }

    void zero(size_t asg) {
      assert(asg < num_asg);
      size_t loc = tree_loc_from_asg(asg);
      // Use CAS
      float_t old_value = tree[loc];
      float_t new_value = 0;
      while(!atomic_compare_and_swap(tree[loc], old_value, new_value)){
        old_value = tree[loc];
      }
      propagate_change(loc);
      if(old_value > 0) {
        num_support.dec();
      }
    }

    //! Set a leaf value
    void set(size_t asg, float_t value) {
      assert(asg < num_asg);
      assert(value >= 0);
      size_t loc = tree_loc_from_asg(asg);
      // Use atomic compare and swap to update the value
      float_t old_value = tree[loc];
      float_t new_value = value;
      while(!atomic_compare_and_swap(tree[loc], old_value, new_value)){
        old_value = tree[loc];
      }
      if(old_value == 0 && new_value > 0) {
        num_support.inc();
      }
      propagate_change(loc);
      // Update support count
      if(old_value > 0 && new_value == 0) {
        num_support.dec();
      }
    } // end of set

    //! Set a leaf value
    void add(size_t asg, float_t value) {
      assert(asg < num_asg);
      assert(value >= 0);
      size_t loc = tree_loc_from_asg(asg);
      // Use atomic compare and swap to update the value
      float_t old_value = tree[loc];
      float_t new_value = value + old_value;
      while(!atomic_compare_and_swap(tree[loc], old_value, new_value)){
        old_value = tree[loc];
        new_value = value + old_value;
      }
      // Update support count
      if(old_value == 0 && new_value > 0) {
        num_support.inc();
      }
      propagate_change(loc);
    } // end of add

    //! Set a leaf value
    void max(size_t asg, float_t value) {
      assert(asg < num_asg);
      assert(value >= 0);
      size_t loc = tree_loc_from_asg(asg);
      // Use atomic compare and swap to update the value
      float_t old_value = tree[loc];
      float_t new_value = std::max(value, old_value);
      while(!atomic_compare_and_swap(tree[loc], old_value, new_value)){
        old_value = tree[loc];
        new_value = std::max(value, old_value);
      }
      if(old_value == 0 && new_value > 0) {
        num_support.inc();
      }
      propagate_change(loc);
      // Update support count
      if(old_value > 0 && new_value == 0) {
        num_support.dec();
      }
    } // end of set

    /**
     * Try to draw a sample from the multinomial.  If everything has
     * probability zero then return false.
     */
    bool sample(size_t& ret_asg, size_t cpuid) {
      // While there is positive support for some assignment
      // size_t spin_count = 0;
      volatile float_t *root = &(tree[0]);
      while(num_support.value > 0 || (*root) > 0) {
        // Try and get a sample
        if(try_sample(ret_asg, cpuid)) {
          assert(ret_asg < num_asg);
          return true;
        }

        //         if(++spin_count % 10000 == 0) {
        //           std::cerr // << THREAD_ID() << ": "
        //                     << "  Sample: " << spin_count
        //                     << ", " << tree[0]
        //                     << ", " << num_support.value
        //                     << std::endl;
        //           float_t sum = 0;
        //           for(size_t i = first_leaf_index; i < tree.size(); ++i) {
        //             sum += tree[i];
        //           }
        //           std::cerr << "Tree Sum: " << sum << std::endl;
        //           std::getchar();
        //         }

      }  // End of While loop

      //       if(spin_count >= 10){
      //         std::cerr // << THREAD_ID() << ": "
      //                   << "  Sample_recover: " << spin_count << std::endl;
      //       }

      return false;
    } // end of sample


    /**
     * Try to draw a sample from the multinomial and zero out the
     * probability of the element that was drawn.  If everything has
     * probability zero then return false.
     */
    bool pop(size_t& ret_asg, size_t cpuid) {
      if(tree.empty()) return false;
      // While there is positive support for some assignment
      while(num_support.value > 0 || tree[0] > 0) {
        // Try and get a sample
        if(try_sample(ret_asg, cpuid)) {
          assert(ret_asg < num_asg);
          // We have a sample but it is possible that another thread
          // also go this sample so we will use CAS to see who "wins"
          // and gets to keep the sample and who has to try again
          size_t loc = tree_loc_from_asg(ret_asg);
          // Use CAS
          float_t old_value = tree[loc];
          float_t new_value = 0;
          while(!atomic_compare_and_swap(tree[loc], old_value, new_value)){
            old_value = tree[loc];
          }
          // Figure out if we won and get to keep the sample or if
          // some other thread won and zeroed out the sample before we
          // got it.
          if(old_value > 0) {
            // We win!!! and keep the sample :-)
            propagate_change(loc);
            num_support.dec();
            return true;
          }
          // The other thread wins and we have to try agian :-(.
        }
      }

      std::cerr << "Queue emptied!: " << tree[0]
                << ", " << num_support.value << std::endl;
      print_tree();
      return false;
    } // end of pop

    /** Get the number of assignments with positive support */
    size_t positive_support() {
      return num_support.value;
    }

    /** print the tree */
    void print_tree() {
      for (size_t i = 0; i < std::min(tree.size(), size_t(1000)); ++i) {
        if(is_leaf(i)) {
          std::cout << "Leaf(" << asg_from_tree_loc(i)
                    << ", [" << parent(i) << "], "
                    << tree[i] << ") ";
        } else {
          std::cout << "Node(" << i <<  ", "
                    << "[" << left_child(i) << ", "
                    << right_child(i) << "], "
                    << tree[i] << ") ";
        }
      }
      std::cout << std::endl;
    }

    float_t get_weight(size_t asg) const {
      size_t loc = tree_loc_from_asg(asg);
      return tree[loc];
    }

    bool has_support(size_t asg) const {
      size_t loc = tree_loc_from_asg(asg);
      return tree[loc] > 0;
    }

    void clear() {
      // not thread safe
      std::fill(tree.begin(), tree.end(), 0.0);
      num_support.value = 0;
    }
  }; // end of fast_multinomial

} // end of namespace

#undef float_t
#endif
