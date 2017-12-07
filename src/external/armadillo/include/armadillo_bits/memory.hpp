// Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2008-2016 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------


//! \addtogroup memory
//! @{


class memory
  {
  public:

  arma_inline static uword enlarge_to_mult_of_chunksize(const uword n_elem);

  template<typename eT> inline arma_malloc static eT*         acquire(const uword n_elem);
  template<typename eT> inline arma_malloc static eT* acquire_chunked(const uword n_elem);

  template<typename eT> arma_inline static void release(eT* mem);

  template<typename eT> arma_inline static bool      is_aligned(const eT*  mem);
  template<typename eT> arma_inline static void mark_as_aligned(      eT*& mem);
  template<typename eT> arma_inline static void mark_as_aligned(const eT*& mem);
  };



arma_inline
uword
memory::enlarge_to_mult_of_chunksize(const uword n_elem)
  {
  const uword chunksize = arma_config::spmat_chunksize;

  // this relies on integer division
  const uword n_elem_mod = (n_elem > 0) ? (((n_elem-1) / chunksize) + 1) * chunksize : uword(0);

  return n_elem_mod;
  }



template<typename eT>
inline
arma_malloc
eT*
memory::acquire(const uword n_elem)
  {
  arma_debug_check
    (
    ( size_t(n_elem) > (std::numeric_limits<size_t>::max() / sizeof(eT)) ),
    "arma::memory::acquire(): requested size is too large"
    );

  eT* out_memptr;

  #if   defined(ARMA_USE_TBB_ALLOC)
    {
    out_memptr = (eT *) scalable_malloc(sizeof(eT)*n_elem);
    }
  #elif defined(ARMA_USE_MKL_ALLOC)
    {
    out_memptr = (eT *) mkl_malloc( sizeof(eT)*n_elem, 128 );
    }
  #elif defined(ARMA_HAVE_POSIX_MEMALIGN)
    {
    eT* memptr;

    const size_t alignment = 16;  // change the 16 to 64 if you wish to align to the cache line

    int status = posix_memalign((void **)&memptr, ( (alignment >= sizeof(void*)) ? alignment : sizeof(void*) ), sizeof(eT)*n_elem);

    out_memptr = (status == 0) ? memptr : NULL;
    }
  #elif defined(_MSC_VER)
    {
    //out_memptr = (eT *) malloc(sizeof(eT)*n_elem);
    out_memptr = (eT *) _aligned_malloc( sizeof(eT)*n_elem, 16 );  // lives in malloc.h
    }
  #else
    {
    //return ( new(std::nothrow) eT[n_elem] );
    out_memptr = (eT *) malloc(sizeof(eT)*n_elem);
    }
  #endif

  // TODO: for mingw, use __mingw_aligned_malloc

  if(n_elem > 0)
    {
    arma_check_bad_alloc( (out_memptr == NULL), "arma::memory::acquire(): out of memory" );
    }

  return out_memptr;
  }



//! get memory in multiples of chunks, holding at least n_elem
template<typename eT>
inline
arma_malloc
eT*
memory::acquire_chunked(const uword n_elem)
  {
  const uword n_elem_mod = memory::enlarge_to_mult_of_chunksize(n_elem);

  return memory::acquire<eT>(n_elem_mod);
  }



template<typename eT>
arma_inline
void
memory::release(eT* mem)
  {
  #if   defined(ARMA_USE_TBB_ALLOC)
    {
    scalable_free( (void *)(mem) );
    }
  #elif defined(ARMA_USE_MKL_ALLOC)
    {
    mkl_free( (void *)(mem) );
    }
  #elif defined(ARMA_HAVE_POSIX_MEMALIGN)
    {
    free( (void *)(mem) );
    }
  #elif defined(_MSC_VER)
    {
    //free( (void *)(mem) );
    _aligned_free( (void *)(mem) );
    }
  #else
    {
    //delete [] mem;
    free( (void *)(mem) );
    }
  #endif

  // TODO: for mingw, use __mingw_aligned_free
  }



template<typename eT>
arma_inline
bool
memory::is_aligned(const eT* mem)
  {
  #if (defined(ARMA_HAVE_ICC_ASSUME_ALIGNED) || defined(ARMA_HAVE_GCC_ASSUME_ALIGNED)) && !defined(ARMA_DONT_CHECK_ALIGNMENT)
    {
    return (sizeof(std::size_t) >= sizeof(eT*)) ? ((std::size_t(mem) & 0x0F) == 0) : false;
    }
  #else
    {
    arma_ignore(mem);

    return false;
    }
  #endif
  }



template<typename eT>
arma_inline
void
memory::mark_as_aligned(eT*& mem)
  {
  #if defined(ARMA_HAVE_ICC_ASSUME_ALIGNED)
    {
    __assume_aligned(mem, 16);
    }
  #elif defined(ARMA_HAVE_GCC_ASSUME_ALIGNED)
    {
    mem = (eT*)__builtin_assume_aligned(mem, 16);
    }
  #else
    {
    arma_ignore(mem);
    }
  #endif

  // TODO: MSVC?  __assume( (mem & 0x0F) == 0 );
  //
  // http://comments.gmane.org/gmane.comp.gcc.patches/239430
  // GCC __builtin_assume_aligned is similar to ICC's __assume_aligned,
  // so for lvalue first argument ICC's __assume_aligned can be emulated using
  // #define __assume_aligned(lvalueptr, align) lvalueptr = __builtin_assume_aligned (lvalueptr, align)
  //
  // http://www.inf.ethz.ch/personal/markusp/teaching/263-2300-ETH-spring11/slides/class19.pdf
  // http://software.intel.com/sites/products/documentation/hpc/composerxe/en-us/cpp/lin/index.htm
  // http://d3f8ykwhia686p.cloudfront.net/1live/intel/CompilerAutovectorizationGuide.pdf
  }



template<typename eT>
arma_inline
void
memory::mark_as_aligned(const eT*& mem)
  {
  #if defined(ARMA_HAVE_ICC_ASSUME_ALIGNED)
    {
    __assume_aligned(mem, 16);
    }
  #elif defined(ARMA_HAVE_GCC_ASSUME_ALIGNED)
    {
    mem = (const eT*)__builtin_assume_aligned(mem, 16);
    }
  #else
    {
    arma_ignore(mem);
    }
  #endif
  }



//! @}
