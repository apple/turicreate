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


//! \addtogroup hdf5_misc
//! @{


#if defined(ARMA_USE_HDF5)
namespace hdf5_misc
{


//! Given a certain type, find the corresponding HDF5 datatype.  This can't be
//! done entirely at compile time, unfortunately, because the H5T_* macros
//! depend on function calls.
template< typename eT >
inline
hid_t
get_hdf5_type()
  {
  return -1; // Return invalid.
  }



//! Specializations for each valid element type
//! (taken from all the possible typedefs of {u8, s8, ..., u64, s64} and the other native types.
//! We can't use the actual u8/s8 typedefs because their relations to the H5T_... types are unclear.
template<>
inline
hid_t
get_hdf5_type< unsigned char >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_UCHAR);
  }

template<>
inline
hid_t
get_hdf5_type< char >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_CHAR);
  }

template<>
inline
hid_t
get_hdf5_type< short >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_SHORT);
  }

template<>
inline
hid_t
get_hdf5_type< unsigned short >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_USHORT);
  }

template<>
inline
hid_t
get_hdf5_type< int >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_INT);
  }

template<>
inline
hid_t
get_hdf5_type< unsigned int >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_UINT);
  }

template<>
inline
hid_t
get_hdf5_type< long >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_LONG);
  }

template<>
inline
hid_t
get_hdf5_type< unsigned long >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_ULONG);
  }


#if defined(ARMA_USE_U64S64) && defined(ULLONG_MAX)
  template<>
  inline
  hid_t
  get_hdf5_type< long long >()
    {
    return arma_H5Tcopy(arma_H5T_NATIVE_LLONG);
    }

  template<>
  inline
  hid_t
  get_hdf5_type< unsigned long long >()
    {
    return arma_H5Tcopy(arma_H5T_NATIVE_ULLONG);
    }
#endif


template<>
inline
hid_t
get_hdf5_type< float >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_FLOAT);
  }

template<>
inline
hid_t
get_hdf5_type< double >()
  {
  return arma_H5Tcopy(arma_H5T_NATIVE_DOUBLE);
  }



//! Utility hid_t since HOFFSET() won't work with std::complex.
template<typename eT>
struct hdf5_complex_t
  {
  eT real;
  eT imag;
  };



template<>
inline
hid_t
get_hdf5_type< std::complex<float> >()
  {
  hid_t type = arma_H5Tcreate(H5T_COMPOUND, sizeof(hdf5_complex_t<float>));

  arma_H5Tinsert(type, "real", HOFFSET(hdf5_complex_t<float>, real), arma_H5T_NATIVE_FLOAT);
  arma_H5Tinsert(type, "imag", HOFFSET(hdf5_complex_t<float>, imag), arma_H5T_NATIVE_FLOAT);

  return type;
  }



template<>
inline
hid_t
get_hdf5_type< std::complex<double> >()
  {
  hid_t type = arma_H5Tcreate(H5T_COMPOUND, sizeof(hdf5_complex_t<double>));

  arma_H5Tinsert(type, "real", HOFFSET(hdf5_complex_t<double>, real), arma_H5T_NATIVE_DOUBLE);
  arma_H5Tinsert(type, "imag", HOFFSET(hdf5_complex_t<double>, imag), arma_H5T_NATIVE_DOUBLE);

  return type;
  }



// Compare datatype against all supported types.
inline
bool
is_supported_arma_hdf5_type(hid_t datatype)
  {
  hid_t  search_type;

  bool is_equal;


  // start with most likely used types: double, complex<double>, float, complex<float>

  search_type = get_hdf5_type<double>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type< std::complex<double> >();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<float>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type< std::complex<float> >();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }


  // remaining supported types: u8, s8, u16, s16, u32, s32, u64, s64, ulng_t, slng_t

  search_type = get_hdf5_type<u8>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<s8>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<u16>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<s16>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<u32>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  search_type = get_hdf5_type<s32>();
  is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
  arma_H5Tclose(search_type);
  if (is_equal) { return true; }

  #if defined(ARMA_USE_U64S64)
    {
    search_type = get_hdf5_type<u64>();
    is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
    arma_H5Tclose(search_type);
    if (is_equal) { return true; }

    search_type = get_hdf5_type<s64>();
    is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
    arma_H5Tclose(search_type);
    if (is_equal) { return true; }
    }
  #endif

  #if defined(ARMA_ALLOW_LONG)
    {
    search_type = get_hdf5_type<ulng_t>();
    is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
    arma_H5Tclose(search_type);
    if (is_equal) { return true; }

    search_type = get_hdf5_type<slng_t>();
    is_equal = ( arma_H5Tequal(datatype, search_type) > 0 );
    arma_H5Tclose(search_type);
    if (is_equal) { return true; }
    }
  #endif

  return false;
  }



//! Auxiliary functions and structs for search_hdf5_file.
struct hdf5_search_info
  {
  const  std::vector<std::string>& names;
  int    num_dims;
  bool   exact;
  hid_t  best_match;
  size_t best_match_position; // Position of best match in names vector.
  };



