//
//  FeatureVectorizer.cpp
//  libmlmodelspec
//
//  Created by Hoyt Koepke on 11/24/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "FeatureVectorizer.hpp"
#include "../Format.hpp"


namespace CoreML {

  FeatureVectorizer::FeatureVectorizer(const std::string& description) :
  Model(description) { }


  FeatureVectorizer::~FeatureVectorizer() = default;

  FeatureVectorizer::FeatureVectorizer(const Specification::Model &modelSpec) {
    m_spec = std::make_shared<Specification::Model>(modelSpec);
  }

  Result FeatureVectorizer::add(const std::string& input_feature, size_t input_dimension) {

    auto p = m_spec->mutable_featurevectorizer();
    auto container = p->mutable_inputlist();

    auto c = new Specification::FeatureVectorizer_InputColumn;
    c->set_inputcolumn(input_feature);
    c->set_inputdimensions(input_dimension);

    // Already allocated.
    container->AddAllocated(c);
    return Result();
  }

  std::vector<std::pair<std::string, size_t> > FeatureVectorizer::get_inputs() const {

    auto p = m_spec->featurevectorizer();
    auto container = p.inputlist();

    std::vector<std::pair<std::string, size_t> > out(static_cast<size_t>(container.size()));

    for(int i = 0; i < container.size(); ++i) {
      out[static_cast<size_t>(i)] = {container[i].inputcolumn(), container[i].inputdimensions()};
    }

    return out;
  }
}
