#include <toolkits/supervised_learning/classifier_evaluations.hpp>

namespace turi {
namespace supervised {


// Returns a table by class that displays accuracy statistics for each class.

gl_sframe classifier_report_by_class(gl_sframe input, const std::string& actual,
                                     const std::string& predicted) {

  // Set up a struct to handle the updates from the iterator below.
  struct class_info {
    atomic<size_t> predicted_correctly{0};
    atomic<size_t> predicted_this_but_incorrect{0};
    atomic<size_t> actual_this_but_prediction_incorrect{0};
  };

  gl_sarray all_classes =
      input[actual].append(input[predicted]).unique().sort();

  std::unordered_map<flexible_type, class_info> class_lookup;

  for (const auto& v : all_classes.range_iterator()) {
    class_lookup[v] = class_info();
  }

  // Get the columns
  input.select_columns({predicted, actual})
      .materialize_to_callback([&](size_t,
                                   const std::shared_ptr<sframe_rows>& rows) -> bool {
        for (const auto& row : *rows) {
          const auto& predicted = row[0];
          const auto& actual = row[1];

          if (predicted == actual) {
            ++(class_lookup.at(predicted).predicted_correctly);
          } else {
            ++(class_lookup.at(predicted).predicted_this_but_incorrect);
            ++(class_lookup.at(actual).actual_this_but_prediction_incorrect);
          }
        }

        // Tell the materialize_to_callback iteration that we aren't yet done.
        return false;
      });

  // Now, go through and make an sframe with all these values, along
  // with the predicted correctness statistics and such.
  //
  gl_sframe report =
      all_classes
          .apply([&](const flexible_type& cl) -> flexible_type {

            const auto& info = class_lookup.at(cl);

            flex_dict ret(
                {{"class", cl},
                 {"actual_count", flex_int(info.predicted_correctly +
                             info.actual_this_but_prediction_incorrect)},
                 {"predicted_correctly", flex_int(info.predicted_correctly)},
                 {"predicted_this_incorrectly",
                  flex_int(info.predicted_this_but_incorrect)},
                 {"missed_predicting_this",
                  flex_int(info.actual_this_but_prediction_incorrect)},
                 {"precision", double(info.predicted_correctly) /
                                   double(info.predicted_correctly +
                                          info.predicted_this_but_incorrect)},
                 {"recall",
                  double(info.predicted_correctly) /
                      double(info.predicted_correctly +
                             info.actual_this_but_prediction_incorrect)}});

            return ret;
          }, flex_type_enum::DICT)
          .unpack("");

  return report;
}

// Returns a confusion matrix displaying a table of the actual classes and the
// predicted classes.
gl_sframe confusion_matrix(gl_sframe data, const std::string& actual,
                           const std::string& predicted) {
  return data.groupby({actual, predicted}, {{"count", aggregate::COUNT()}})
      .sort({actual, predicted});
}

}  // namespace supervised_learning

}  // namespace turi
