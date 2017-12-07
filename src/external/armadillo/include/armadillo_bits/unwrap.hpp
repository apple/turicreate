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


//! \addtogroup unwrap
//! @{



template<typename T1>
struct unwrap_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  unwrap_default(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT> M;
  };



template<typename T1>
struct unwrap_fixed
  {
  typedef T1 stored_type;

  inline explicit
  unwrap_fixed(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const T1& M;
  };



template<typename T1, bool condition>
struct unwrap_redirect {};

template<typename T1>
struct unwrap_redirect<T1, false> { typedef unwrap_default<T1> result; };

template<typename T1>
struct unwrap_redirect<T1, true>  { typedef unwrap_fixed<T1>   result; };


template<typename T1>
struct unwrap : public unwrap_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  inline
  unwrap(const T1& A)
    : unwrap_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct unwrap< Mat<eT> >
  {
  typedef Mat<eT> stored_type;

  inline
  unwrap(const Mat<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT>& M;
  };



template<typename eT>
struct unwrap< Row<eT> >
  {
  typedef Row<eT> stored_type;

  inline
  unwrap(const Row<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Row<eT>& M;
  };



template<typename eT>
struct unwrap< Col<eT> >
  {
  typedef Col<eT> stored_type;

  inline
  unwrap(const Col<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Col<eT>& M;
  };



template<typename out_eT, typename T1, typename T2, typename glue_type>
struct unwrap< mtGlue<out_eT, T1, T2, glue_type> >
  {
  typedef Mat<out_eT> stored_type;

  inline
  unwrap(const mtGlue<out_eT, T1, T2, glue_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<out_eT> M;
  };



template<typename out_eT, typename T1, typename op_type>
struct unwrap< mtOp<out_eT, T1, op_type> >
  {
  typedef Mat<out_eT> stored_type;

  inline
  unwrap(const mtOp<out_eT, T1, op_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<out_eT> M;
  };



//
//
//



template<typename T1>
struct quasi_unwrap_default
  {
  typedef typename T1::elem_type eT;

  static const bool has_subview = false;

  inline
  quasi_unwrap_default(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  // NOTE: DO NOT DIRECTLY CHECK FOR ALIASING BY TAKING THE ADDRESS OF THE "M" OBJECT IN ANY quasi_unwrap CLASS !!!
  const Mat<eT> M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>&) const { return false; }
  };



template<typename T1>
struct quasi_unwrap_fixed
  {
  typedef typename T1::elem_type eT;

  static const bool has_subview = false;

  inline explicit
  quasi_unwrap_fixed(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const T1& M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&M) == void_ptr(&X)); }
  };



template<typename T1, bool condition>
struct quasi_unwrap_redirect {};

template<typename T1>
struct quasi_unwrap_redirect<T1, false> { typedef quasi_unwrap_default<T1> result; };

template<typename T1>
struct quasi_unwrap_redirect<T1, true>  { typedef quasi_unwrap_fixed<T1>   result; };


template<typename T1>
struct quasi_unwrap : public quasi_unwrap_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename quasi_unwrap_redirect<T1, is_Mat_fixed<T1>::value >::result quasi_unwrap_extra;

  static const bool has_subview = quasi_unwrap_extra::has_subview;

  inline
  quasi_unwrap(const T1& A)
    : quasi_unwrap_extra(A)
    {
    }

  using quasi_unwrap_extra::M;
  using quasi_unwrap_extra::is_alias;
  };



template<typename eT>
struct quasi_unwrap< Mat<eT> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const Mat<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT>& M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&M) == void_ptr(&X)); }
  };



template<typename eT>
struct quasi_unwrap< Row<eT> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const Row<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Row<eT>& M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&M) == void_ptr(&X)); }
  };



template<typename eT>
struct quasi_unwrap< Col<eT> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const Col<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Col<eT>& M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&M) == void_ptr(&X)); }
  };



template<typename eT>
struct quasi_unwrap< subview<eT> >
  {
  static const bool has_subview = true;

  inline
  quasi_unwrap(const subview<eT>& A)
    : sv( A                  )
    , M ( A, (A.n_cols == 1) )  // reuse memory if the subview has only one column
    {
    arma_extra_debug_sigprint();
    }

  const subview<eT>& sv;
  const Mat<eT>      M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return ( (sv.n_cols == 1) ? (void_ptr(&(sv.m)) == void_ptr(&X)) : false ); }
  };