inline
herr_t
hdf5_search_callback
  (
  hid_t             loc_id,
  const char*       name,
  const H5O_info_t* info,
  void*             operator_data  // hdf5_search_info
  )
  {
  hdf5_search_info* search_info = (hdf5_search_info*) operator_data;

  // We are looking for datasets.
  if (info->type == H5O_TYPE_DATASET)
    {
    // Check type of dataset to see if we could even load it.
    hid_t dataset  = arma_H5Dopen(loc_id, name, H5P_DEFAULT);
    hid_t datatype = arma_H5Dget_type(dataset);

    const bool is_supported = is_supported_arma_hdf5_type(datatype);

    arma_H5Tclose(datatype);
    arma_H5Dclose(dataset);

    if(is_supported == false)
      {
      // Forget about it and move on.
      return 0;
      }

    // Now we have to check against our set of names.
    // Only check names which could be better.
    for (size_t string_pos = 0; string_pos < search_info->best_match_position; ++string_pos)
      {
      // name is the full path (/path/to/dataset); names[string_pos] may be
      // "dataset", "/to/dataset", or "/path/to/dataset".
      // So if we count the number of forward slashes in names[string_pos],
      // and then simply take the last substring of name containing that number of slashes,
      // we can do the comparison.

      // Count the number of forward slashes in names[string_pos].
      uword name_count = 0;
      for (uword i = 0; i < search_info->names[string_pos].length(); ++i)
        {
        if ((search_info->names[string_pos])[i] == '/') { ++name_count; }
        }

      // Count the number of forward slashes in the full name.
      uword count = 0;
      const std::string str = std::string(name);
      for (uword i = 0; i < str.length(); ++i)
        {
        if (str[i] == '/') { ++count; }
        }

      // Is the full string the same?
      if (str == search_info->names[string_pos])
        {
        // We found it exactly.
        hid_t match_candidate = arma_H5Dopen(loc_id, name, H5P_DEFAULT);

        if (match_candidate < 0)
          {
          return -1;
          }

        // Ensure that the dataset is valid and of the correct dimensionality.
        hid_t filespace = arma_H5Dget_space(match_candidate);
        int num_dims = arma_H5Sget_simple_extent_ndims(filespace);

        if (num_dims <= search_info->num_dims)
          {
          // Valid dataset -- we'll keep it.
          // If we already have an existing match we have to close it.
          if (search_info->best_match != -1)
            {
            arma_H5Dclose(search_info->best_match);
            }

          search_info->best_match_position = string_pos;
          search_info->best_match          = match_candidate;
          }

        arma_H5Sclose(filespace);
        // There is no possibility of anything better, so terminate the search.
        return 1;
        }

      // If we are asking for more slashes than we have, this can't be a match.
      // Skip to below, where we decide whether or not to keep it anyway based
      // on the exactness condition of the search.
      if (count <= name_count)
        {
        size_t start_pos = (count == 0) ? 0 : std::string::npos;
        while (count > 0)
          {
          // Move pointer to previous slash.
          start_pos = str.rfind('/', start_pos);

          // Break if we've run out of slashes.
          if (start_pos == std::string::npos) { break; }

          --count;
          }

        // Now take the substring (this may end up being the full string).
        const std::string substring = str.substr(start_pos);

        // Are they the same?
        if (substring == search_info->names[string_pos])
          {
          // We have found the object; it must be better than our existing match.
          hid_t match_candidate = arma_H5Dopen(loc_id, name, H5P_DEFAULT);


          // arma_check(match_candidate < 0, "Mat::load(): cannot open an HDF5 dataset");
          if(match_candidate < 0)
            {
            return -1;
            }


          // Ensure that the dataset is valid and of the correct dimensionality.
          hid_t filespace = arma_H5Dget_space(match_candidate);
          int num_dims = arma_H5Sget_simple_extent_ndims(filespace);

          if (num_dims <= search_info->num_dims)
            {
            // Valid dataset -- we'll keep it.
            // If we already have an existing match we have to close it.
            if (search_info->best_match != -1)
              {
              arma_H5Dclose(search_info->best_match);
              }

            search_info->best_match_position = string_pos;
            search_info->best_match          = match_candidate;
            }

          arma_H5Sclose(filespace);
          }
        }


      // If they are not the same, but we have not found anything and we don't need an exact match, take this.
      if ((search_info->exact == false) && (search_info->best_match == -1))
        {
        hid_t match_candidate = arma_H5Dopen(loc_id, name, H5P_DEFAULT);

        // arma_check(match_candidate < 0, "Mat::load(): cannot open an HDF5 dataset");
        if(match_candidate < 0)
          {
          return -1;
          }

        hid_t filespace = arma_H5Dget_space(match_candidate);
        int num_dims = arma_H5Sget_simple_extent_ndims(filespace);

        if (num_dims <= search_info->num_dims)
          {
          // Valid dataset -- we'll keep it.
          search_info->best_match = arma_H5Dopen(loc_id, name, H5P_DEFAULT);
          }

        arma_H5Sclose(filespace);
        }
      }
    }

  return 0;
  }



