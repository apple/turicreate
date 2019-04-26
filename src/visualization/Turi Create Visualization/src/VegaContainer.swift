//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import WebKit

func log(_ message: String) {
    let withNewline = String(format: "%@\n", message)
    guard let data = withNewline.data(using: .utf8) else {
        assert(false)
        return
    }
    FileHandle.standardError.write(data)
    fflush(__stderrp)
}

func debug_log(_ message: String) {
    if let _ = ProcessInfo.processInfo.environment["TC_VISUALIZATION_CLIENT_ENABLE_DEBUG_LOGGING"] {
        log("DEBUG: " + message + "\n")
    }
}

class VegaContainer: NSObject, WKScriptMessageHandler {
    
    public var vega_spec: [String: Any]?
    public var evaluation_spec: [String: Any]?
    public var data_spec: [[String: Any]] = []
    public var image_spec: [[String: Any]] = []
    public var table_spec: [String: Any]?
    public var view: CustomWebKitView
    public var pipe: Pipe?
    private var loaded: Bool = false
    private var ready: Bool = false
    
    init(view: CustomWebKitView) {
        
        // initialize variables
        self.view = view;
        self.pipe = nil
        
        // super init call
        super.init()
        
        // start the pipe
        self.pipe = Pipe(graph_data: self)
        
        // load app bundle
        let appBundle = Bundle.main
        let htmlPath = appBundle.url(forResource: "index", withExtension: "html", subdirectory: "build")
        self.view.loadFileURL(htmlPath!, allowingReadAccessTo: appBundle.bundleURL)
        self.view.configuration.userContentController.add(self, name: "scriptHandler")
    }
    