template<typename eT>
struct quasi_unwrap< subview_row<eT> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const subview_row<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Row<eT> M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>&) const { return false; }
  };



template<typename eT>
struct quasi_unwrap< subview_col<eT> >
  {
  static const bool has_subview = true;

  inline
  quasi_unwrap(const subview_col<eT>& A)
    : orig( A.m )
    , M  ( const_cast<eT*>( A.colptr(0) ), A.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT>& orig;
  const Col<eT>  M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&orig) == void_ptr(&X)); }
  };



template<typename out_eT, typename T1, typename T2, typename glue_type>
struct quasi_unwrap< mtGlue<out_eT, T1, T2, glue_type> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const mtGlue<out_eT, T1, T2, glue_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<out_eT> M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>&) const { return false; }
  };



template<typename out_eT, typename T1, typename op_type>
struct quasi_unwrap< mtOp<out_eT, T1, op_type> >
  {
  static const bool has_subview = false;

  inline
  quasi_unwrap(const mtOp<out_eT, T1, op_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<out_eT> M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>&) const { return false; }
  };



template<typename T1>
struct quasi_unwrap< Op<T1, op_vectorise_col> >
  {
  static const bool has_subview = true;

  typedef typename T1::elem_type eT;

  inline
  quasi_unwrap(const Op<T1, op_vectorise_col>& A)
    : U( A.m )
    , M( const_cast<eT*>(U.M.memptr()), U.M.n_elem, 1, false, false )
    {
    arma_extra_debug_sigprint();
    }

  const unwrap<T1> U;
  const Mat<eT>    M;

  template<typename eT2>
  arma_inline bool is_alias(const Mat<eT2>& X) const { return (void_ptr(&(U.M)) == void_ptr(&X)); }
  };



//
//
//



template<typename T1>
struct unwrap_check_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  unwrap_check_default(const T1& A, const Mat<eT>&)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  inline
  unwrap_check_default(const T1& A, const bool)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT> M;
  };



template<typename T1>
struct unwrap_check_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline
  unwrap_check_fixed(const T1& A, const Mat<eT>& B)
    : M_local( (&A == &B) ? new T1(A) : 0 )
    , M      ( (&A == &B) ? *M_local  : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  unwrap_check_fixed(const T1& A, const bool is_alias)
    : M_local( is_alias ? new T1(A) : 0 )
    , M      ( is_alias ? *M_local  : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct unwrap_check_redirect {};

template<typename T1>
struct unwrap_check_redirect<T1, false> { typedef unwrap_check_default<T1> result; };

template<typename T1>
struct unwrap_check_redirect<T1, true>  { typedef unwrap_check_fixed<T1>   result; };


template<typename T1>
struct unwrap_check : public unwrap_check_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  inline unwrap_check(const T1& A, const Mat<typename T1::elem_type>& B)
    : unwrap_check_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }

  inline unwrap_check(const T1& A, const bool is_alias)
    : unwrap_check_redirect< T1, is_Mat_fixed<T1>::value >::result(A, is_alias)
    {
    }
  };



