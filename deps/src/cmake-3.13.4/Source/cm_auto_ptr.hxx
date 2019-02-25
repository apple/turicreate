/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef CM_AUTO_PTR_HXX
#define CM_AUTO_PTR_HXX

#include "cmConfigure.h"

#ifdef CMake_HAVE_CXX_AUTO_PTR

#include <memory>
#define CM_AUTO_PTR std::auto_ptr

#else

#define CM_AUTO_PTR cm::auto_ptr

// The HP compiler cannot handle the conversions necessary to use
// auto_ptr_ref to pass an auto_ptr returned from one function
// directly to another function as in use_auto_ptr(get_auto_ptr()).
// We instead use const_cast to achieve the syntax on those platforms.
// We do not use const_cast on other platforms to maintain the C++
// standard design and guarantee that if an auto_ptr is bound
// to a reference-to-const then ownership will be maintained.
#if defined(__HP_aCC)
#define cm_AUTO_PTR_REF 0
#define cm_AUTO_PTR_CONST const
#define cm_AUTO_PTR_CAST(a) cast(a)
#else
#define cm_AUTO_PTR_REF 1
#define cm_AUTO_PTR_CONST
#define cm_AUTO_PTR_CAST(a) a
#endif

// In C++11, clang will warn about using dynamic exception specifications
// as they are deprecated.  But as this class is trying to faithfully
// mimic std::auto_ptr, we want to keep the 'throw()' decorations below.
// So we suppress the warning.
#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wdeprecated")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#endif
#endif

namespace cm {

template <class X>
class auto_ptr;

#if cm_AUTO_PTR_REF
namespace detail {
// The auto_ptr_ref template is supposed to be a private member of
// auto_ptr but Borland 5.8 cannot handle it.  Instead put it in
// a private namespace.
template <class Y>
struct auto_ptr_ref
{
  Y* p_;

  // The extra constructor argument prevents implicit conversion to
  // auto_ptr_ref from auto_ptr through the constructor.  Normally
  // this should be done with the explicit keyword but Borland 5.x
  // generates code in the conversion operator to call itself
  // infinately.
  auto_ptr_ref(Y* p, int)
    : p_(p)
  {
  }
};
}
#endif

/** C++98 Standard Section 20.4.5 - Template class auto_ptr.  */
template <class X>
class auto_ptr
{
#if !cm_AUTO_PTR_REF
  template <typename Y>
  static inline auto_ptr<Y>& cast(auto_ptr<Y> const& a)
  {
    return const_cast<auto_ptr<Y>&>(a);
  }
#endif

  /** The pointer to the object held.  */
  X* x_;

public:
  /** The type of object held by the auto_ptr.  */
  typedef X element_type;

  /** Construct from an auto_ptr holding a compatible object.  This
      transfers ownership to the newly constructed auto_ptr.  */
  template <class Y>
  auto_ptr(auto_ptr<Y> cm_AUTO_PTR_CONST& a) throw()
    : x_(cm_AUTO_PTR_CAST(a).release())
  {
  }

  /** Assign from an auto_ptr holding a compatible object.  This
      transfers ownership to the left-hand-side of the assignment.  */
  template <class Y>
  auto_ptr& operator=(auto_ptr<Y> cm_AUTO_PTR_CONST& a) throw() // NOLINT
  {
    this->reset(cm_AUTO_PTR_CAST(a).release());
    return *this; // NOLINT
  }

  /**
   * Explicitly construct from a raw pointer.  This is typically
   * called with the result of operator new.  For example:
   *
   *   auto_ptr<X> ptr(new X());
   */
  explicit auto_ptr(X* p = CM_NULLPTR) throw()
    : x_(p)
  {
  }

  /** Construct from another auto_ptr holding an object of the same
      type.  This transfers ownership to the newly constructed
      auto_ptr.  */
  auto_ptr(auto_ptr cm_AUTO_PTR_CONST& a) throw()
    : x_(cm_AUTO_PTR_CAST(a).release())
  {
  }

  /** Assign from another auto_ptr holding an object of the same type.
      This transfers ownership to the newly constructed auto_ptr.  */
  auto_ptr& operator=(auto_ptr cm_AUTO_PTR_CONST& a) throw() // NOLINT
  {
    this->reset(cm_AUTO_PTR_CAST(a).release());
    return *this; // NOLINT
  }

  /** Destruct and delete the object held.  */
  ~auto_ptr() throw()
  {
    // Assume object destructor is nothrow.
    delete this->x_;
  }

  /** Dereference and return a reference to the object held.  */
  X& operator*() const throw() { return *this->x_; }

  /** Return a pointer to the object held.  */
  X* operator->() const throw() { return this->x_; }

  /** Return a pointer to the object held.  */
  X* get() const throw() { return this->x_; }

  /** Return a pointer to the object held and reset to hold no object.
      This transfers ownership to the caller.  */
  X* release() throw()
  {
    X* x = this->x_;
    this->x_ = CM_NULLPTR;
    return x;
  }

  /** Assume ownership of the given object.  The object previously
      held is deleted.  */
  void reset(X* p = 0) throw()
  {
    if (this->x_ != p) {
      // Assume object destructor is nothrow.
      delete this->x_;
      this->x_ = p;
    }
  }

  /** Convert to an auto_ptr holding an object of a compatible type.
      This transfers ownership to the returned auto_ptr.  */
  template <class Y>
  operator auto_ptr<Y>() throw()
  {
    return auto_ptr<Y>(this->release());
  }

#if cm_AUTO_PTR_REF
  /** Construct from an auto_ptr_ref.  This is used when the
      constructor argument is a call to a function returning an
      auto_ptr.  */
  auto_ptr(detail::auto_ptr_ref<X> r) throw()
    : x_(r.p_)
  {
  }

  /** Assign from an auto_ptr_ref.  This is used when a function
      returning an auto_ptr is passed on the right-hand-side of an
      assignment.  */
  auto_ptr& operator=(detail::auto_ptr_ref<X> r) throw()
  {
    this->reset(r.p_);
    return *this; // NOLINT
  }

  /** Convert to an auto_ptr_ref.  This is used when a function
      returning an auto_ptr is the argument to the constructor of
      another auto_ptr.  */
  template <class Y>
  operator detail::auto_ptr_ref<Y>() throw()
  {
    return detail::auto_ptr_ref<Y>(this->release(), 1);
  }
#endif
};

} // namespace cm

// Undo warning suppression.
#if defined(__clang__) && defined(__has_warning)
#if __has_warning("-Wdeprecated")
#pragma clang diagnostic pop
#endif
#endif

#endif

#endif
