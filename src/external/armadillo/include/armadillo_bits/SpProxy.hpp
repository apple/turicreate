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


//! \addtogroup SpProxy
//! @{



template<typename eT>
class SpProxy< SpMat<eT> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpMat<eT>                                stored_type;

  typedef typename SpMat<eT>::const_iterator       const_iterator_type;
  typedef typename SpMat<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = false;

  static const bool is_row = false;
  static const bool is_col = false;

  arma_aligned const SpMat<eT>& Q;

  inline explicit SpProxy(const SpMat<eT>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    Q.sync();
    }

  arma_inline uword get_n_rows()    const { return Q.n_rows;    }
  arma_inline uword get_n_cols()    const { return Q.n_cols;    }
  arma_inline uword get_n_elem()    const { return Q.n_elem;    }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero; }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>& X) const { return (void_ptr(&Q) == void_ptr(&X)); }
  };



template<typename eT>
class SpProxy< SpCol<eT> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpCol<eT>                                stored_type;

  typedef typename SpCol<eT>::const_iterator       const_iterator_type;
  typedef typename SpCol<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = false;

  static const bool is_row = false;
  static const bool is_col = true;

  arma_aligned const SpCol<eT>& Q;

  inline explicit SpProxy(const SpCol<eT>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    Q.sync();
    }

  arma_inline uword get_n_rows()    const { return Q.n_rows;    }
  arma_inline uword get_n_cols()    const { return 1;           }
  arma_inline uword get_n_elem()    const { return Q.n_elem;    }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero; }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword)             const { return Q.begin();            }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>& X) const { return (void_ptr(&Q) == void_ptr(&X)); }
  };



template<typename eT>
class SpProxy< SpRow<eT> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpRow<eT>                                stored_type;

  typedef typename SpRow<eT>::const_iterator       const_iterator_type;
  typedef typename SpRow<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = false;

  static const bool is_row = true;
  static const bool is_col = false;

  arma_aligned const SpRow<eT>& Q;

  inline explicit SpProxy(const SpRow<eT>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    Q.sync();
    }

  arma_inline uword get_n_rows()    const { return 1;           }
  arma_inline uword get_n_cols()    const { return Q.n_cols;    }
  arma_inline uword get_n_elem()    const { return Q.n_elem;    }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero; }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>& X) const { return (void_ptr(&Q) == void_ptr(&X)); }
  };



template<typename eT>
class SpProxy< SpSubview<eT> >
  {
  public:

  typedef eT                                           elem_type;
  typedef typename get_pod_type<elem_type>::result     pod_type;
  typedef SpSubview<eT>                                stored_type;

  typedef typename SpSubview<eT>::const_iterator       const_iterator_type;
  typedef typename SpSubview<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = true;
  static const bool Q_is_generated = false;

  static const bool is_row = false;
  static const bool is_col = false;

  arma_aligned const SpSubview<eT>& Q;

  inline explicit SpProxy(const SpSubview<eT>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    Q.m.sync();
    }

  arma_inline uword get_n_rows()    const { return Q.n_rows;    }
  arma_inline uword get_n_cols()    const { return Q.n_cols;    }
  arma_inline uword get_n_elem()    const { return Q.n_elem;    }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero; }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.m.values;      }
  arma_inline const uword* get_row_indices() const { return Q.m.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.m.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>& X) const { return (void_ptr(&Q.m) == void_ptr(&X)); }
  };



template<typename eT>
class SpProxy< spdiagview<eT> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpMat<eT>                                stored_type;

  typedef typename SpMat<eT>::const_iterator       const_iterator_type;
  typedef typename SpMat<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = true;

  static const bool is_row = false;
  static const bool is_col = true;

  arma_aligned const SpMat<eT> Q;

  inline explicit SpProxy(const spdiagview<eT>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline uword get_n_rows()    const { return Q.n_rows;    }
  arma_inline uword get_n_cols()    const { return 1;           }
  arma_inline uword get_n_elem()    const { return Q.n_elem;    }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero; }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>&) const { return false; }
  };