template<typename eT>
struct unwrap_check< Mat<eT> >
  {
  typedef Mat<eT> stored_type;

  inline
  unwrap_check(const Mat<eT>& A, const Mat<eT>& B)
    : M_local( (&A == &B) ? new Mat<eT>(A) : 0 )
    , M      ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  unwrap_check(const Mat<eT>& A, const bool is_alias)
    : M_local( is_alias ? new Mat<eT>(A) : 0 )
    , M      ( is_alias ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct unwrap_check< Row<eT> >
  {
  typedef Row<eT> stored_type;

  inline
  unwrap_check(const Row<eT>& A, const Mat<eT>& B)
    : M_local( (&A == &B) ? new Row<eT>(A) : 0 )
    , M      ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  unwrap_check(const Row<eT>& A, const bool is_alias)
    : M_local( is_alias ? new Row<eT>(A) : 0 )
    , M      ( is_alias ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct unwrap_check< Col<eT> >
  {
  typedef Col<eT> stored_type;

  inline
  unwrap_check(const Col<eT>& A, const Mat<eT>& B)
    : M_local( (&A == &B) ? new Col<eT>(A) : 0 )
    , M      ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  unwrap_check(const Col<eT>& A, const bool is_alias)
    : M_local( is_alias ? new Col<eT>(A) : 0 )
    , M      ( is_alias ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Col<eT>* M_local;
  const Col<eT>& M;
  };



//
//
//



template<typename T1>
struct unwrap_check_mixed
  {
  typedef typename T1::elem_type eT1;

  template<typename eT2>
  inline
  unwrap_check_mixed(const T1& A, const Mat<eT2>&)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  //template<typename eT2>
  inline
  unwrap_check_mixed(const T1& A, const bool)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const Mat<eT1> M;
  };



template<typename eT1>
struct unwrap_check_mixed< Mat<eT1> >
  {
  template<typename eT2>
  inline
  unwrap_check_mixed(const Mat<eT1>& A, const Mat<eT2>& B)
    : M_local( (void_ptr(&A) == void_ptr(&B)) ? new Mat<eT1>(A) : 0 )
    , M      ( (void_ptr(&A) == void_ptr(&B)) ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }

  //template<typename eT2>
  inline
  unwrap_check_mixed(const Mat<eT1>& A, const bool is_alias)
    : M_local( is_alias ? new Mat<eT1>(A) : 0 )
    , M      ( is_alias ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check_mixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Mat<eT1>* M_local;
  const Mat<eT1>& M;
  };



template<typename eT1>
struct unwrap_check_mixed< Row<eT1> >
  {
  template<typename eT2>
  inline
  unwrap_check_mixed(const Row<eT1>& A, const Mat<eT2>& B)
    : M_local( (void_ptr(&A) == void_ptr(&B)) ? new Row<eT1>(A) : 0 )
    , M      ( (void_ptr(&A) == void_ptr(&B)) ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }


  //template<typename eT2>
  inline
  unwrap_check_mixed(const Row<eT1>& A, const bool is_alias)
    : M_local( is_alias ? new Row<eT1>(A) : 0 )
    , M      ( is_alias ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check_mixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Row<eT1>* M_local;
  const Row<eT1>& M;
  };



template<typename eT1>
struct unwrap_check_mixed< Col<eT1> >
  {
  template<typename eT2>
  inline
  unwrap_check_mixed(const Col<eT1>& A, const Mat<eT2>& B)
    : M_local( (void_ptr(&A) == void_ptr(&B)) ? new Col<eT1>(A) : 0 )
    , M      ( (void_ptr(&A) == void_ptr(&B)) ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }

  //template<typename eT2>
  inline
  unwrap_check_mixed(const Col<eT1>& A, const bool is_alias)
    : M_local( is_alias ? new Col<eT1>(A) : 0 )
    , M      ( is_alias ? (*M_local)      : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~unwrap_check_mixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }


  // the order below is important
  const Col<eT1>* M_local;
  const Col<eT1>& M;
  };



//
//
//



template<typename T1>
struct partial_unwrap_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_default(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Mat<eT> M;
  };


template<typename T1>
struct partial_unwrap_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_fixed(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_redirect {};

template<typename T1>
struct partial_unwrap_redirect<T1, false> { typedef partial_unwrap_default<T1> result; };

template<typename T1>
struct partial_unwrap_redirect<T1, true>  { typedef partial_unwrap_fixed<T1>   result; };

template<typename T1>
struct partial_unwrap : public partial_unwrap_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  inline
  partial_unwrap(const T1& A)
    : partial_unwrap_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct partial_unwrap< Mat<eT> >
  {
  typedef Mat<eT> stored_type;

  inline
  partial_unwrap(const Mat<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return ((&X) == (&M)); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Row<eT> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const Row<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Col<eT> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const Col<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Col<eT>& M;
  };



template<typename eT>
struct partial_unwrap< subview_col<eT> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const subview_col<eT>& A)
    : orig( A.m )
    , M   ( const_cast<eT*>( A.colptr(0) ), A.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&orig)); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Mat<eT>& orig;
  const Col<eT>  M;
  };



template<typename eT>
struct partial_unwrap< subview_row<eT> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const subview_row<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Row<eT> M;
  };



template<typename T1>
struct partial_unwrap_htrans_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_htrans_default(const Op<T1, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Mat<eT> M;
  };


template<typename T1>
struct partial_unwrap_htrans_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_htrans_fixed(const Op<T1, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_htrans_redirect {};

template<typename T1>
struct partial_unwrap_htrans_redirect<T1, false> { typedef partial_unwrap_htrans_default<T1> result; };

template<typename T1>
struct partial_unwrap_htrans_redirect<T1, true>  { typedef partial_unwrap_htrans_fixed<T1>   result; };

template<typename T1>
struct partial_unwrap< Op<T1, op_htrans> > : public partial_unwrap_htrans_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  inline partial_unwrap(const Op<T1, op_htrans>& A)
    : partial_unwrap_htrans_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct partial_unwrap< Op< Mat<eT>, op_htrans> >
  {
  typedef Mat<eT> stored_type;

  inline
  partial_unwrap(const Op< Mat<eT>, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< Row<eT>, op_htrans> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const Op< Row<eT>, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< Col<eT>, op_htrans> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const Op< Col<eT>, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Col<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< subview_col<eT>, op_htrans> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const Op< subview_col<eT>, op_htrans>& A)
    : orig( A.m.m )
    , M   ( const_cast<eT*>( A.m.colptr(0) ), A.m.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&orig)); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Mat<eT>& orig;
  const Col<eT>  M;
  };



template<typename eT>
struct partial_unwrap< Op< subview_row<eT>, op_htrans> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const Op< subview_row<eT>, op_htrans>& A)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Row<eT> M;
  };



template<typename T1>
struct partial_unwrap_htrans2_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_htrans2_default(const Op<T1, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return val; }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT      val;
  const Mat<eT> M;
  };


template<typename T1>
struct partial_unwrap_htrans2_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_htrans2_fixed(const Op<T1, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT  val;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_htrans2_redirect {};

template<typename T1>
struct partial_unwrap_htrans2_redirect<T1, false> { typedef partial_unwrap_htrans2_default<T1> result; };

template<typename T1>
struct partial_unwrap_htrans2_redirect<T1, true>  { typedef partial_unwrap_htrans2_fixed<T1>   result; };

template<typename T1>
struct partial_unwrap< Op<T1, op_htrans2> > : public partial_unwrap_htrans2_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  inline partial_unwrap(const Op<T1, op_htrans2>& A)
    : partial_unwrap_htrans2_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct partial_unwrap< Op< Mat<eT>, op_htrans2> >
  {
  typedef Mat<eT> stored_type;

  inline
  partial_unwrap(const Op< Mat<eT>, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT       val;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< Row<eT>, op_htrans2> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const Op< Row<eT>, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT       val;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< Col<eT>, op_htrans2> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const Op< Col<eT>, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT       val;
  const Col<eT>& M;
  };



template<typename eT>
struct partial_unwrap< Op< subview_col<eT>, op_htrans2> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const Op< subview_col<eT>, op_htrans2>& A)
    : orig( A.m.m )
    , val ( A.aux )
    , M   ( const_cast<eT*>( A.m.colptr(0) ), A.m.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&orig)); }

  static const bool do_trans = true;
  static const bool do_times = true;

  const Mat<eT>& orig;

  const eT      val;
  const Col<eT> M;
  };



