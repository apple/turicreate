#include "OneHotEncoder.hpp"
#include "../Format.hpp"

namespace CoreML {

    OneHotEncoder::OneHotEncoder(const std::string& description) :
    Model(description) {
        // make sure we become a one hot encoder
        (void) m_spec->mutable_onehotencoder();
    }

    Result OneHotEncoder::addInput(const std::string& inputName, FeatureType inputType) {

        HANDLE_RESULT_AND_RETURN_ON_ERROR(enforceTypeInvariant({
            FeatureType::Int64(),
            FeatureType::String(),
        }, inputType));

        HANDLE_RESULT_AND_RETURN_ON_ERROR(Model::addInput(inputName, inputType));
        return Result();
    }

    Result OneHotEncoder::setHandleUnknown(MLHandleUnknown state) {
        auto ohe = m_spec->mutable_onehotencoder();
        ohe->set_handleunknown(static_cast<Specification::OneHotEncoder_HandleUnknown>(state));

        return Result();
    }

    Result OneHotEncoder::setUseSparse(bool state) {
        auto ohe = m_spec->mutable_onehotencoder();
        ohe->set_outputsparse(state);
        return Result();
    }

    Result OneHotEncoder::setFeatureEncoding(const std::vector<int64_t>& container) {
        auto ohe = m_spec->mutable_onehotencoder();
        ohe->clear_int64categories();

        for (auto element : container) {
            ohe->mutable_int64categories()->add_vector(element);
        }
        return Result();
    }

    Result OneHotEncoder::setFeatureEncoding(const std::vector<std::string>& container) {
        auto ohe = m_spec->mutable_onehotencoder();
        ohe->clear_stringcategories();

        for (auto element : container) {
            std::string *value = ohe->mutable_stringcategories()->add_vector();
            *value = element;
        }
        return Result();
    }

    Result OneHotEncoder::getHandleUnknown(MLHandleUnknown *state) {
        if (state != nullptr) {
            auto ohe = m_spec->mutable_onehotencoder();
            *state = static_cast<MLHandleUnknown>(ohe->handleunknown());
        }
        return Result();
    }

    Result OneHotEncoder::getUseSparse(bool *state) {
        if (state != nullptr) {
            auto ohe = m_spec->mutable_onehotencoder();
            *state = ohe->outputsparse();
        }
        return Result();
    }

    Result OneHotEncoder::getFeatureEncoding(std::vector<int64_t>& container) {
        auto *ohe = m_spec->mutable_onehotencoder();

        for (int i = 0; i < ohe->int64categories().vector_size(); i++) {
            container.push_back(ohe->int64categories().vector(i));
        }
        return Result();
    }

    Result OneHotEncoder::getFeatureEncoding(std::vector<std::string>& container) {
        auto *ohe = m_spec->mutable_onehotencoder();

        for (int i = 0; i < ohe->stringcategories().vector_size(); i++) {
            container.push_back(ohe->stringcategories().vector(i));
        }
        return Result();
    }

}