    // callback from the javascript
    public func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage) {
        
        guard let messageBody = message.body as? [String: Any] else {
            assert(false)
            return
        }
        guard let status = messageBody["status"] as? String else {
            assert(false)
            return
        }
        
        switch status {
            
        case "loaded":
            debug_log("got loaded event")
            assert(!self.loaded)
            assert(!self.ready)
            self.loaded = true
            self.send_spec()
            break
            
        case "ready":
            debug_log("got ready event")
            assert(self.loaded)
            assert(!self.ready)
            self.ready = true
            self.send_data()
            break
        
        case "log":
            guard let level = messageBody["level"] as? String else {
                assert(false)
                return
            }
            guard let logMessage = messageBody["message"] as? String else {
                assert(false)
                return
            }
            switch level {
            case "debug":
                #if DEBUG
                // Only log "debug" messages in DEBUG builds
                log(logMessage)
                #endif
                break
                
            case "log": fallthrough
            case "info": fallthrough
            case "warn":
                // TODO: Change if vega tooltips have log level options
                // Until we are able to change the log level of the tooltip, we'll have to supress warnings of the Tooltip
                /*
                     This is because the way we are rendering summary view with one spec allows us to use our summary view design as a data_representation at each container. This means that to the tooltips we just generated a different type of plot, a plot that contains "bars" that are summary view containers. Once we define the tooltips and specified data points aren't found in the current selection, tooltips throws a warning saying no tooltip information found, and doesn't display the tooltip. Until we can supress warnings from Tooltip we'll have to ignore the logMessages.
                 */
                if logMessage.range(of:"[Tooltip]") == nil {
                    log(logMessage)
                }
                break
                
            case "error":
                log(logMessage)
                assert(false, "Encountered an unhandled JavaScript error.")
                break
                
            default:
                log(logMessage)
                assert(false, "Unexpected log level specified.")
                break;
            }
            break
            
        case "print_message":
            guard let logMessage = messageBody["message"] as? String else {
                assert(false, "Expected a message provided in print_message")
                return
            }
            
            log(logMessage)
            break;
            
        case "getRows":
            guard let start_num = messageBody["start"] as? Int else {
                assert(false, "Expected start in getRows")
                return
            }
            
            guard let end_num = messageBody["end"] as? Int else {
                assert(false, "Expected end in getRows")
                return
            }
            
            self.pipe!.writePipe(method: "get_rows", start: start_num, end: end_num)
            break
            
        case "getIncorrects":
            guard let label = messageBody["label"] as? String else {
                assert(false, "Expected label in getRows")
                return
            }
            
            self.pipe!.writeIncorrect(label: label)
            break
        
        case "getCorrects":
            self.pipe!.writeCorrect()
            break
            
        case "getRowsEval":
            guard let start_idx = messageBody["start"] as? Int else {
                assert(false, "Expected start in getRowsEval")
                return
            }
            
            guard let length = messageBody["length"] as? Int else {
                assert(false, "Expected length in getRowsEval")
                return
            }
            
            guard let row_type = messageBody["row_type"] as? String else {
                assert(false, "Expected row_type in getRowsEval")
                return
            }
            
            guard let mat_type = messageBody["mat_type"] as? String else {
                assert(false, "Expected mat_type in getRowsEval")
                return
            }
            
            guard let cells = messageBody["cells"] as? [Any] else {
                assert(false, "Expected cells in getRowsEval")
                return
            }
            
            let arrData = try! JSONSerialization.data(withJSONObject: cells)
            let json_string = String(data: arrData, encoding: .utf8)!
            self.pipe!.writePipeEval(start: start_idx, length: length, row_type: row_type, mat_type: mat_type, cells: json_string)
            
            break
            
        case "getAccordion":
            guard let column_name = messageBody["column"] as? String else {
                assert(false, "column in getAccordion")
                return
            }
            
            guard let index_num = messageBody["index"] as? String else {
                assert(false, "index in getAccordion")
                return
            }
            
            self.pipe!.writeAccordion(method: "get_accordian", column_name: column_name, index_val: index_num)
            
            break
        case "writeProtoBuf":
            guard let proto_message = messageBody["message"] as? String else {
                assert(false, "no message in protobuf")
                return
            }
            
            self.pipe!.writeProtoBuf(message: proto_message);
        default:
            assert(false)
            break
        }
    }
    
    public func set_table(table_spec: [String: Any]) {
        self.data_spec.removeAll()
        self.table_spec = table_spec
        
        DispatchQueue.main.async {
            SharedData.shared.save_image?.isHidden = true
            SharedData.shared.save_vega?.isHidden = true
            SharedData.shared.print_vega?.isHidden = true
            SharedData.shared.page_setup?.isHidden = true
        }
    }
    
    public func set_vega(vega_spec: [String: Any]) {
        // TODO: write function to check valid vega spec
        self.data_spec.removeAll()
        self.vega_spec = vega_spec
        
        DispatchQueue.main.async {
            SharedData.shared.save_image?.isHidden = false
            SharedData.shared.save_vega?.isHidden = false
            SharedData.shared.print_vega?.isHidden = false
            SharedData.shared.page_setup?.isHidden = false
        }
    }
    
    // set evaluation spec and initial data
    public func set_evaluation(evaluation_spec: [String: Any]){
        self.data_spec.removeAll()
        self.evaluation_spec = evaluation_spec
        
        DispatchQueue.main.async {
            SharedData.shared.save_image?.isHidden = true
            SharedData.shared.save_vega?.isHidden = true
            SharedData.shared.print_vega?.isHidden = true
            SharedData.shared.page_setup?.isHidden = true
        }
    }
    
    public func add_data(data_spec: [String: Any]) {
        // TODO: write function to check valid data spec
        self.data_spec.append(data_spec)
    }
    
    public func add_images(image_spec: [String: Any]) {
        // TODO: write function to check valid image spec
        self.image_spec.append(image_spec)
    }
    
    public func add_accordion(accordion_spec: [String: Any]){
        DispatchQueue.main.async {
            
            let raw_data = ["data": accordion_spec] as [String : Any]
            let arrData = try! JSONSerialization.data(withJSONObject: raw_data)
            let json_string = String(data: arrData, encoding: .utf8)!
            let updateJS = String(format: "setAccordionData(%@);", json_string)
            
            self.view.evaluateJavaScript(updateJS, completionHandler: {(value, err) in
                if err != nil {
                    // if we got here, we got a JS error
                    log(err.debugDescription)
                    assert(false)
                }
            });
        }
    }
    
    public func send_proto(protobuf: String) {
        DispatchQueue.main.async {
            if(self.loaded){
                let updateJS = String(format: "setProtoMessage(\"%@\");", protobuf);
                self.view.evaluateJavaScript(updateJS, completionHandler: { (value, err) in
                    if err != nil {
                        // if we got here, we got a JS error
                        log(err.debugDescription)
                        assert(false)
                    }
                });
            }else{
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: { self.send_proto(protobuf: protobuf) })
            }
        }
    }
    
    public func save_data() {
        
        // open save panel
        let savePanel = NSSavePanel()
        
        // start the saving of the json
        savePanel.begin { (result: NSApplication.ModalResponse) -> Void in
            if result == .OK {
                let exportedFileURL = savePanel.url?.appendingPathExtension("csv")
                
                let jsString = "getData();";
                
                self.view.evaluateJavaScript(jsString, completionHandler: { (value , err) in
                    
                    if err != nil {
                        log(err.debugDescription)
                        assert(false)
                        return
                    }

                    var data_string = value.debugDescription
                    
                    // filter bytes
                    let index1 = data_string.index(data_string.endIndex, offsetBy: -1)
                    data_string = String(data_string[..<index1])
                    
                    // filter more bytes
                    let index = data_string.index(data_string.startIndex, offsetBy: 9)
                    let substring2 = data_string[index...]
                    
                    // encode data
                    let dataDecoded = substring2.data(using: .utf8)!
                    
                    // write data
                    try? dataDecoded.write(to: exportedFileURL!)
                });
                
            }
        }
    }
    
    public func save_vega() {
        
        // open save panel
        let savePanel = NSSavePanel()
        savePanel.allowedFileTypes = ["json"];
        
        // start the saving of the json
        savePanel.begin { (result: NSApplication.ModalResponse) -> Void in
            if result == .OK {
                let exportedFileURL = savePanel.url
                
                let jsString = "getSpec();";
                
                self.view.evaluateJavaScript(jsString, completionHandler: { (value , err) in
                    
                    if err != nil {
                        log(err.debugDescription)
                        assert(false)
                        return
                    }
                    
                    let s = String(describing: value!)
                    let dataDecoded = s.data(using: .utf8)!
                    try? dataDecoded.write(to: exportedFileURL!)
                    
                });
            }
        }
    }
    
    public func save_image() {
        
        // open save panel
        let savePanel = NSSavePanel()
        savePanel.allowedFileTypes = ["png"];
        
        // start the saving of the image
        savePanel.begin { (result: NSApplication.ModalResponse) -> Void in
            if result == .OK {
                let exportedFileURL = savePanel.url
                let jsString = "export_png();";
                
                // call function to get images
                self.view.evaluateJavaScript(jsString, completionHandler: { (value , err) in
                    
                    if(err != nil){
                        return
                    }
                    
                    let s = String(describing: value!)
                    let dataDecoded = Data(base64Encoded: s, options: .ignoreUnknownCharacters)!
                    try? dataDecoded.write(to: exportedFileURL!)
                });
            }
        }
    }
    
    public func get_image(completion: @escaping (NSImage) -> Void) {
        let jsString = "export_png();";
        self.view.evaluateJavaScript(jsString, completionHandler: { (value , err) in
            
            if(err != nil){
                return
            }
            
            let s = String(describing: value!)
            let dataDecoded = Data(base64Encoded: s, options: Data.Base64DecodingOptions(rawValue: NSData.Base64DecodingOptions.RawValue(0)))!
            let image = NSImage(data: dataDecoded)
            
            completion(image!);
        });
    }

    
    private func send_spec_js(spec: [String: Any], type: String) {
        debug_log("sending vega spec to JS")

        let raw_data = ["data": spec, "type": type] as [String : Any]
        let arrData = try! JSONSerialization.data(withJSONObject: raw_data)
        let json_string = String(data: arrData, encoding: .utf8)!
        let updateJS = String(format: "setSpec(%@);", json_string)
        
        self.view.evaluateJavaScript(updateJS, completionHandler: {(value, err) in
            if err != nil {
                // if we got here, we got a JS error
                log(err.debugDescription)
                assert(false)
            }
            
            debug_log("successfully sent vega spec to JS")
        })
    }
    
    private func send_spec() {
        DispatchQueue.main.async {
            if let spec = self.table_spec {
                debug_log("queuing up sending vega spec to JS")
                self.send_spec_js(spec: spec, type: "table")
            } else if let spec = self.vega_spec {
                debug_log("queuing up sending vega spec to JS")
                self.send_spec_js(spec: spec, type: "vega")
                // Working on sending the evaluation spec
            } else if let spec = self.evaluation_spec {
                debug_log("queuing up sending evaluation spec to JS")
                self.send_spec_js(spec: spec, type: "evaluate")
            } else {
                // Still waiting for a spec - if we get here,
                // it means the UI loaded before the backend actually sent us
                // a spec to render. This can happen if the spec takes a
                // long time to generate on the server side (if the data
                // is slow to access or requires materialization), or if the
                // serialization takes a long time (really long strings in
                // the spec itself).
                // Wait 100ms and try again.
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: self.send_spec)
            }
        }
    }
    
    private func send_data(){
        DispatchQueue.main.async {
            
            if self.data_spec.count == 0 && self.image_spec.count == 0 {
                // if no data to send, wait 100ms and try again
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: self.send_data)
                return
            }
            
            
            let updateJS: String
            if self.data_spec.count != 0 {
                
                let data_spec = self.data_spec.removeFirst()
                guard let data_json = try? JSON.stringify(obj: data_spec) else {
                    // should be JSON serializable, or we have a bug
                    assert(false)
                    return
                }
                updateJS = String(format: "updateData(%@);", data_json);
                
            } else {
                
                assert(self.image_spec.count != 0)
                let spec = self.image_spec.removeFirst()
                let raw_data = ["data": spec] as [String : Any]
                let arrData = try! JSONSerialization.data(withJSONObject: raw_data)
                let json_string = String(data: arrData, encoding: .utf8)!
                updateJS = String(format: "setImageData(%@);", json_string)
                
            }

            self.view.evaluateJavaScript(updateJS, completionHandler: {(value, err) in
                if err != nil {
                    // if we got here, we got a JS error
                    log(err.debugDescription)
                    assert(false)
                }
            });
            
            // recurse -- once the process is ready for data, we should be able to freely push as much data to it
            // as we have available, and as soon as we have more available, we should be able to push that too.
            // the 100ms wait at the beginning of this function (on empty queue) should prevent this from eating too much CPU time.
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1, execute: self.send_data)
        }
    }
}