template<typename eT>
struct partial_unwrap< Op< subview_row<eT>, op_htrans2> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const Op< subview_row<eT>, op_htrans2>& A)
    : val(A.aux)
    , M  (A.m  )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return val; }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT      val;
  const Row<eT> M;
  };



template<typename T1>
struct partial_unwrap_scalar_times_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_scalar_times_default(const eOp<T1, eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return val; }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT      val;
  const Mat<eT> M;
  };



template<typename T1>
struct partial_unwrap_scalar_times_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_scalar_times_fixed(const eOp<T1, eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT  val;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_scalar_times_redirect {};

template<typename T1>
struct partial_unwrap_scalar_times_redirect<T1, false> { typedef partial_unwrap_scalar_times_default<T1> result; };

template<typename T1>
struct partial_unwrap_scalar_times_redirect<T1, true>  { typedef partial_unwrap_scalar_times_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap< eOp<T1, eop_scalar_times> > : public partial_unwrap_scalar_times_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline
  partial_unwrap(const eOp<T1, eop_scalar_times>& A)
    : partial_unwrap_scalar_times_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct partial_unwrap< eOp<Mat<eT>, eop_scalar_times> >
  {
  typedef Mat<eT> stored_type;

  inline
  partial_unwrap(const eOp<Mat<eT>,eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<Row<eT>, eop_scalar_times> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const eOp<Row<eT>,eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<Col<eT>, eop_scalar_times> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const eOp<Col<eT>,eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Col<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<subview_col<eT>, eop_scalar_times> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap(const eOp<subview_col<eT>,eop_scalar_times>& A)
    : orig( A.P.Q.m )
    , val ( A.aux   )
    , M   ( const_cast<eT*>( A.P.Q.colptr(0) ), A.P.Q.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  arma_hot arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&orig)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT>& orig;

  const eT      val;
  const Col<eT> M;
  };



template<typename eT>
struct partial_unwrap< eOp<subview_row<eT>, eop_scalar_times> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap(const eOp<subview_row<eT>,eop_scalar_times>& A)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return val; }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT      val;
  const Row<eT> M;
  };



template<typename T1>
struct partial_unwrap_neg_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_neg_default(const eOp<T1, eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT> M;
  };



template<typename T1>
struct partial_unwrap_neg_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_neg_fixed(const eOp<T1, eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_neg_redirect {};

template<typename T1>
struct partial_unwrap_neg_redirect<T1, false> { typedef partial_unwrap_neg_default<T1> result; };

template<typename T1>
struct partial_unwrap_neg_redirect<T1, true>  { typedef partial_unwrap_neg_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap< eOp<T1, eop_neg> > : public partial_unwrap_neg_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline
  partial_unwrap(const eOp<T1, eop_neg>& A)
    : partial_unwrap_neg_redirect< T1, is_Mat_fixed<T1>::value >::result(A)
    {
    }
  };



template<typename eT>
struct partial_unwrap< eOp<Mat<eT>, eop_neg> >
  {
  typedef Mat<eT> stored_type;

  inline
  partial_unwrap(const eOp<Mat<eT>,eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<Row<eT>, eop_neg> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const eOp<Row<eT>,eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<Col<eT>, eop_neg> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const eOp<Col<eT>,eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&M)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Col<eT>& M;
  };



template<typename eT>
struct partial_unwrap< eOp<subview_col<eT>, eop_neg> >
  {
  typedef Col<eT> stored_type;

  inline
  partial_unwrap(const eOp<subview_col<eT>,eop_neg>& A)
    : orig( A.P.Q.m )
    , M   ( const_cast<eT*>( A.P.Q.colptr(0) ), A.P.Q.n_rows, false, false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>& X) const { return (void_ptr(&X) == void_ptr(&orig)); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT>& orig;
  const Col<eT>  M;
  };



template<typename eT>
struct partial_unwrap< eOp<subview_row<eT>, eop_neg> >
  {
  typedef Row<eT> stored_type;

  inline
  partial_unwrap(const eOp<subview_row<eT>,eop_neg>& A)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  arma_inline bool is_alias(const Mat<eT>&) const { return false; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Row<eT> M;
  };



//



template<typename T1>
struct partial_unwrap_check_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_check_default(const T1& A, const Mat<eT>&)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Mat<eT> M;
  };


template<typename T1>
struct partial_unwrap_check_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_check_fixed(const T1& A, const Mat<eT>& B)
    : M_local( (&A == &B) ? new T1(A)  : 0 )
    , M      ( (&A == &B) ? (*M_local) : A )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_check_redirect {};

template<typename T1>
struct partial_unwrap_check_redirect<T1, false> { typedef partial_unwrap_check_default<T1> result; };

template<typename T1>
struct partial_unwrap_check_redirect<T1, true>  { typedef partial_unwrap_check_fixed<T1>   result; };

template<typename T1>
struct partial_unwrap_check : public partial_unwrap_check_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline partial_unwrap_check(const T1& A, const Mat<eT>& B)
    : partial_unwrap_check_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }
  };



template<typename eT>
struct partial_unwrap_check< Mat<eT> >
  {
  typedef Mat<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Mat<eT>& A, const Mat<eT>& B)
    : M_local ( (&A == &B) ? new Mat<eT>(A) : 0 )
    , M       ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }


  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  // the order below is important
  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Row<eT> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Row<eT>& A, const Mat<eT>& B)
    : M_local ( (&A == &B) ? new Row<eT>(A) : 0 )
    , M       ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }


  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  // the order below is important
  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Col<eT> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Col<eT>& A, const Mat<eT>& B)
    : M_local ( (&A == &B) ? new Col<eT>(A) : 0 )
    , M       ( (&A == &B) ? (*M_local)     : A )
    {
    arma_extra_debug_sigprint();
    }


  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  // the order below is important
  const Col<eT>* M_local;
  const Col<eT>& M;
  };



