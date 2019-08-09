//  Copyright © 2019 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import Cocoa

let delegate = AppDelegate()
NSApplication.shared.setActivationPolicy(.regular)
NSApplication.shared.delegate = delegate
DispatchQueue.main.async {
    NSApplication.shared.activate(ignoringOtherApps: true)
}
NSApplication.shared.run()
