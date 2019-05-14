//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import Cocoa

class AppDelegate: NSObject, NSApplicationDelegate {
    
    var newWindow: NSWindow?
    var controller: ViewController?
    
    @objc func print_vega(_ sender: Any) {
        SharedData.shared.vegaContainer?.get_image {optionalImage in
            switch optionalImage {
            case .some(let image):
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
                
            case .none:
                return
            }
        }
    }
    
    @objc func save_image(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_image()
    }
    
    @objc func save_vega(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_vega()
    }
    
    func save_data(_ sender: Any) {
        SharedData.shared.vegaContainer?.save_data()
    }
    
    
    func applicationDidFinishLaunching(_ aNotification: Notification) {
        let contentFrame = NSMakeRect(0, 0, 820, 630)
        
        newWindow = NSWindow(contentRect: contentFrame, styleMask: [.resizable, .titled, .closable, .miniaturizable], backing: .buffered, defer: false)
        newWindow?.center()
        newWindow?.minSize = NSMakeSize(600, 400)
        
        controller = ViewController()
        let content = newWindow!.contentView! as NSView
        let view = controller!.view
        content.frame = contentFrame
        view.frame = contentFrame
        content.addSubview(view)
        view.autoresizingMask = [.width, .height]
        
        let mainMenu = NSMenu()
        let appMenuItem = mainMenu.addItem(withTitle: "Turi Create Visualization", action: nil, keyEquivalent: "")
        let appMenu = NSMenu(title: "Turi Create Visualization")
        appMenuItem.submenu = appMenu
        
        let save_image_object = NSMenuItem(title: "Export as PNG", action: #selector(self.save_image), keyEquivalent: "S")
        save_image_object.keyEquivalentModifierMask = .command
        
        let save_vega_object = NSMenuItem(title: "Export as Vega JSON", action: #selector(self.save_vega), keyEquivalent: "S")
        save_vega_object.keyEquivalentModifierMask = [.command, .shift]
        
        let print_vega_image = NSMenuItem(title: "Print...", action: #selector(self.print_vega), keyEquivalent: "P")
        print_vega_image.keyEquivalentModifierMask = .command
        
        let page_setup_object = NSMenuItem(title: "Page Setup...", action: #selector(self.print_vega), keyEquivalent: "P")
        page_setup_object.keyEquivalentModifierMask = [.command, .shift]
        
        let hideItem = appMenu.addItem(withTitle: "Hide Turi Create Visualization", action: #selector(NSApplication.shared.hide), keyEquivalent: "h")
        hideItem.keyEquivalentModifierMask = .command
        let hideOthersItem = appMenu.addItem(withTitle: "Hide Others", action: #selector(NSApplication.shared.hideOtherApplications), keyEquivalent: "h")
        hideOthersItem.keyEquivalentModifierMask = [.command, .option]
        
        appMenu.addItem(.separator())
        
        let quitItem = appMenu.addItem(withTitle: "Quit Turi Create Visualization", action: #selector(NSApplication.shared.terminate), keyEquivalent: "q")
        quitItem.keyEquivalentModifierMask = .command
        
        let fileMenu = NSMenu(title: "File")
        let closeItem = fileMenu.addItem(withTitle: "Close", action: #selector(newWindow?.close), keyEquivalent: "w")
        closeItem.keyEquivalentModifierMask = .command
        fileMenu.addItem(save_image_object)
        fileMenu.addItem(save_vega_object)
        fileMenu.addItem(.separator())
        fileMenu.addItem(page_setup_object)
        fileMenu.addItem(print_vega_image)
        
        let fileMenuItem = mainMenu.addItem(withTitle: "File", action: nil, keyEquivalent: "")
        fileMenuItem.submenu = fileMenu
        
        let viewMenu = NSMenu(title: "View")
        let fullScreenItem = viewMenu.addItem(withTitle: "Toggle Full Screen", action: #selector(newWindow?.toggleFullScreen), keyEquivalent: "f")
        fullScreenItem.keyEquivalentModifierMask = [.command, .control]
        
        let viewMenuItem = mainMenu.addItem(withTitle: "View", action: nil, keyEquivalent: "")
        viewMenuItem.submenu = viewMenu
        
        let windowMenu = NSMenu(title: "Window")
        let minimizeItem = windowMenu.addItem(withTitle: "Minimize", action: #selector(newWindow?.miniaturize), keyEquivalent: "m")
        minimizeItem.keyEquivalentModifierMask = .command
        windowMenu.addItem(withTitle: "Zoom", action: #selector(newWindow?.zoom), keyEquivalent: "")
        
        let windowMenuItem = mainMenu.addItem(withTitle: "Window", action: nil, keyEquivalent: "")
        windowMenuItem.submenu = windowMenu
        
        SharedData.shared.save_image = save_image_object
        SharedData.shared.save_vega = save_vega_object
        SharedData.shared.print_vega = print_vega_image
        SharedData.shared.page_setup = page_setup_object
        
        NSApplication.shared.mainMenu = mainMenu
        
        newWindow!.makeKeyAndOrderFront(nil)
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }
}

