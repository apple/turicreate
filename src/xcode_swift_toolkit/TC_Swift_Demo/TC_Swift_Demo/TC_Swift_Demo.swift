//
//  SwiftMLClass.swift
//  TC_Swift_Demo
//
//  Created by Hoyt Koepke on 12/23/19.
//  Copyright © 2019 Hoyt Koepke. All rights reserved.
//

import Foundation


@objc public class SwiftMLClass : NSObject {
    var s: String

    public override init() { // Constructor
        self.s = ""
         super.init()
    }
    
    @objc public func append_string(x: String) {
        self.s += x
        print("New string: \(self.s)")
    }
    
    @objc public func get_string() -> String {
        return self.s
    }
}

