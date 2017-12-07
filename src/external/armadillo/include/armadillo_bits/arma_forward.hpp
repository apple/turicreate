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


using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::size_t;

template<typename elem_type, typename derived> struct Base;
template<typename elem_type, typename derived> struct BaseCube;

template<typename eT> class Mat;
template<typename eT> class Col;
template<typename eT> class Row;
template<typename eT> class Cube;
template<typename eT> class xvec_htrans;
template<typename oT> class field;

template<typename eT, bool do_conj> class xtrans_mat;


template<typename eT> class subview;
template<typename eT> class subview_col;
template<typename eT> class subview_row;
template<typename eT> class subview_row_strans;
template<typename eT> class subview_row_htrans;
template<typename eT> class subview_cube;
template<typename oT> class subview_field;

template<typename eT> class SpValProxy;
template<typename eT> class SpMat;
template<typename eT> class SpCol;
template<typename eT> class SpRow;
template<typename eT> class SpSubview;

template<typename eT> class diagview;
template<typename eT> class spdiagview;

template<typename eT> class MapMat;
template<typename eT> class MapMat_val;
template<typename eT> class MapMat_elem;
template<typename eT> class MapMat_svel;

template<typename eT, typename T1>              class subview_elem1;
template<typename eT, typename T1, typename T2> class subview_elem2;

template<typename parent, unsigned int mode>              class subview_each1;
template<typename parent, unsigned int mode, typename TB> class subview_each2;

template<typename eT>              class subview_cube_each1;
template<typename eT, typename TB> class subview_cube_each2;


class SizeMat;
class SizeCube;

class arma_empty_class {};

class diskio;

class op_min;
class op_max;

class op_strans;
class op_htrans;
class op_htrans2;
class op_inv;
class op_sum;
class op_abs;
class op_arg;
class op_diagmat;
class op_trimat;
class op_diagvec;
class op_vectorise_col;
class op_normalise_vec;
class op_clamp;
class op_cumsum_default;
class op_cumprod_default;
class op_shift;
class op_shift_default;
class op_shuffle;
class op_shuffle_default;
class op_sort;
class op_sort_default;
class op_find;
class op_find_simple;
class op_find_unique;
class op_flipud;
class op_fliplr;
class op_real;
class op_imag;
class op_nonzeros;
class op_sort_index;
class op_stable_sort_index;
class op_unique;
class op_unique_index;
class op_diff_default;
class op_hist;

class eop_conj;

class glue_times;
class glue_times_diag;
class glue_conv;
class glue_join_cols;
class glue_join_rows;
class glue_atan2;
class glue_hypot;
class glue_max;
class glue_min;
class glue_polyfit;
class glue_polyval;
class glue_intersect;
class glue_affmul;

class glue_rel_lt;
class glue_rel_gt;
class glue_rel_lteq;
class glue_rel_gteq;
class glue_rel_eq;
class glue_rel_noteq;
class glue_rel_and;
class glue_rel_or;

class op_rel_lt_pre;
class op_rel_lt_post;
class op_rel_gt_pre;
class op_rel_gt_post;
class op_rel_lteq_pre;
class op_rel_lteq_post;
class op_rel_gteq_pre;
class op_rel_gteq_post;
class op_rel_eq;
class op_rel_noteq;

class gen_eye;
class gen_ones;
class gen_zeros;
class gen_randu;
class gen_randn;

class glue_mixed_plus;
class glue_mixed_minus;
class glue_mixed_div;
class glue_mixed_schur;
class glue_mixed_times;

class glue_hist;
class glue_hist_default;

class glue_histc;
class glue_histc_default;

class op_cx_scalar_times;
class op_cx_scalar_plus;
class op_cx_scalar_minus_pre;
class op_cx_scalar_minus_post;
class op_cx_scalar_div_pre;
class op_cx_scalar_div_post;



class op_internal_equ;
class op_internal_plus;
class op_internal_minus;
class op_internal_schur;
class op_internal_div;



template<const bool, const bool, const bool, const bool> class gemm;
template<const bool, const bool, const bool>             class gemv;


template<                 typename eT, typename gen_type> class  Gen;

template<                 typename T1, typename  op_type> class   Op;
template<                 typename T1, typename eop_type> class  eOp;
template<typename out_eT, typename T1, typename  op_type> class mtOp;

template<                 typename T1, typename T2, typename  glue_type> class   Glue;
template<                 typename T1, typename T2, typename eglue_type> class  eGlue;
template<typename out_eT, typename T1, typename T2, typename  glue_type> class mtGlue;



