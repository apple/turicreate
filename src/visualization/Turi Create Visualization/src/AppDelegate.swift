//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {
    
    @IBOutlet weak var save_image_object: NSMenuItem!
    @IBOutlet weak var save_vega_object: NSMenuItem!
    @IBOutlet weak var print_vega_image: NSMenuItem!
    @IBOutlet weak var page_setup_object: NSMenuItem!
    
    
    @IBAction func print_vega(_ sender: Any) {
        SharedData.shared.vegaContainer?.get_image {image in
            let nsImage = NSImageView(image: image)
            nsImage.sizeToFit()
            nsImage.setFrameOrigin(NSPoint(x: 0.0, y: 0.0))
            nsImage.setFrameSize(NSSize(width: image.size.width, height: image.size.height))
            let print_info = NSPrintInfo()
        
            let print_image = NSPrintOperation(view: nsImage, printInfo: print_info)
            print_image.canSpawnSeparateThread = true
            print_image.printPanel.options.insert(NSPrintPanel.Options.showsOrientation)
            print_image.printPanel.options.insert(NSPrintPanel.Options.showsScaling)
            print_image.run()
        }
    }
    
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
        SharedData.shared.save_image = save_image_object
        SharedData.shared.save_vega = save_vega_object
        SharedData.shared.print_vega = print_vega_image
        SharedData.shared.page_setup = page_setup_object
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
}

