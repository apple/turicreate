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
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wenum-compare-switch"

#include "Model.hpp"
#include "Validation/Validators.hpp"
#include "Utils.hpp"

#include "transforms/Pipeline.hpp"
#include "transforms/LinearModel.hpp"
#include "transforms/TreeEnsemble.hpp"
#include "transforms/DictVectorizer.hpp"
#include "transforms/NeuralNetwork.hpp"
#include "transforms/OneHotEncoder.hpp"
#include "transforms/FeatureVectorizer.hpp"
#include "transforms/TreeEnsemble.hpp"
#include "TreeEnsembleCommon.hpp"

#include "../build/format/Model.pb.h"

#pragma clang diagnostic pop

#endif /* MLModelification_h */
