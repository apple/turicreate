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


namespace newarp
{


// When comparing eigenvalues, we first calculate the "target" to sort.
// For example, if we want to choose the eigenvalues with largest magnitude, the target will be -std::abs(x).
// The minus sign is due to the fact that std::sort() sorts in ascending order.


// default target: throw an exception
template<typename eT, int SelectionRule>
struct SortingTarget
  {
  arma_inline static typename get_pod_type<eT>::result get(const eT& val)
    {
    arma_ignore(val);
    arma_stop_logic_error("newarp::SortingTarget: incompatible selection rule");

    typedef typename get_pod_type<eT>::result out_T;
    return out_T(0);
    }
  };


// specialisation for LARGEST_MAGN: this covers [float, double, complex] x [LARGEST_MAGN]
template<typename eT>
struct SortingTarget<eT, EigsSelect::LARGEST_MAGN>
  {
  arma_inline static typename get_pod_type<eT>::result get(const eT& val)
    {
    return -std::abs(val);
    }
  };


// specialisation for LARGEST_REAL: this covers [complex] x [LARGEST_REAL]
template<typename T>
struct SortingTarget<std::complex<T>, EigsSelect::LARGEST_REAL>
  {
  arma_inline static T get(const std::complex<T>& val)
    {
    return -val.real();
    }
  };


// specialisation for LARGEST_IMAG: this covers [complex] x [LARGEST_IMAG]
template<typename T>
struct SortingTarget<std::complex<T>, EigsSelect::LARGEST_IMAG>
  {
  arma_inline static T get(const std::complex<T>& val)
    {
    return -std::abs(val.imag());
    }
  };


// specialisation for LARGEST_ALGE: this covers [float, double] x [LARGEST_ALGE]
template<typename eT>
struct SortingTarget<eT, EigsSelect::LARGEST_ALGE>
  {
  arma_inline static eT get(const eT& val)
    {
    return -val;
    }
  };


// Here BOTH_ENDS is the same as LARGEST_ALGE, but we need some additional steps,
// which are done in SymEigsSolver => retrieve_ritzpair().
// There we move the smallest values to the proper locations.
template<typename eT>
struct SortingTarget<eT, EigsSelect::BOTH_ENDS>
  {
  arma_inline static eT get(const eT& val)
    {
    return -val;
    }
  };


// specialisation for SMALLEST_MAGN: this covers [float, double, complex] x [SMALLEST_MAGN]
template<typename eT>
struct SortingTarget<eT, EigsSelect::SMALLEST_MAGN>
  {
  arma_inline static typename get_pod_type<eT>::result get(const eT& val)
    {
    return std::abs(val);
    }
  };


// specialisation for SMALLEST_REAL: this covers [complex] x [SMALLEST_REAL]
template<typename T>
struct SortingTarget<std::complex<T>, EigsSelect::SMALLEST_REAL>
  {
  arma_inline static T get(const std::complex<T>& val)
    {
    return val.real();
    }
  };


// specialisation for SMALLEST_IMAG: this covers [complex] x [SMALLEST_IMAG]
template<typename T>
struct SortingTarget<std::complex<T>, EigsSelect::SMALLEST_IMAG>
  {
  arma_inline static T get(const std::complex<T>& val)
    {
    return std::abs(val.imag());
    }
  };


// specialisation for SMALLEST_ALGE: this covers [float, double] x [SMALLEST_ALGE]
template<typename eT>
struct SortingTarget<eT, EigsSelect::SMALLEST_ALGE>
  {
  arma_inline static eT get(const eT& val)
    {
    return val;
    }
  };


// Sort eigenvalues and return the order index
template<typename PairType>
struct PairComparator
  {
  arma_inline bool operator() (const PairType& v1, const PairType& v2)
    {
    return v1.first < v2.first;
    }
  };


template<typename eT, int SelectionRule>
class SortEigenvalue
  {
  private:

  typedef typename get_pod_type<eT>::result TargetType;  // type of the sorting target, will be a floating number type, eg. double
  typedef std::pair<TargetType, uword>      PairType;    // type of the sorting pair, including the sorting target and the index

  std::vector<PairType> pair_sort;


  public:

  inline
  SortEigenvalue(const eT* start, const uword size)
    : pair_sort(size)
    {
    arma_extra_debug_sigprint();

    for(uword i = 0; i < size; i++)
      {
      pair_sort[i].first  = SortingTarget<eT, SelectionRule>::get(start[i]);
      pair_sort[i].second = i;
      }

    PairComparator<PairType> comp;

    std::sort(pair_sort.begin(), pair_sort.end(), comp);
    }


  inline
  std::vector<uword>
  index()
    {
    arma_extra_debug_sigprint();

    const uword len = pair_sort.size();

    std::vector<uword> ind(len);

    for(uword i = 0; i < len; i++) { ind[i] = pair_sort[i].second; }

    return ind;
    }
  };


}  // namespace newarp
