//
//  MLModelSpecification.hpp
//  CoreML
//
//  Project header for including libmlmodelspec along with protobuf definitions
//
//  Created by Michael Siracusa on 12/7/16.
//  Copyright © 2016 Apple Inc. All rights reserved.
//

#ifndef MLModelification_h
#define MLModelification_h

// Protobuf generated code has lots of issues we don't want to globally ignore
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wextended-offsetof"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wweak-vtables"

#include "mlmodel/src/Model.hpp"
#include "mlmodel/src/Validators.hpp"
#include "mlmodel/src/Utils.hpp"

#include "mlmodel/src/transforms/Pipeline.hpp"
#include "mlmodel/src/transforms/LinearModel.hpp"
#include "mlmodel/src/transforms/TreeEnsemble.hpp"
#include "mlmodel/src/transforms/DictVectorizer.hpp"
#include "mlmodel/src/transforms/NeuralNetwork.hpp"
#include "mlmodel/src/transforms/OneHotEncoder.hpp"
#include "mlmodel/src/transforms/FeatureVectorizer.hpp"
#include "mlmodel/src/transforms/TreeEnsemble.hpp"
#include "mlmodel/src/TreeEnsembleCommon.hpp"

#include "mlmodel/build/format/Model.pb.h"

#pragma clang diagnostic pop

#endif /* MLModelification_h */
