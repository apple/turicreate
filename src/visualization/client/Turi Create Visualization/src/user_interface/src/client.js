/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

export async function fetch(url, body) {
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
      reject('An error occurred while requesting ' + url + ' from location ' + window.location);
    });
    req.responseType = "json";
    if (typeof body === 'object') {
      body = JSON.stringify(body);
    }
    var method = "GET";
    if (body) {
      method = "POST";
    }
    req.open(method, url);
    req.send(body);
  });
}

async function initVisualization(id, type) {
  let spec = await fetch(`/spec/${type}/${id}`);
  var body = undefined;
  if (type == 'table') {
    // for table, fetch the first 100 rows by default
    body = {'type': 'rows', start: 0, end: 100};
  }
  var data = await fetch(`/data/${type}/${id}`, body);

  window.setSpec(spec);
  window.handleInput(data);

  if (type == 'plot') {
    // Stream in the rest of the data, if necessary
    while (data.progress < 1.0) {
      data = await fetch(`/data/${type}/${id}`);
      window.handleInput(data);
    }
  }
}

export async function initBrowserClient() {
  // Parse the query string and determine type of visualization
  let params = new URLSearchParams(window.location.search);
  let id = params.get('id');
  let type = params.get('type');
  initVisualization(id, type);
}