template<                 typename eT, typename gen_type> class  GenCube;

template<                 typename T1, typename  op_type> class   OpCube;
template<                 typename T1, typename eop_type> class  eOpCube;
template<typename out_eT, typename T1, typename  op_type> class mtOpCube;

template<                 typename T1, typename T2, typename  glue_type> class   GlueCube;
template<                 typename T1, typename T2, typename eglue_type> class  eGlueCube;
template<typename out_eT, typename T1, typename T2, typename  glue_type> class mtGlueCube;


template<typename T1> class Proxy;
template<typename T1> class ProxyCube;

template<typename T1> class diagmat_proxy;

class spop_strans;
class spop_htrans;
class spop_scalar_times;

class spglue_plus;
class spglue_plus2;

class spglue_minus;
class spglue_minus2;

class spglue_times;
class spglue_times2;


template<                 typename T1, typename spop_type> class   SpOp;
template<typename out_eT, typename T1, typename spop_type> class mtSpOp;

template<typename T1, typename T2, typename spglue_type> class SpGlue;


template<typename T1> class SpProxy;



struct arma_vec_indicator   {};
struct arma_fixed_indicator {};


//! \addtogroup injector
//! @{

template<typename Dummy = int> struct injector_end_of_row {};

static const injector_end_of_row<> endr = injector_end_of_row<>();
//!< endr indicates "end of row" when using the << operator;
//!< similar conceptual meaning to std::endl

//! @}



//! \addtogroup diskio
//! @{


enum file_type
  {
  file_type_unknown,
  auto_detect,        //!< Automatically detect the file type
  raw_ascii,          //!< ASCII format (text), without any other information.
  arma_ascii,         //!< Armadillo ASCII format (text), with information about matrix type and size
  csv_ascii,          //!< comma separated values (CSV), without any other information
  raw_binary,         //!< raw binary format, without any other information.
  arma_binary,        //!< Armadillo binary format, with information about matrix type and size
  pgm_binary,         //!< Portable Grey Map (greyscale image)
  ppm_binary,         //!< Portable Pixel Map (colour image), used by the field and cube classes
  hdf5_binary,        //!< Open binary format, not specific to Armadillo, which can store arbitrary data
  hdf5_binary_trans,  //!< as per hdf5_binary, but save/load the data with columns transposed to rows
  coord_ascii         //!< simple co-ordinate format for sparse matrices
  };


struct hdf5_name
  {
  const std::string filename;
  const std::string dsname;

  inline
  hdf5_name(const std::string& in_filename)
    : filename(in_filename)
    {}

  inline
  hdf5_name(const std::string& in_filename, const std::string& in_dsname)
    : filename(in_filename)
    , dsname  (in_dsname  )
    {}
  };


//! @}



//! \addtogroup fill
//! @{

namespace fill
  {
  struct fill_none  {};
  struct fill_zeros {};
  struct fill_ones  {};
  struct fill_eye   {};
  struct fill_randu {};
  struct fill_randn {};

  template<typename fill_type>
  struct fill_class { inline fill_class() {} };

  static const fill_class<fill_none > none;
  static const fill_class<fill_zeros> zeros;
  static const fill_class<fill_ones > ones;
  static const fill_class<fill_eye  > eye;
  static const fill_class<fill_randu> randu;
  static const fill_class<fill_randn> randn;
  }

//! @}



//! \addtogroup fn_spsolve
//! @{


struct spsolve_opts_base
  {
  const unsigned int id;

  inline spsolve_opts_base(const unsigned int in_id) : id(in_id) {}
  };


struct spsolve_opts_none : public spsolve_opts_base
  {
  inline spsolve_opts_none() : spsolve_opts_base(0) {}
  };


struct superlu_opts : public spsolve_opts_base
  {
  typedef enum {NATURAL, MMD_ATA, MMD_AT_PLUS_A, COLAMD} permutation_type;

  typedef enum {REF_NONE, REF_SINGLE, REF_DOUBLE, REF_EXTRA} refine_type;

  bool             equilibrate;
  bool             symmetric;
  double           pivot_thresh;
  permutation_type permutation;
  refine_type      refine;

  inline superlu_opts()
    : spsolve_opts_base(1)
    {
    equilibrate  = false;
    symmetric    = false;
    pivot_thresh = 1.0;
    permutation  = COLAMD;
    refine       = REF_DOUBLE;
    }
  };


//! @}