// NOTE: we can get away with this shortcut as the partial_unwrap_check class is only used by the glue_times class,
// NOTE: which relies on partial_unwrap_check to check for aliasing
template<typename eT>
struct partial_unwrap_check< subview_col<eT> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const subview_col<eT>& A, const Mat<eT>& B)
    : M  ( const_cast<eT*>( A.colptr(0) ), A.n_rows, (&(A.m) == &B), false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = false;
  static const bool do_times = false;

  const Col<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_htrans_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_check_htrans_default(const Op<T1, op_htrans>& A, const Mat<eT>&)
    : M(A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Mat<eT> M;
  };


template<typename T1>
struct partial_unwrap_check_htrans_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_check_htrans_fixed(const Op<T1, op_htrans>& A, const Mat<eT>& B)
    : M_local( (&(A.m) == &B) ? new T1(A.m) : 0   )
    , M      ( (&(A.m) == &B) ? (*M_local)  : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check_htrans_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_check_htrans_redirect {};

template<typename T1>
struct partial_unwrap_check_htrans_redirect<T1, false> { typedef partial_unwrap_check_htrans_default<T1> result; };

template<typename T1>
struct partial_unwrap_check_htrans_redirect<T1, true>  { typedef partial_unwrap_check_htrans_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap_check< Op<T1, op_htrans> > : public partial_unwrap_check_htrans_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline partial_unwrap_check(const Op<T1, op_htrans>& A, const Mat<eT>& B)
    : partial_unwrap_check_htrans_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }
  };



template<typename eT>
struct partial_unwrap_check< Op< Mat<eT>, op_htrans> >
  {
  typedef Mat<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Mat<eT>, op_htrans>& A, const Mat<eT>& B)
    : M_local ( (&A.m == &B) ? new Mat<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  // the order below is important
  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Op< Row<eT>, op_htrans> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Row<eT>, op_htrans>& A, const Mat<eT>& B)
    : M_local ( (&A.m == &B) ? new Row<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  // the order below is important
  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Op< Col<eT>, op_htrans> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Col<eT>, op_htrans>& A, const Mat<eT>& B)
    : M_local ( (&A.m == &B) ? new Col<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  // the order below is important
  const Col<eT>* M_local;
  const Col<eT>& M;
  };



// NOTE: we can get away with this shortcut as the partial_unwrap_check class is only used by the glue_times class,
// NOTE: which relies on partial_unwrap_check to check for aliasing
template<typename eT>
struct partial_unwrap_check< Op< subview_col<eT>, op_htrans> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< subview_col<eT>, op_htrans>& A, const Mat<eT>& B)
    : M  ( const_cast<eT*>( A.m.colptr(0) ), A.m.n_rows, (&(A.m.m) == &B), false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(1); }

  static const bool do_trans = true;
  static const bool do_times = false;

  const Col<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_htrans2_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_check_htrans2_default(const Op<T1, op_htrans2>& A, const Mat<eT>&)
    : val(A.aux)
    , M  (A.m)
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT      val;
  const Mat<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_htrans2_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_check_htrans2_fixed(const Op<T1, op_htrans2>& A, const Mat<eT>& B)
    : val    (A.aux)
    , M_local( (&(A.m) == &B) ? new T1(A.m) : 0   )
    , M      ( (&(A.m) == &B) ? (*M_local)  : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check_htrans2_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT  val;
  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_check_htrans2_redirect {};

template<typename T1>
struct partial_unwrap_check_htrans2_redirect<T1, false> { typedef partial_unwrap_check_htrans2_default<T1> result; };

template<typename T1>
struct partial_unwrap_check_htrans2_redirect<T1, true>  { typedef partial_unwrap_check_htrans2_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap_check< Op<T1, op_htrans2> > : public partial_unwrap_check_htrans2_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline partial_unwrap_check(const Op<T1, op_htrans2>& A, const Mat<eT>& B)
    : partial_unwrap_check_htrans2_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }
  };



