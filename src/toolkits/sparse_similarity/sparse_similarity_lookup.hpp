/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_ITEM_SIMILARITY_LOOKUP_H_
#define TURI_UNITY_ITEM_SIMILARITY_LOOKUP_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/extensions/option_manager.hpp>

namespace turi {

class sframe;
template <typename T> class sarray;

/** A model that can be used for sparse similarity lookup.
 *
 *  A trained version of this model contains a lookup table of the
 *  nearest items to each item along with a similarity score.  This
 *  allows both retrieval of the most similar items to a given item,
 *  and to generate a list of the items most similar to a collection
 *  of items.  The latter is used to recommend items, e.g. by
 *  providing a list of the items and ratings for a particular user.
 *
 *  The similarity metrics are an implementation of a class that
 *  implements a number of methods dictating the math used in the
 *  accumulation.  See similarities.hpp for details.
 *
 *  This model is creating using the create(...) methods below, which
 *  takes the name of the similarity and the current options.  The
 *  options given in add_options(...) must be present in the options
 *  map.
 *
 *  The model can be trained by either providing the similarities of
 *  the items directly, or by training the model on a sarray of
 *  user-item-ratings.  See the below functions for details.
 *
 *
 *  Code Structure:
 *
 * - This model is intended to be encapsulated by other user-facing
 *   models such as item similarity.  In this case, item similarity
 *   provides the user facing API, creates this model and then uses
 *   it.  Some item cf specific features, like how to handle new
 *   users, are punted to that model -- this one only can be queried
 *   with a list of items and ratings which then produce the output.
 *
 * - The similarity class defines the metric used, and then how the
 *   averaging at prediction time is done.
 *
 * - The similarity class is given as a template to the implementation
 *   part of this class, sparse_similarity_lookup_impl, which inherits
 *   from this one.  This class does the training (if it's not farmed
 *   out to different helper functions), and takes care of the
 *   prediction and item scoring.  Basically, the core of the
 *   algorithm is in sparse_similarity_lookup_impl.hpp.
 *
 * - A number of accompaning utilities -- for example, nearest
 *   neighbors functions, utilities to generate the per-item
 *   statistics, and
 *
 * - A dense matrix class that stores only the upper diaganol part of
 *   a matrix is provided in sliced_itemitem_matrix.hpp. Included in
 *   that header are tools for estimating the number of row-slices and
 *   passes through the data to make given constraints on the memory
 *   usage.
 *
 */
class sparse_similarity_lookup {
 public:

  virtual ~sparse_similarity_lookup() = default;

  /** Returns the name of the similarity this version uses.
   */
  virtual std::string similarity_name() const = 0;

  /** Adds in all of the options needed for this class to the option
   *  manager.
   */
  static void add_options(option_manager& options);

  /**  Factory method: Call this to create or load a model from one of
   *   the existing similarities by name.
   */
  static std::shared_ptr<sparse_similarity_lookup>
  create(const std::string& similarity, const std::map<std::string, flexible_type>& options);

  /** Trains the model from an sarray of vectors of (index, score)
   *  pairs.  Each row is assumed to be the user, and each index in
   *  the score is an item that the user rated.  The similarity of a
   *  given item to another item is given by treating the user ratings
   *  of each item as a sparse vector and then measuring the
   *  similarity between them.  This calculation is done as
   *  efficiently as possible using a combination of nearest neighbors
   *  search and lookup tables.
   */
  virtual std::map<std::string, flexible_type>
  train_from_sparse_matrix_sarray(
      size_t num_items,
      const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data) = 0;

  /** Sets the lookup tables directly from an sframe of interaction
   *  data.  The interaction data is an sframe containing columns
   *  item_column, similar_item_column, and similarity.  The items and
   *  similar items must be indices in {0, ..., num_items-1}.
   *
   *  An error is raised if the similarity value does not conform to
   *  the similarity chosen -- e.g. jaccard similarity must be between
   *  0 and 1.
   *
   *  If add_reverse is true, then all (i,j, rating) entries are also
   *  interpreted as (j, i, rating).  Note that no duplicates can be
   *  present.
   */
  virtual void setup_by_raw_similarity(
      size_t num_items,
      const flex_list& item_data,
      const sframe& _interaction_data,
      const std::string& item_column,
      const std::string& similar_item_column,
      const std::string& similarity,
      bool add_reverse = false) = 0;


  ////////////////////////////////////////////////////////////////////////////////
  // Prediction and item scoring.

  /**  Score all items in a list of item predictions given a list of
   *   user-item interactions.  This method fills in the second part
   *   of the tuple for all the items in the item_predictions
   *   container.
   *
   *   This is also the way to generate values for predict --
   *
   *   Returns the number of item similarity pairs that were
   *   considered.  (In some corner cases, such as when an item had no
   *   users that also rated other items, we want to recommend by
   *   popularity or some other metric).
   */
  virtual size_t score_items(std::vector<std::pair<size_t, double> >& item_predictions,
                             const std::vector<std::pair<size_t, double> >& user_item_data) const = 0;

  /** Fills an array with the most similar items to a given item.
   *  This is read directly from the lookup table.  If fewer than
   *  top_k items are present in the lookup table, then only those in
   *  the lookup table are returned.
   */
  virtual void get_similar_items(
      std::vector<std::pair<size_t, flexible_type> >& similar_items_dest,
      size_t item, size_t top_k) const = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // Routines for loading and serialization.

  virtual size_t get_version() const = 0;

  /** Serialization in sparse_similarity_lookup_impl.
   */
  virtual void save(turi::oarchive& oarc) const = 0;
  virtual void load(turi::iarchive& iarc) = 0;

  /** The current options.
   */
  const std::map<std::string, flexible_type>& current_options() const {
    return options;
  }

  /** A method to detect if two sparse_similarity_lookup classes are
   *  essentially the same.
   *
   */
  virtual bool _debug_check_equal(const sparse_similarity_lookup& other) const = 0;

 protected:

  /**  The stored options.
   */
  std::map<std::string, flexible_type> options;

};

}

BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<sparse_similarity_lookup>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    static const size_t verification_number = 0x36fe3812b00eddb0ULL;

    // Save the similarity name
    arc << m->similarity_name();

    // Save the options
    arc << m->current_options();

    // Save the model.
    m->save(arc);

    arc << verification_number;

  }

} END_OUT_OF_PLACE_SAVE()

BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<sparse_similarity_lookup>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    static const size_t verification_number = 0x36fe3812b00eddb0ULL;

    std::string similarity_name;
    arc >> similarity_name;

    std::map<std::string, flexible_type> options;
    arc >> options;

    std::shared_ptr<sparse_similarity_lookup> new_m
        = sparse_similarity_lookup::create(similarity_name, options);

    new_m->load(arc);

    size_t test_verification_number = 0;
    arc >> test_verification_number;

    ASSERT_MSG(test_verification_number == verification_number,
               "Unknown error loading similarity model.");

    m = new_m;

  } else {
    m = std::shared_ptr<sparse_similarity_lookup>();
  }
} END_OUT_OF_PLACE_LOAD()

#endif /* _ITEM_SIMILARITY_LOOKUP_H_ */
