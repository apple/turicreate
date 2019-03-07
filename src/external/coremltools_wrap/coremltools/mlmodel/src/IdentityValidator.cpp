//
//  IdentityValidator.cpp
//  mlmodelspec
//
//  Created by Zachary Nation on 4/20/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_identity>(const Specification::Model&) {
        // all identities are valid
        return Result();
    }

}