template<typename T1, typename spop_type>
class SpProxy< SpOp<T1, spop_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename T1::elem_type                   eT;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpMat<eT>                                stored_type;

  typedef typename SpMat<eT>::const_iterator       const_iterator_type;
  typedef typename SpMat<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = true;

  static const bool is_row = SpOp<T1, spop_type>::is_row;
  static const bool is_col = SpOp<T1, spop_type>::is_col;

  arma_aligned const SpMat<eT> Q;

  inline explicit SpProxy(const SpOp<T1, spop_type>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline uword get_n_rows()    const { return is_row ? 1 : Q.n_rows; }
  arma_inline uword get_n_cols()    const { return is_col ? 1 : Q.n_cols; }
  arma_inline uword get_n_elem()    const { return Q.n_elem;              }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero;           }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>&) const { return false; }
  };



template<typename T1, typename T2, typename spglue_type>
class SpProxy< SpGlue<T1, T2, spglue_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename T1::elem_type                   eT;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpMat<eT>                                stored_type;

  typedef typename SpMat<eT>::const_iterator       const_iterator_type;
  typedef typename SpMat<eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = true;

  static const bool is_row = SpGlue<T1, T2, spglue_type>::is_row;
  static const bool is_col = SpGlue<T1, T2, spglue_type>::is_col;

  arma_aligned const SpMat<eT> Q;

  inline explicit SpProxy(const SpGlue<T1, T2, spglue_type>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline uword get_n_rows()    const { return is_row ? 1 : Q.n_rows; }
  arma_inline uword get_n_cols()    const { return is_col ? 1 : Q.n_cols; }
  arma_inline uword get_n_elem()    const { return Q.n_elem;              }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero;           }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const eT*    get_values()      const { return Q.values;      }
  arma_inline const uword* get_row_indices() const { return Q.row_indices; }
  arma_inline const uword* get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>&) const { return false; }
  };



template<typename out_eT, typename T1, typename spop_type>
class SpProxy< mtSpOp<out_eT, T1, spop_type> >
  {
  public:

  typedef          out_eT                          elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef SpMat<out_eT>                            stored_type;

  typedef typename SpMat<out_eT>::const_iterator       const_iterator_type;
  typedef typename SpMat<out_eT>::const_row_iterator   const_row_iterator_type;

  static const bool use_iterator   = false;
  static const bool Q_is_generated = true;

  static const bool is_row = mtSpOp<out_eT, T1, spop_type>::is_row;
  static const bool is_col = mtSpOp<out_eT, T1, spop_type>::is_col;

  arma_aligned const SpMat<out_eT> Q;

  inline explicit SpProxy(const mtSpOp<out_eT, T1, spop_type>& A)
    : Q(A)
    {
    arma_extra_debug_sigprint();
    }

  arma_inline uword get_n_rows()    const { return is_row ? 1 : Q.n_rows; }
  arma_inline uword get_n_cols()    const { return is_col ? 1 : Q.n_cols; }
  arma_inline uword get_n_elem()    const { return Q.n_elem;              }
  arma_inline uword get_n_nonzero() const { return Q.n_nonzero;           }

  arma_inline elem_type operator[](const uword i)                    const { return Q[i];           }
  arma_inline elem_type at        (const uword row, const uword col) const { return Q.at(row, col); }

  arma_inline const out_eT* get_values()      const { return Q.values;      }
  arma_inline const uword*  get_row_indices() const { return Q.row_indices; }
  arma_inline const uword*  get_col_ptrs()    const { return Q.col_ptrs;    }

  arma_inline const_iterator_type     begin()                            const { return Q.begin();            }
  arma_inline const_iterator_type     begin_col(const uword col_num)     const { return Q.begin_col(col_num); }
  arma_inline const_row_iterator_type begin_row(const uword row_num = 0) const { return Q.begin_row(row_num); }

  arma_inline const_iterator_type     end()                        const { return Q.end();            }
  arma_inline const_row_iterator_type end_row()                    const { return Q.end_row();        }
  arma_inline const_row_iterator_type end_row(const uword row_num) const { return Q.end_row(row_num); }

  template<typename eT2>
  arma_inline bool is_alias(const SpMat<eT2>&) const { return false; }
  };



//! @}
