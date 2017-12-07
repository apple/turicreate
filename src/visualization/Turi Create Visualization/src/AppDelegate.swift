//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    
    @IBAction func save_image(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_image()
    }
    
    @IBAction func save_vega(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_vega()
    }
    
    @IBAction func save_data(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_data()
    }
    
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {

    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
}

