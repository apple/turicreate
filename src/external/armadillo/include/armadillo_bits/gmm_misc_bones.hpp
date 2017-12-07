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


//! \addtogroup gmm_misc
//! @{


struct gmm_dist_mode { const uword id;  inline explicit gmm_dist_mode(const uword in_id) : id(in_id) {} };

inline bool operator==(const gmm_dist_mode& a, const gmm_dist_mode& b) { return (a.id == b.id); }
inline bool operator!=(const gmm_dist_mode& a, const gmm_dist_mode& b) { return (a.id != b.id); }

struct gmm_dist_eucl : public gmm_dist_mode { inline gmm_dist_eucl() : gmm_dist_mode(1) {} };
struct gmm_dist_maha : public gmm_dist_mode { inline gmm_dist_maha() : gmm_dist_mode(2) {} };
struct gmm_dist_prob : public gmm_dist_mode { inline gmm_dist_prob() : gmm_dist_mode(3) {} };

static const gmm_dist_eucl eucl_dist;
static const gmm_dist_maha maha_dist;
static const gmm_dist_prob prob_dist;



struct gmm_seed_mode { const uword id; inline explicit gmm_seed_mode(const uword in_id) : id(in_id) {} };

inline bool operator==(const gmm_seed_mode& a, const gmm_seed_mode& b) { return (a.id == b.id); }
inline bool operator!=(const gmm_seed_mode& a, const gmm_seed_mode& b) { return (a.id != b.id); }

struct gmm_seed_keep_existing : public gmm_seed_mode { inline gmm_seed_keep_existing() : gmm_seed_mode(1) {} };
struct gmm_seed_static_subset : public gmm_seed_mode { inline gmm_seed_static_subset() : gmm_seed_mode(2) {} };
struct gmm_seed_static_spread : public gmm_seed_mode { inline gmm_seed_static_spread() : gmm_seed_mode(3) {} };
struct gmm_seed_random_subset : public gmm_seed_mode { inline gmm_seed_random_subset() : gmm_seed_mode(4) {} };
struct gmm_seed_random_spread : public gmm_seed_mode { inline gmm_seed_random_spread() : gmm_seed_mode(5) {} };

static const gmm_seed_keep_existing keep_existing;
static const gmm_seed_static_subset static_subset;
static const gmm_seed_static_spread static_spread;
static const gmm_seed_random_subset random_subset;
static const gmm_seed_random_spread random_spread;


namespace gmm_priv
{


template<typename eT> class gmm_diag;
template<typename eT> class gmm_full;


struct gmm_empty_arg {};


// running_mean_scalar

template<typename eT>
class running_mean_scalar
  {
  public:

  inline running_mean_scalar();
  inline running_mean_scalar(const running_mean_scalar& in_rms);

  inline const running_mean_scalar& operator=(const running_mean_scalar& in_rms);

  arma_hot inline void operator() (const eT X);

  inline void  reset();

  inline uword count() const;
  inline eT    mean()  const;


  private:

  arma_aligned uword counter;
  arma_aligned eT    r_mean;
  };



// distance

template<typename eT, uword dist_id>
struct distance {};


template<typename eT>
struct distance<eT, uword(1)>
  {
  arma_inline arma_hot static eT eval(const uword N, const eT* A, const eT* B, const eT*);
  };



template<typename eT>
struct distance<eT, uword(2)>
  {
  arma_inline arma_hot static eT eval(const uword N, const eT* A, const eT* B, const eT* C);
  };


}


//! @}