//! Search an HDF5 file for the given dataset names.
//! If 'exact' is true, failure to find a dataset in the list of names means that -1 is returned.
//! If 'exact' is false and no datasets are found, -1 is returned.
//! The number of dimensions is used to help prune down invalid datasets;
//! 2 dimensions is a matrix, 1 dimension is a vector, and 3 dimensions is a cube.
//! If the number of dimensions in a dataset is less than or equal to num_dims,
//! it will be considered -- for instance, a one-dimensional HDF5 vector can be loaded as a single-column matrix.
inline
hid_t
search_hdf5_file
  (
  const std::vector<std::string>& names,
  hid_t                           hdf5_file,
  int                             num_dims = 2,
  bool                            exact = false
  )
  {
  hdf5_search_info search_info = { names, num_dims, exact, -1, names.size() };

  // We'll use the H5Ovisit to track potential entries.
  herr_t status = arma_H5Ovisit(hdf5_file, H5_INDEX_NAME, H5_ITER_NATIVE, hdf5_search_callback, void_ptr(&search_info));

  // Return the best match; it will be -1 if there was a problem.
  return (status < 0) ? -1 : search_info.best_match;
  }



//! Load an HDF5 matrix into an array of type specified by datatype,
//! then convert that into the desired array 'dest'.
//! This should only be called when eT is not the datatype.
template<typename eT>
inline
hid_t
load_and_convert_hdf5
  (
  eT   *dest,
  hid_t dataset,
  hid_t datatype,
  uword n_elem
  )
  {

  // We can't use nice template specializations here
  // as the determination of the type of 'datatype' must be done at runtime.
  // So we end up with this ugliness...
  hid_t search_type;

  bool is_equal;


  // u8
  search_type = get_hdf5_type<u8>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<u8> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // s8
  search_type = get_hdf5_type<s8>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<s8> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // u16
  search_type = get_hdf5_type<u16>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<u16> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // s16
  search_type = get_hdf5_type<s16>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<s16> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // u32
  search_type = get_hdf5_type<u32>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<u32> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // s32
  search_type = get_hdf5_type<s32>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<s32> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  #if defined(ARMA_USE_U64S64)
    {
    // u64
    search_type = get_hdf5_type<u64>();
    is_equal = (arma_H5Tequal(datatype, search_type) > 0);
    arma_H5Tclose(search_type);

    if(is_equal)
      {
      Col<u64> v(n_elem);
      hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
      arrayops::convert(dest, v.memptr(), n_elem);

      return status;
      }


    // s64
    search_type = get_hdf5_type<s64>();
    is_equal = (arma_H5Tequal(datatype, search_type) > 0);
    arma_H5Tclose(search_type);

    if(is_equal)
      {
      Col<s64> v(n_elem);
      hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
      arrayops::convert(dest, v.memptr(), n_elem);

      return status;
      }
    }
  #endif


  #if defined(ARMA_ALLOW_LONG)
    {
    // ulng_t
    search_type = get_hdf5_type<ulng_t>();
    is_equal = (arma_H5Tequal(datatype, search_type) > 0);
    arma_H5Tclose(search_type);

    if(is_equal)
      {
      Col<ulng_t> v(n_elem);
      hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
      arrayops::convert(dest, v.memptr(), n_elem);

      return status;
      }


    // slng_t
    search_type = get_hdf5_type<slng_t>();
    is_equal = (arma_H5Tequal(datatype, search_type) > 0);
    arma_H5Tclose(search_type);

    if(is_equal)
      {
      Col<slng_t> v(n_elem);
      hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
      arrayops::convert(dest, v.memptr(), n_elem);

      return status;
      }
    }
  #endif


  // float
  search_type = get_hdf5_type<float>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<float> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // double
  search_type = get_hdf5_type<double>();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    Col<double> v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert(dest, v.memptr(), n_elem);

    return status;
    }


  // complex float
  search_type = get_hdf5_type< std::complex<float> >();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    if(is_complex<eT>::value == false)
      {
      return -1; // can't read complex data into non-complex matrix/cube
      }

    Col< std::complex<float> > v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert_cx(dest, v.memptr(), n_elem);

    return status;
    }


  // complex double
  search_type = get_hdf5_type< std::complex<double> >();
  is_equal = (arma_H5Tequal(datatype, search_type) > 0);
  arma_H5Tclose(search_type);

  if(is_equal)
    {
    if(is_complex<eT>::value == false)
      {
      return -1; // can't read complex data into non-complex matrix/cube
      }

    Col< std::complex<double> > v(n_elem);
    hid_t status = arma_H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, void_ptr(v.memptr()));
    arrayops::convert_cx(dest, v.memptr(), n_elem);

    return status;
    }


  return -1; // Failure.
  }



}       // namespace hdf5_misc
#endif  // #if defined(ARMA_USE_HDF5)



//! @}
