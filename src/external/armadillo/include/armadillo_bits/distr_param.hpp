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



//! \addtogroup distr_param
//! @{



class distr_param
  {
  public:

  uword state;

  union
    {
    int    a_int;
    double a_double;
    };

  union
    {
    int    b_int;
    double b_double;
    };


  inline distr_param()
    : state(0)
    {
    }


  inline explicit distr_param(const int a, const int b)
    : state(1)
    , a_int(a)
    , b_int(b)
    {
    }


  inline explicit distr_param(const double a, const double b)
    : state(2)
    , a_double(a)
    , b_double(b)
    {
    }
  };



//! @}
