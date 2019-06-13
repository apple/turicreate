/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BLOB_HPP
#define TURI_BLOB_HPP

#include <cstring>
#include <cstdlib>
#include <cassert>

#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/vector.hpp>
#include <core/storage/serialization/map.hpp>

namespace turi {
  /**
   * blob is the general representation of a "block" of information.
   *. 'data' must be exactly 'length' bytes and must be entirely
   * self contained.  It must not hold references to other memory
   * regions.  That is to say, I should be able read off exactly
   * 'length' bytes from 'data', and send it across a network/write it
   * to a disk/etc, and the information should still be consistent
   * The blob is self-managed and will free and delete the underlying memory
   * when it goes out of scope.
   */
  class blob {
        size_t size_;    /// number of bytes of the 'data' field
        void* data_;     /// user information
  public:

    /** Create an empty blob */
    blob() : size_(0), data_(NULL) { }

    /** Create simple blob of a certain size (with allocation)*/
    blob(size_t new_size) :
      size_(0), data_(NULL) {
      resize(new_size);
    } // end of basic blob constructor

    /** Makes a copy of the ptr provided */
    blob(size_t osize, void* odata) : size_(0), data_(NULL) {
      if (osize > 0) { copy(osize, odata);  }
    } // end of basic blob constructor

    /** Copy constructor */
    blob(const blob &b) : size_(0), data_(NULL) {
      if (b.size_ != 0 && b.data_ != NULL) {
        copy(b);
      }
    }

    ~blob() { clear();  }

    /** Smart Casting */
    template<typename T>
    T& as() {
      assert(data_ != NULL);
      assert(sizeof(T) <= size_);
      return *reinterpret_cast<T*>(data_);
    }

    /** Smart Casting */
    template<typename T>
    const T& as() const {
      assert(data_ != NULL);
      assert(sizeof(T) <= size_);
      return *reinterpret_cast<T*>(data_);
    }

    /** Smart Casting */
    template<typename T>
    T* as_ptr() {
      assert(data_ != NULL);
      assert(sizeof(T) <= size_);
      return reinterpret_cast<T*>(data_);
    }

    /** Smart Casting */
    template<typename T>
    const T* as_ptr() const {
      assert(data_ != NULL);
      assert(sizeof(T) <= size_);
      return reinterpret_cast<const T*>(data_);
    }

    /** Get the size of the blob */
    size_t size() const { return size_; }

    /** Get the size of the blob */
    void* data() { return data_; }

    /** Get the size of the blob */
    const void* data() const { return data_; }



    blob& operator=(const blob& b){
      copy(b);
      return *this;
    }



    /** make a copy of the data passed in as arguments. */
    void copy(size_t osize, void* odata) {
      resize(osize);
      // Copy the contents over
      memcpy(data_, odata, osize);
    }

    /** Make "deep" copy of the blob by replicate its binary data */
    void copy(const blob& other) {
      assert(other.size_ == 0 || other.data_ != NULL);
      // Do an allocation (which is only done if necessary)
      resize(other.size_);
      // Copy the contents over
      memcpy(data_, other.data_, size_);
    }

    /** deprecated. Just use operator= */
    blob copy() const{
      return *this;
    }
    /** Resize the blob to any size including 0 */
    void resize(size_t new_size) {
      if(new_size == 0) {
        // if resize to zero then just clear
        clear();
      } else if(size_ == new_size ) {
        // if resize to current size then nop
        assert(data_ != NULL);
      } else {
        clear();
        assert(data_ == NULL && size_ == 0);
        size_ = new_size;
        data_ = malloc(new_size);
        assert(data_ != NULL);
      }
    } // end of malloc


    /** free the memory associated with this blob */
    void clear() {
      if(data_ != NULL) {
        assert(size_ > 0);
        free(data_);
        data_ = NULL;
      }
      size_ = 0;
    } // end of free


    /** Swaps the contents of two blobs. A "safe" version of a shallow copy */
    void swap(blob &b) {
      void* tmp = b.data_;
      size_t tmpsize = b.size_;

      b.data_ = data_;
      b.size_ = size_;

      data_ = tmp;
      size_ = tmpsize;
    }

    void load(iarchive& arc) {
      clear();
      arc >> size_;
      if (size_ == 0) {
        data_ = NULL;
      } else {
        data_ = malloc(size_);
        deserialize(arc, data_, size_);
      }
    }

    void save(oarchive& arc) const {
      arc << size_;
      if (size_ != 0) {
        serialize(arc, data_, size_);
      }
    }
  }; // end of blob


} // end of namespace
#endif
