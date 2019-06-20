//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

// make utilities file

import Foundation

public class JSON {
    private static func stringify(anything: Any) throws -> String {
        let objectData = try JSONSerialization.data(withJSONObject: anything, options: JSONSerialization.WritingOptions(rawValue: 0))
        guard let objectString = String(data: objectData, encoding: .utf8) else {
            throw VisualizationError.JSONSerializationError
        }
        return objectString
    }

    public static func stringify(obj: [String: Any]) throws -> String {
        return try self.stringify(anything:obj)
    }

    public static func stringify(arr: [Any]) throws -> String {
        return try self.stringify(anything:arr)
    }

    public static func parse(str: String) throws -> Any {
        guard let data = str.data(using: String.Encoding.utf8, allowLossyConversion: false) else {
            throw VisualizationError.JSONSerializationError
        }
        let obj_returned = try JSONSerialization.jsonObject(with: data, options: JSONSerialization.ReadingOptions.init(rawValue: 0));
        return obj_returned;
    }
}
