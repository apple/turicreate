//
//  DictVectorizer.cpp
//  libmlmodelspec
//
//  Created by Hoyt Koepke on 2/7/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include <stdio.h>
#include "DictVectorizer.hpp"
#include "../Format.hpp"

namespace CoreML {

    DictVectorizer::DictVectorizer(const std::string& description) :
    Model(description) {
            // make sure we become a dict vectorizer
        (void) m_spec->mutable_dictvectorizer();
    }

    Result DictVectorizer::addInput(const std::string& inputName, FeatureType inputType) {

        HANDLE_RESULT_AND_RETURN_ON_ERROR(enforceTypeInvariant({
            FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_stringKeyType),
            FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType)
        }, inputType));

        HANDLE_RESULT_AND_RETURN_ON_ERROR(Model::addInput(inputName, inputType));
        return Result();
    }


    Result DictVectorizer::setFeatureEncoding(const std::vector<int64_t>& container) {
        auto ohe = m_spec->mutable_dictvectorizer();
        ohe->mutable_int64toindex()->clear_vector();

        for (auto element : container) {
            ohe->mutable_int64toindex()->add_vector(element);
        }
        return Result();
    }

    Result DictVectorizer::setFeatureEncoding(const std::vector<std::string>& container) {
        auto ohe = m_spec->mutable_dictvectorizer();
        ohe->mutable_stringtoindex()->clear_vector();

        for (auto element : container) {
            auto *value = ohe->mutable_stringtoindex()->add_vector();
            *value = element;
        }
        return Result();
    }

}
