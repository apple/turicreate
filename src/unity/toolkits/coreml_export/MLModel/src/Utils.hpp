/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef MLMODEL_UTILS
#define MLMODEL_UTILS

#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Globals.hpp"
#include "Model.hpp"
#include "Result.hpp"
#include "Format.hpp"

namespace CoreML {

    // insert_or_assign not available until C++17
    template<typename K, typename V>
    inline void insert_or_assign(std::unordered_map<K, V>& map, const K& k, const V& v) {
        const auto result = map.find(k);
        if (result == map.end()) {
            // insert case
            map.insert(std::make_pair(k, v));
        } else {
            // assign case
            V& existing = map.at(k);
            existing = v;
        }
    }
   
    template <typename T>
    static inline Result saveSpecification(const T& formatObj,
                                           std::ostream& out) {
        google::protobuf::io::OstreamOutputStream rawOutput(&out);

        if (!formatObj.SerializeToZeroCopyStream(&rawOutput)) {
            return Result(ResultType::FAILED_TO_SERIALIZE,
                          "unable to serialize object");
        }

        return Result();
    }


    static inline Result saveSpecificationPath(const Specification::Model& formatObj,
                                               const std::string& path) {
        Model m(formatObj);
        return m.save(path);
    }

    template <typename T>
    static inline  Result loadSpecification(T& formatObj,
                                            std::istream& in) {

        google::protobuf::io::IstreamInputStream rawInput(&in);
        google::protobuf::io::CodedInputStream codedInput(&rawInput);

        codedInput.SetTotalBytesLimit(std::numeric_limits<int>::max(), -1);

        if (!formatObj.ParseFromCodedStream(&codedInput)) {
            return Result(ResultType::FAILED_TO_DESERIALIZE,
                          "unable to deserialize object");
        }
        
        return Result();
    }
    
    static inline Result loadSpecificationPath(Specification::Model& formatObj,
                                               const std::string& path) {
        Model m;
        Result r = CoreML::Model::load(path, m);
        if (!r.good()) { return r; }
        formatObj = m.getProto();
        return Result();
    }

}
#endif
