//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import WebKit

class ViewController: NSViewController, NSWindowDelegate {
    
    @IBOutlet weak var webView: WKWebView!
    
    // load view
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    override func viewDidAppear() {
        self.view.window?.delegate = self
        self.view.layer?.backgroundColor = NSColor.white.cgColor
        self.view.window?.setContentSize(NSSize(width: 500, height: 500))
        var window_size = NSSize(width: 820, height: 630)
        
        let screenSize: NSRect = (NSScreen.main()?.frame)!
        
        if let width_index = CommandLine.arguments.index(of: "--width") {
            if(CommandLine.arguments.count > width_index + 1){
                let width_size = CommandLine.arguments[width_index + 1]
                if let int_width = Int(width_size){
                    if(CGFloat(int_width) < screenSize.width){
                        window_size.width = CGFloat(int_width)
                    }else{
                        window_size.width = screenSize.width
                    }
                }
            }
        }
        
        if let height_index = CommandLine.arguments.index(of: "--height") {
            if(CommandLine.arguments.count > height_index + 1){
                let height_size = CommandLine.arguments[height_index + 1]
                if let int_height = Int(height_size){
                    if(CGFloat(int_height) < screenSize.height){
                        window_size.height = CGFloat(int_height)
                    }else{
                        window_size.height = screenSize.height
                    }
                }
            }
        }
        
        self.view.window?.minSize.height = window_size.height
        self.view.window?.minSize.width = window_size.width
        
        self.view.window?.setContentSize(window_size)
        
        SharedData.shared.vegaContainer = VegaContainer(view: webView, window_handle: self.view.window!)
    }
    
    func windowShouldClose(_ sender: Any) -> Bool {
        NSApplication.shared().terminate(self)
        return true
    }

}