template<typename eT>
struct partial_unwrap_check< Op< Mat<eT>, op_htrans2> >
  {
  typedef Mat<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Mat<eT>, op_htrans2>& A, const Mat<eT>& B)
    : val     (A.aux)
    , M_local ( (&A.m == &B) ? new Mat<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  // the order below is important
  const eT       val;
  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Op< Row<eT>, op_htrans2> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Row<eT>, op_htrans2>& A, const Mat<eT>& B)
    : val     (A.aux)
    , M_local ( (&A.m == &B) ? new Row<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  // the order below is important
  const eT       val;
  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< Op< Col<eT>, op_htrans2> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< Col<eT>, op_htrans2>& A, const Mat<eT>& B)
    : val     (A.aux)
    , M_local ( (&A.m == &B) ? new Col<eT>(A.m) : 0   )
    , M       ( (&A.m == &B) ? (*M_local)       : A.m )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  // the order below is important
  const eT       val;
  const Col<eT>* M_local;
  const Col<eT>& M;
  };



// NOTE: we can get away with this shortcut as the partial_unwrap_check class is only used by the glue_times class,
// NOTE: which relies on partial_unwrap_check to check for aliasing
template<typename eT>
struct partial_unwrap_check< Op< subview_col<eT>, op_htrans2> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const Op< subview_col<eT>, op_htrans2>& A, const Mat<eT>& B)
    : val( A.aux )
    , M  ( const_cast<eT*>( A.m.colptr(0) ), A.m.n_rows, (&(A.m.m) == &B), false )
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = true;
  static const bool do_times = true;

  const eT      val;
  const Col<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_scalar_times_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_check_scalar_times_default(const eOp<T1, eop_scalar_times>& A, const Mat<eT>&)
    : val(A.aux)
    , M  (A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT      val;
  const Mat<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_scalar_times_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_check_scalar_times_fixed(const eOp<T1, eop_scalar_times>& A, const Mat<eT>& B)
    : val    ( A.aux )
    , M_local( (&(A.P.Q) == &B) ? new T1(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? (*M_local)    : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check_scalar_times_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT  val;
  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_check_scalar_times_redirect {};

template<typename T1>
struct partial_unwrap_check_scalar_times_redirect<T1, false> { typedef partial_unwrap_check_scalar_times_default<T1> result; };

template<typename T1>
struct partial_unwrap_check_scalar_times_redirect<T1, true>  { typedef partial_unwrap_check_scalar_times_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap_check< eOp<T1, eop_scalar_times> > : public partial_unwrap_check_scalar_times_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline partial_unwrap_check(const eOp<T1, eop_scalar_times>& A, const Mat<eT>& B)
    : partial_unwrap_check_scalar_times_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }
  };



template<typename eT>
struct partial_unwrap_check< eOp<Mat<eT>, eop_scalar_times> >
  {
  typedef Mat<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Mat<eT>,eop_scalar_times>& A, const Mat<eT>& B)
    : val    (A.aux)
    , M_local( (&(A.P.Q) == &B) ? new Mat<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< eOp<Row<eT>, eop_scalar_times> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Row<eT>,eop_scalar_times>& A, const Mat<eT>& B)
    : val(A.aux)
    , M_local( (&(A.P.Q) == &B) ? new Row<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< eOp<Col<eT>, eop_scalar_times> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Col<eT>,eop_scalar_times>& A, const Mat<eT>& B)
    : val    ( A.aux )
    , M_local( (&(A.P.Q) == &B) ? new Col<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT       val;
  const Col<eT>* M_local;
  const Col<eT>& M;
  };



// NOTE: we can get away with this shortcut as the partial_unwrap_check class is only used by the glue_times class,
// NOTE: which relies on partial_unwrap_check to check for aliasing
template<typename eT>
struct partial_unwrap_check< eOp<subview_col<eT>, eop_scalar_times> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<subview_col<eT>,eop_scalar_times>& A, const Mat<eT>& B)
    : val( A.aux )
    , M  ( const_cast<eT*>( A.P.Q.colptr(0) ), A.P.Q.n_rows, (&(A.P.Q.m) == &B), false )
    {
    arma_extra_debug_sigprint();
    }

  arma_hot arma_inline eT get_val() const { return val; }

  static const bool do_trans = false;
  static const bool do_times = true;

  const eT      val;
  const Col<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_neg_default
  {
  typedef typename T1::elem_type eT;
  typedef Mat<eT>                stored_type;

  inline
  partial_unwrap_check_neg_default(const eOp<T1, eop_neg>& A, const Mat<eT>&)
    : M(A.P.Q)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT> M;
  };



template<typename T1>
struct partial_unwrap_check_neg_fixed
  {
  typedef typename T1::elem_type eT;
  typedef T1                     stored_type;

  inline explicit
  partial_unwrap_check_neg_fixed(const eOp<T1, eop_neg>& A, const Mat<eT>& B)
    : M_local( (&(A.P.Q) == &B) ? new T1(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? (*M_local)    : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check_neg_fixed()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const T1* M_local;
  const T1& M;
  };



template<typename T1, bool condition>
struct partial_unwrap_check_neg_redirect {};

template<typename T1>
struct partial_unwrap_check_neg_redirect<T1, false> { typedef partial_unwrap_check_neg_default<T1> result; };

template<typename T1>
struct partial_unwrap_check_neg_redirect<T1, true>  { typedef partial_unwrap_check_neg_fixed<T1>   result; };


template<typename T1>
struct partial_unwrap_check< eOp<T1, eop_neg> > : public partial_unwrap_check_neg_redirect<T1, is_Mat_fixed<T1>::value >::result
  {
  typedef typename T1::elem_type eT;

  inline partial_unwrap_check(const eOp<T1, eop_neg>& A, const Mat<eT>& B)
    : partial_unwrap_check_neg_redirect< T1, is_Mat_fixed<T1>::value >::result(A, B)
    {
    }
  };



template<typename eT>
struct partial_unwrap_check< eOp<Mat<eT>, eop_neg> >
  {
  typedef Mat<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Mat<eT>,eop_neg>& A, const Mat<eT>& B)
    : M_local( (&(A.P.Q) == &B) ? new Mat<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Mat<eT>* M_local;
  const Mat<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< eOp<Row<eT>, eop_neg> >
  {
  typedef Row<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Row<eT>,eop_neg>& A, const Mat<eT>& B)
    : M_local( (&(A.P.Q) == &B) ? new Row<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Row<eT>* M_local;
  const Row<eT>& M;
  };



template<typename eT>
struct partial_unwrap_check< eOp<Col<eT>, eop_neg> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<Col<eT>,eop_neg>& A, const Mat<eT>& B)
    : M_local( (&(A.P.Q) == &B) ? new Col<eT>(A.P.Q) : 0     )
    , M      ( (&(A.P.Q) == &B) ? *M_local           : A.P.Q )
    {
    arma_extra_debug_sigprint();
    }

  inline
  ~partial_unwrap_check()
    {
    arma_extra_debug_sigprint();

    if(M_local) { delete M_local; }
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Col<eT>* M_local;
  const Col<eT>& M;
  };



// NOTE: we can get away with this shortcut as the partial_unwrap_check class is only used by the glue_times class,
// NOTE: which relies on partial_unwrap_check to check for aliasing
template<typename eT>
struct partial_unwrap_check< eOp<subview_col<eT>, eop_neg> >
  {
  typedef Col<eT> stored_type;

  arma_hot inline
  partial_unwrap_check(const eOp<subview_col<eT>,eop_neg>& A, const Mat<eT>& B)
    : M  ( const_cast<eT*>( A.P.Q.colptr(0) ), A.P.Q.n_rows, (&(A.P.Q.m) == &B), false )
    {
    arma_extra_debug_sigprint();
    }

  arma_inline eT get_val() const { return eT(-1); }

  static const bool do_trans = false;
  static const bool do_times = true;

  const Col<eT> M;
  };



//! @}
