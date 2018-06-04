#include "MLModelTests.hpp"

// TODO -- Fix these headers.
#include "../src/Model.hpp"
#include "../src/transforms/OneHotEncoder.hpp"
#include "../src/transforms/LinearModel.hpp"
#include "../src/transforms/TreeEnsemble.hpp"


#include "framework/TestUtils.hpp"
#include <cassert>
#include <cstdio>


typedef CoreML::TreeEnsembleBase::BranchMode BranchMode;

int testTreeEnsembleBasic() {
    CoreML::TreeEnsembleRegressor tr("z", "");
    tr.setDefaultPredictionValue(0.0);
    tr.setupBranchNode(0, 0, 1, BranchMode::BranchOnValueGreaterThan, 5, 1, 2);
    tr.setupLeafNode(0, 1, 1);
    tr.setupLeafNode(0, 2, 2);

    ML_ASSERT_GOOD(tr.addInput("x", CoreML::FeatureType::Double()));
    ML_ASSERT_GOOD(tr.addInput("y", CoreML::FeatureType::Double()));
    ML_ASSERT_GOOD(tr.addOutput("z", CoreML::FeatureType::Double()));
    ML_ASSERT_EQ(tr.modelType(), MLModelType_treeEnsembleRegressor);

    CoreML::SchemaType expectedInputSchema {
        {"x", CoreML::FeatureType::Double()},
        {"y", CoreML::FeatureType::Double()},
    };

    CoreML::SchemaType expectedOutputSchema {
        {"z", CoreML::FeatureType::Double()},
    };

    ML_ASSERT_EQ(tr.inputSchema(), expectedInputSchema);
    ML_ASSERT_EQ(tr.outputSchema(), expectedOutputSchema);

    // TODO -- don't leave stuff in /tmp
    ML_ASSERT_GOOD(tr.save("/tmp/tA-tree.mlmodel"));

    CoreML::Model loadedA;
    ML_ASSERT_GOOD(CoreML::Model::load("/tmp/tA-tree.mlmodel", loadedA));

    ML_ASSERT_EQ(loadedA.modelType(), MLModelType_treeEnsembleRegressor);

    ML_ASSERT_EQ(loadedA.inputSchema(), expectedInputSchema);
    ML_ASSERT_EQ(loadedA.outputSchema(), expectedOutputSchema);
    return 0;
}
