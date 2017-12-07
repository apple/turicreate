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


#if defined(ARMA_USE_HDF5)

#if !defined(ARMA_USE_HDF5_ALT)

  // macros needed if the wrapper run-time library is not being used

  #define arma_H5Tcopy      H5Tcopy
  #define arma_H5Tcreate    H5Tcreate
  #define arma_H5Tinsert    H5Tinsert
  #define arma_H5Tequal     H5Tequal
  #define arma_H5Tclose     H5Tclose

  #define arma_H5Dopen      H5Dopen
  #define arma_H5Dget_type  H5Dget_type
  #define arma_H5Dclose     H5Dclose
  #define arma_H5Dwrite     H5Dwrite
  #define arma_H5Dget_space H5Dget_space
  #define arma_H5Dread      H5Dread
  #define arma_H5Dcreate    H5Dcreate

  #define arma_H5Sget_simple_extent_ndims   H5Sget_simple_extent_ndims
  #define arma_H5Sget_simple_extent_dims    H5Sget_simple_extent_dims
  #define arma_H5Sclose                     H5Sclose
  #define arma_H5Screate_simple             H5Screate_simple

  #define arma_H5Ovisit     H5Ovisit

  #define arma_H5Eset_auto  H5Eset_auto
  #define arma_H5Eget_auto  H5Eget_auto

  #define arma_H5Fopen      H5Fopen
  #define arma_H5Fcreate    H5Fcreate
  #define arma_H5Fclose     H5Fclose
  #define arma_H5Fis_hdf5   H5Fis_hdf5

  #define arma_H5Gcreate    H5Gcreate
  #define arma_H5Gclose     H5Gclose

  #define arma_H5T_NATIVE_UCHAR   H5T_NATIVE_UCHAR
  #define arma_H5T_NATIVE_CHAR    H5T_NATIVE_CHAR
  #define arma_H5T_NATIVE_SHORT   H5T_NATIVE_SHORT
  #define arma_H5T_NATIVE_USHORT  H5T_NATIVE_USHORT
  #define arma_H5T_NATIVE_INT     H5T_NATIVE_INT
  #define arma_H5T_NATIVE_UINT    H5T_NATIVE_UINT
  #define arma_H5T_NATIVE_LONG    H5T_NATIVE_LONG
  #define arma_H5T_NATIVE_ULONG   H5T_NATIVE_ULONG
  #define arma_H5T_NATIVE_LLONG   H5T_NATIVE_LLONG
  #define arma_H5T_NATIVE_ULLONG  H5T_NATIVE_ULLONG
  #define arma_H5T_NATIVE_FLOAT   H5T_NATIVE_FLOAT
  #define arma_H5T_NATIVE_DOUBLE  H5T_NATIVE_DOUBLE

#else

// prototypes for the wrapper functions defined in the wrapper run-time library (src/wrapper.cpp)

extern "C"
  {
  // Wrapper functions for H5* functions.
  hid_t  arma_H5Tcopy(hid_t dtype_id);
  hid_t  arma_H5Tcreate(H5T_class_t cl, size_t size);
  herr_t arma_H5Tinsert(hid_t dtype_id, const char* name, size_t offset, hid_t field_id);
  htri_t arma_H5Tequal(hid_t dtype_id1, hid_t dtype_id2);
  herr_t arma_H5Tclose(hid_t dtype_id);

  hid_t  arma_H5Dopen(hid_t loc_id, const char* name, hid_t dapl_id);
  hid_t  arma_H5Dget_type(hid_t dataset_id);
  herr_t arma_H5Dclose(hid_t dataset_id);
  hid_t  arma_H5Dcreate(hid_t loc_id, const char* name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id);
  herr_t arma_H5Dwrite(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void* buf);
  hid_t  arma_H5Dget_space(hid_t dataset_id);
  herr_t arma_H5Dread(hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void* buf);

  int    arma_H5Sget_simple_extent_ndims(hid_t space_id);
  int    arma_H5Sget_simple_extent_dims(hid_t space_id, hsize_t* dims, hsize_t* maxdims);
  herr_t arma_H5Sclose(hid_t space_id);
  hid_t  arma_H5Screate_simple(int rank, const hsize_t* current_dims, const hsize_t* maximum_dims);

  herr_t arma_H5Ovisit(hid_t object_id, H5_index_t index_type, H5_iter_order_t order, H5O_iterate_t op, void* op_data);

  herr_t arma_H5Eset_auto(hid_t estack_id, H5E_auto_t func, void* client_data);
  herr_t arma_H5Eget_auto(hid_t estack_id, H5E_auto_t* func, void** client_data);

  hid_t  arma_H5Fopen(const char* name, unsigned flags, hid_t fapl_id);
  hid_t  arma_H5Fcreate(const char* name, unsigned flags, hid_t fcpl_id, hid_t fapl_id);
  herr_t arma_H5Fclose(hid_t file_id);
  htri_t arma_H5Fis_hdf5(const char* name);

  hid_t  arma_H5Gcreate(hid_t loc_id, const char* name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id);
  herr_t arma_H5Gclose(hid_t group_id);

  // Wrapper variables that represent the hid_t values for the H5T_NATIVE_*
  // types.  Note that H5T_NATIVE_UCHAR itself is a macro that resolves to about
  // forty other macros, and we definitely don't want to hijack those,
  // so this is the best way to go about wrapping these...
  extern hid_t arma_H5T_NATIVE_UCHAR;
  extern hid_t arma_H5T_NATIVE_CHAR;
  extern hid_t arma_H5T_NATIVE_SHORT;
  extern hid_t arma_H5T_NATIVE_USHORT;
  extern hid_t arma_H5T_NATIVE_INT;
  extern hid_t arma_H5T_NATIVE_UINT;
  extern hid_t arma_H5T_NATIVE_LONG;
  extern hid_t arma_H5T_NATIVE_ULONG;
  extern hid_t arma_H5T_NATIVE_LLONG;
  extern hid_t arma_H5T_NATIVE_ULLONG;
  extern hid_t arma_H5T_NATIVE_FLOAT;
  extern hid_t arma_H5T_NATIVE_DOUBLE;

  }

  // Lastly, we have to hijack H5open() and H5check_version(), which are called
  // by some expanded macros of the other H5* functions.  This means we can't
  // create arma_H5open(), because we can't modify those macros.  Instead, we'll
  // create arma::H5open() and arma::H5check_version(), and then issue a using
  // directive so that arma::H5open() and arma::H5check_version() are always
  // called.
  //
  // There is potential danger in the use of a using directive like this, but in
  // this case, I can't think of a better way to solve the problem, and I doubt
  // this will cause problems in any situations that aren't truly bizarre.  And
  // if it does cause problems, the user can #define ARMA_DONT_USE_WRAPPER or
  // #undef ARMA_USE_WRAPPER in their Armadillo configuration.
  herr_t H5open();
  herr_t H5check_version(unsigned majnum, unsigned minnum, unsigned relnum);

  using arma::H5open;
  using arma::H5check_version;

#endif

#endif
