//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import WebKit

class ViewController: NSViewController, NSWindowDelegate, WKUIDelegate {
    var customWebView: CustomWebKitView!
    
    // load view
    override func loadView() {
        let webConfiguration = WKWebViewConfiguration()
        customWebView = CustomWebKitView(frame: .zero, configuration: webConfiguration)
        customWebView.uiDelegate = self
        view = customWebView
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // create graph object
        SharedData.shared.vegaContainer = VegaContainer(view: customWebView)
    }
    
    override func viewDidAppear() {
        self.view.window?.delegate = self
        self.view.layer?.backgroundColor = NSColor.white.cgColor
    }
    
    func windowShouldClose(_ sender: NSWindow) -> Bool {
        NSApplication.shared.terminate(self)
        return true
    }

}



