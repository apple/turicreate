//  Copyright © 2017 Apple Inc. All rights reserved.
//
//  Use of this source code is governed by a BSD-3-clause license that can
//  be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import Foundation

/*!
     @class         Pipe
 
     @brief         The Pipe Class handles the Stdin
 
     @discussion    The Pipe object reads the stdin pipe, and accordingly updates the shared_data element
 
     @helps         Graph
 
!*/

class Pipe {
    
    // private variables here
    private var graph_data: VegaContainer
    
    init(graph_data:VegaContainer){
        self.graph_data = graph_data
        
        DispatchQueue.global(qos: .userInitiated).async {
            self.readPipe();
        }
        
    }
    
    public func readPipe(){
        #if DEBUG
            
            /*
             * For debugging purposes, throw some data through the pipe over an interval,
             * so at least by launching through the Xcode run scheme, we get some validation
             * of the real path of Pipe -> VegaContainer -> embedded WebKit view -> Vega
             * and of the vega rendering part.
             */
            if let _ = ProcessInfo.processInfo.environment["TC_VISUALIZATION_CLIENT_USE_FAKE_INPUT"] {
                process_data(data: Debugging.TableViewWithImage.spec)
                process_data(data: Debugging.TableViewWithImage.data1)
                process_data(data: Debugging.TableViewWithImage.data2)
            }

        #endif
        
        while (true) {
            guard let data = readLine() else {
                // nil readLine result means EOF
                // see https://developer.apple.com/documentation/swift/1641199-readline
                break
            }

            if data == "" {
                // ignore newlines
                continue
            }
            
            debug_log("Processing input: ")
            debug_log(data)
            process_data(data: data);
        }
    }
    
    public func writePipe(method: String, start: Int, end: Int){
        print("{'method':'get_rows','start':" + String(start) + ", 'end': " + String(end) + "}");
        fflush(__stdoutp)
    }
    
    public func writeAccordion(method: String, column_name: String, index_val: String){
        print("{'method':'get_accordion','column': '" + column_name + "', 'index': " + index_val + "}");
        fflush(__stdoutp)
    }
    
    private func process_data(data: String) {
        do {
            // expect "data" to contain JSON of the form [String: Any]
            let json = try JSON.parse(str: data) as! [String: Any]
            
            if let table_spec = json["table_spec"] as? [String: Any] {
                self.graph_data.set_table(table_spec: table_spec)
            }
            
            if let vega_spec = json["vega_spec"] as? [String: Any] {
                self.graph_data.set_vega(vega_spec: vega_spec)
            }
            
            if let data_spec = json["data_spec"] as? [String: Any] {
                self.graph_data.add_data(data_spec: data_spec)
            }
            
            if let image_spec = json["image_spec"] as? [String: Any] {
                self.graph_data.add_images(image_spec: image_spec)
            }
            
            if let accordion_spec = json["accordion_spec"] as? [String: Any] {
                self.graph_data.add_accordion(accordion_spec: accordion_spec)
            }

        } catch let error as NSError {
            log("Got error: \"\(error.localizedDescription)\" while trying to read:\n\(data)\n")
            assert(false)
        } catch {
            log("Encountered an unexpected error while processing input string \"\(data)\".")
            assert(false)
        }
    }
}
