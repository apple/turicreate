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


//! \addtogroup wall_clock
//! @{


inline
wall_clock::wall_clock()
  : valid(false)
  {
  arma_extra_debug_sigprint();
  }



inline
wall_clock::~wall_clock()
  {
  arma_extra_debug_sigprint();
  }



inline
void
wall_clock::tic()
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_CXX11)
    {
    chrono_time1 = std::chrono::steady_clock::now();
    valid = true;
    }
  #elif defined(ARMA_HAVE_GETTIMEOFDAY)
    {
    gettimeofday(&posix_time1, 0);
    valid = true;
    }
  #else
    {
    time1 = std::clock();
    valid = true;
    }
  #endif
  }



inline
double
wall_clock::toc()
  {
  arma_extra_debug_sigprint();

  if(valid)
    {
    #if defined(ARMA_USE_CXX11)
      {
      const std::chrono::steady_clock::time_point chrono_time2 = std::chrono::steady_clock::now();

      typedef std::chrono::duration<double> duration_type;

      const duration_type chrono_span = std::chrono::duration_cast< duration_type >(chrono_time2 - chrono_time1);

      return chrono_span.count();
      }
    #elif defined(ARMA_HAVE_GETTIMEOFDAY)
      {
      gettimeofday(&posix_time2, 0);

      const double tmp_time1 = double(posix_time1.tv_sec) + double(posix_time1.tv_usec) * 1.0e-6;
      const double tmp_time2 = double(posix_time2.tv_sec) + double(posix_time2.tv_usec) * 1.0e-6;

      return tmp_time2 - tmp_time1;
      }
    #else
      {
      std::clock_t time2 = std::clock();

      std::clock_t diff = time2 - time1;

      return double(diff) / double(CLOCKS_PER_SEC);
      }
    #endif
    }
  else
    {
    return 0.0;
    }
  }



//! @}
