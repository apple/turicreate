//
//  CustomWebKitView.swift
//  Turi Create Visualization
//
//  Created by Abhishek Pratapa on 4/24/19.
//  Copyright © 2019 Apple. All rights reserved.
//

import Cocoa
import WebKit

class CustomWebKitView: WKWebView {
    override func performKeyEquivalent(with event: NSEvent) -> Bool {
        super.performKeyEquivalent(with: event)
        return true
    }
}
