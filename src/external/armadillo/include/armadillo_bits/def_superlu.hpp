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

#if defined(ARMA_USE_SUPERLU)

extern "C"
  {
  extern void arma_wrapper(sgssv)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(dgssv)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(cgssv)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(zgssv)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperMatrix*, superlu::SuperLUStat_t*, int*);

  extern void arma_wrapper(sgssvx)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, int*, char*,  float*,  float*, superlu::SuperMatrix*, superlu::SuperMatrix*, void*, int, superlu::SuperMatrix*, superlu::SuperMatrix*,  float*,  float*,  float*,  float*, superlu::GlobalLU_t*, superlu::mem_usage_t*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(dgssvx)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, int*, char*, double*, double*, superlu::SuperMatrix*, superlu::SuperMatrix*, void*, int, superlu::SuperMatrix*, superlu::SuperMatrix*, double*, double*, double*, double*, superlu::GlobalLU_t*, superlu::mem_usage_t*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(cgssvx)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, int*, char*,  float*,  float*, superlu::SuperMatrix*, superlu::SuperMatrix*, void*, int, superlu::SuperMatrix*, superlu::SuperMatrix*,  float*,  float*,  float*,  float*, superlu::GlobalLU_t*, superlu::mem_usage_t*, superlu::SuperLUStat_t*, int*);
  extern void arma_wrapper(zgssvx)(superlu::superlu_options_t*, superlu::SuperMatrix*, int*, int*, int*, char*, double*, double*, superlu::SuperMatrix*, superlu::SuperMatrix*, void*, int, superlu::SuperMatrix*, superlu::SuperMatrix*, double*, double*, double*, double*, superlu::GlobalLU_t*, superlu::mem_usage_t*, superlu::SuperLUStat_t*, int*);

  extern void arma_wrapper(StatInit)(superlu::SuperLUStat_t*);
  extern void arma_wrapper(StatFree)(superlu::SuperLUStat_t*);
  extern void arma_wrapper(set_default_options)(superlu::superlu_options_t*);

  extern void arma_wrapper(Destroy_SuperNode_Matrix)(superlu::SuperMatrix*);
  extern void arma_wrapper(Destroy_CompCol_Matrix)(superlu::SuperMatrix*);
  extern void arma_wrapper(Destroy_SuperMatrix_Store)(superlu::SuperMatrix*);

  // We also need superlu_malloc() and superlu_free().
  // When using the original SuperLU code directly, you (the user) may
  // define USER_MALLOC and USER_FREE, but the joke is on you because
  // if you are linking against SuperLU and not compiling from scratch,
  // it won't actually make a difference anyway!  If you've compiled
  // SuperLU against a custom USER_MALLOC and USER_FREE, you're probably up
  // shit creek about a thousand different ways before you even get to this
  // code, so, don't do that!

  extern void* arma_wrapper(superlu_malloc)(size_t);
  extern void  arma_wrapper(superlu_free)(void*);
  }

#endif
