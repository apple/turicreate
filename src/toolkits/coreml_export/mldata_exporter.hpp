/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_MLDATA_EXPORTER_HPP
#define COREML_MLDATA_EXPORTER_HPP

#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <ml/ml_data/metadata.hpp>

namespace turi {

/**
 *  Initiates a pipeline from an MLData metadata object so that it takes the input
 *  in the form of the input from the mldata, then outputs it as a final
 *  vector named __vectorized_features__ that can then be used by other algorithms.
 *  The pipeline is returned.  The input variables of the pipeline are the same
 *  as those given in the ml_data metadata object. The classifier or regressor
 *  needs to be added to the pipeline, along with the appropriate output variables.
 *
 */
void setup_pipeline_from_mldata(
    CoreML::Pipeline& pipeline,
    std::shared_ptr<ml_metadata> metadata);

}

#endif
