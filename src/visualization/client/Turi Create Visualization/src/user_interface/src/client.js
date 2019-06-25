/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

async function fetch(url) {
  // Create the XHR request
  var req = new XMLHttpRequest();

  // Return it as a Promise
  return new Promise(function(resolve, reject) {
    req.addEventListener("load", function() {
      if (req.status == 200) {
        resolve(this.response);
      } else {
        reject("Got unexpected status " + req.status + " from server");
      }
    });
    req.addEventListener("error", function() {
      reject(this.response);
    });
    req.responseType = "json";
    req.open("GET", url);
    req.send();
  });
}

export async function initBrowserClient() {
  // Reconstruct the current visualization ID from the query string
  // Fetch the spec for the current visualization
  let id = window.location.search.slice(1); // chop off "?"
  let spec = await fetch("/spec/" + id);

  // TODO - this logic assumes we're getting data for a streaming Plot object.
  // This will need to be refactored to support visualizations like explore().
  var data = await fetch("/data/" + id);

  window.setSpec(spec);
  window.updateData(data);
  while (data.progress < 1.0) {
    data = await fetch("/data/" + id);
    window.updateData(data);
  }
}
