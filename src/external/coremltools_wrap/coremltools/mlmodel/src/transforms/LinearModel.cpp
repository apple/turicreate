#include "LinearModel.hpp"
#include "../Format.hpp"

namespace CoreML {

    LinearModel::LinearModel(const std::string& predictedValueOutput,
                             const std::string& description)
    : Model(description) {
        m_spec->mutable_description()->set_predictedfeaturename(predictedValueOutput);
    }

    LinearModel::LinearModel(const Model &modelSpec) : Model(modelSpec) {
    }

    Result LinearModel::setOffsets(std::vector<double> offsets) {
        auto lr = m_spec->mutable_glmregressor();
        for(double n : offsets) {
            lr->add_offset(n);
        }
        return Result();
    }

    std::vector<double> LinearModel::getOffsets() {
        std::vector<double> result;
        auto lr = m_spec->mutable_glmregressor();
        for(double n : lr->offset()) {
            result.push_back(n);
        }
        return result;
    }

    Result LinearModel::setWeights(std::vector< std::vector<double>> weights) {
        auto lr = m_spec->mutable_glmregressor();
        for(auto w : weights) {
            Specification::GLMRegressor::DoubleArray* cur_arr = lr->add_weights();
            for(double n : w) {
                cur_arr->add_value(n);
            }
        }
        return Result();
    }

    std::vector< std::vector<double>> LinearModel::getWeights() {
        std::vector< std::vector<double>> result;
        auto lr = m_spec->mutable_glmregressor();
        for(auto v : lr->weights()) {
            std::vector<double> cur;
            for(double n : v.value()) {
                cur.push_back(n);
            }
            result.push_back(cur);
        }
        return result;
    }

}
