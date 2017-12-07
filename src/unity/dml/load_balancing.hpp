/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GENERALIZED_LOAD_BALANCING
#define TURI_GENERALIZED_LOAD_BALANCING
#include <numerics/armadillo.hpp>
#include <util/cityhash_tc.hpp>

namespace turi {

namespace distributed_sgraph_compute {

  /**
   * Given a binary m by n matrix M, representing n jobs
   * being processed by m processers, and job j can only be
   * processed by i's where M[i,j] == 1
   *
   * The solver *approximately
   * solves" the following optimization problem:
   *
   * Let M' be an m by n solution matrix:
   * - M'.col(j) has one and only one non-zero entry. // each job processed
   *                                                     once and only once
   * - If M'[i, j] == 1, then M[i, j] == 1 // processor i is in the constraint
   *                                          set
   * - L(i) = sum(M'.row(i)) is the load of i
   * - Minimize the largest load: max_i L(i)
   */

  /**
   * Heuristics:
   * First we try to assign job j to machine i == j % numprocs, if i is in the constraint set.
   * Then for the remanining partitions, we assign randomly.
   *
   * Returns {assignemnt[job_id], max_load}
   */
  inline std::pair<std::vector<size_t>, double> solve_genearlized_load_balancing(
      const EIGTODO::SparseMatrix<size_t>& constraint_mat) {
    size_t nprocs = constraint_mat.n_rows;
    size_t njobs  = constraint_mat.n_cols;

    std::vector<size_t> assignment(njobs, (size_t)(-1));
    std::vector<size_t> load(nprocs, 0);
    size_t max_load;

    // Assign j to j % proc if feasible.
    for (size_t j = 0; j < njobs; ++j) {
      size_t i = j % nprocs;
      if (constraint_mat.coeff(i,j) == 1) {
        assignment[j] = i;
        load[i]++;
        break;
      }
    }

    // Assign j to hash(j) % constraint_set(j)
    for (size_t j = 0; j < njobs; ++j) {
      if (assignment[j] == (size_t)(-1)) {
        size_t nnz = constraint_mat.col(j).nonZeros();
        size_t rand = turi::hash64(j) % nnz;
        for (size_t i = 0; i < nprocs; ++i) {
          if (constraint_mat.coeff(i, j) == 1) {
            if (rand == 0) {
              assignment[j] = i;
              load[i]++;
              break;
            }
            rand--;
          }
        }
      }
    }

    max_load = *std::max_element(load.begin(), load.end());

    // Sanity check
#ifndef NDEBUG
    for (size_t j = 0; j < assignment.size(); ++j) {
      DASSERT_LT(assignment[j], nprocs);
      size_t val = constraint_mat.coeff(assignment[j], j);
      DASSERT_EQ(val, 1);
    }
#endif
    return {assignment, (double)max_load};
  }

} // end of distributed sgraph compute
} // end of turicreate

#endif
