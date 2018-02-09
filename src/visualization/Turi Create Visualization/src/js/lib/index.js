import Tctable from "./elements/Table";
import React from 'react';
import ReactDOM from 'react-dom';

var command_down = 0;
var body_zoom = 100;

document.onkeydown = function(e) {
    var key_code = e.keyCode || e.charCode;
    if (key_code == 91 || key_code == 93){
        command_down += 1;
    }

    if(key_code == 187 && command_down > 0){
        body_zoom += 10;
        document.body.style.zoom = body_zoom+"%"
    }

    if(key_code == 189 && command_down > 0){
        if(body_zoom > 10){
            body_zoom -= 10;
            document.body.style.zoom = body_zoom+"%"
        }
    }
};

document.onkeyup = function(e) {
    var key_code = e.keyCode || e.charCode;

    if (key_code == 91 || key_code == 93){
        command_down -= 1;
    }
};

// vlSpec
var loaded = false;
var vlSpec = null;
var vegaView = null;
var vegaLoading = false;
var tableSpec = null;

// scroll variables
window.step_size = 50;
var max_sframe = 0;
var setScroll = true;


// store current positions
window.set_lower = 0;
window.set_higher = 2;

var scrollVal = -1;
window.image_dictionary = {};

function useEmbeddedVega(data) {

    if (vegaLoading) {
        // should never get here
        debugger;
    }

    if (!data || !data.values) {
        // should never get here
        debugger;
    }

    var newData = data.values.slice();
    var changeSet = vega.changeset();

    if(data.progress != null){
        var progress = parseFloat(data.progress);
        if(parseInt(progress*100) >= 100){
            document.getElementById('progress_bar').style.width = parseInt(progress*100)+"%";

            setTimeout(function(){
                       document.getElementById('progress_bar').style.opacity = "0.0";
                       }, 1000);
        }else{
            document.getElementById('progress_bar').style.width = parseInt(progress*100)+"%";
        }
    }

    var prevData = vegaView.data("source_2");

    for(var y = 0; y < newData.length; y++ ){
        if(newData[y]["a"] != null){ // hack to determine whether SFrame summary view
            for(var x = 0; x < prevData.length; x++ ){
                if(prevData[x]["a"] == newData[y]["a"]){
                    changeSet = changeSet.remove(prevData[x]);
                }
            }
        }else{ // or not SFrame summary view
            if(prevData.length > 0){
                changeSet = changeSet.remove(prevData);
                break;
            }
        }
    }

    changeSet = changeSet.insert(newData);
    vegaView.change("source_2", changeSet).runAfter(function(viewInstance) {
                                                    document.getElementById('vega_vis').style.opacity = '1';
                                                    });

    vegaView.toCanvas().then(function(result){
                             document.getElementById("hidden_cont").innerHTML = '';
                             document.getElementById("hidden_cont").appendChild(result);
                             })
}


/**
 *
 * Update the vega spec and redraw the vega graph
 *
 * @param {Array<JSON>} newData - Array of data values for the vega spec
 *
 * @returns {Bool} - True if the JSON is valid, False if JSON is invalid
 *
 */
window.updateData = function updateData(data) {
    if (tableSpec) {
        if (!data) {
            return;
        }

        document.getElementById("loading_container").style.display = "none";

        var window_height = window.innerHeight-44;
        max_sframe = tableSpec["size"];
        var table_title = tableSpec["title"];
        ReactDOM.render(<Tctable spec={tableSpec} data={data} size={max_sframe} enterHandler={jumpToRowCallback} tableTitle={table_title} starting_height={window_height}/>, document.getElementById('table_vis'), function() {

                        if(scrollVal != -1){
                        scrollToValue(scrollVal);
                        }

                        setScroll = true;

                        });

    } else if (vlSpec) {
        document.getElementById("loading_container").style.display = "none";

        useEmbeddedVega(data);
    }
}

var jumpToRowCallback = function (event) {
    if(document.getElementById("rowNum") != null){
        var row_number = document.getElementById("rowNum").value;
        document.getElementById("rowNum").value = "";
        if(!isNaN(parseInt(row_number, 10))){
            jump_to(row_number);
        }
    }
};

function windowResized(){
    var window_height = window.innerHeight;
    var element = document.getElementsByClassName('resize_container')[0];
    var tableTitle = document.getElementsByClassName('tableTitle');
    if(element!= null){
        if(tableTitle.length == 0){
            // 44px is the total height of the bottom strip containing the # row text field (14px top-padding, 14px bottom-padding, 14.4px line height, and 1px border top )
            element.style.height = (window_height-44)+"px";
        }else{
            // 90px is the total height of the bottom strip containing the # row text field, plus the padding on the title (bottom_row: 14px top-padding, 14px bottom-padding, 14.4px line height, and 1px border top; title-padding: padding-bottom:16px; padding-top: 16px; line-height: 16px)
            element.style.height = (window_height-tableTitle[0].offsetHeight-90)+"px";
        }
    }
}

/**
 *
 * Get the JSON in string format from the spec
 *
 * @returns {String} A string representative of the JSON data in the spec
 *
 **/
window.getSpec = function getSpec() {
    var previousData = vegaView.data("source_2");

    for(var x = 0; x < window.vegaResult.data.length; x++){
        if(window.vegaResult.data[x]["name"] == "source_2"){
            window.vegaResult.data[x]["values"] = previousData;
        }
    }

    return JSON.stringify(window.vegaResult);
}

/**
 *
 * Get the JSON in CSV format from the data
 *
 * @returns {String} A CSV representative of the JSON data in the data
 *
 **/

window.ConvertToCSV = function ConvertToCSV(objArray) {
    var array = typeof objArray != 'object' ? JSON.parse(objArray) : objArray;
    var str = '';

    for (var i = 0; i < array.length; i++) {
        var line = '';
        for (var index in array[i]) {
            if (line != '') line += ','

                line += array[i][index];
        }

        str += line + '\r\n';
    }

    return str;
}

window.getData = function getData() {
    return ConvertToCSV(data);
}

window.getRows = function getRows(start_index, end_index){
  if(window.navigator.platform == 'MacIntel'){
    window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'getRows', start: start_index, end: end_index});
  }else{
    window.linux_two_coms('{"method":"get_rows", "start":' + start_index + ', "end": ' + end_index + '}');
  }
}

window.scrollToValue = function scrollToValue(value){
    var element = document.getElementsByClassName("header_element");
    for(var x = 0; x < element.length; x++){
        if(element[x].innerText == value){
            document.getElementById("y-scrollbar").scrollTop = element[x].offsetTop - element[x].offsetHeight - 6;
            break;
        }
    }

    scrollVal = -1;
}

window.jump_to = function jump_to(value){
    setScroll = false;
    var lower_value = 0;
    var upper_value = 0;
    var lower_bound;
    var upper_bound;

    if(value > max_sframe || value < 0){
        return "err: out of bounds";
    }

    if(value%window.step_size  != 0){
        lower_bound = Math.floor(value/window.step_size);
        upper_bound = Math.ceil(value/window.step_size);
    }else{
        lower_bound = Math.floor(value/window.step_size);
        upper_bound = lower_bound + 1;
    }

    window.set_lower = (lower_bound-1 >= 0)?(lower_bound-1):0;
    window.set_higher = upper_bound

    lower_value = (lower_bound-1 >= 0)?((lower_bound-1)*window.step_size):0;
    upper_value = (upper_bound*window.step_size > max_sframe)?max_sframe:(upper_bound*window.step_size);

    window.getRows(lower_value, upper_value);
    scrollVal = value;

}

window.updateScrollUp = function updateScrollUp(){
    var lower_value;
    var upper_value;

    if(window.set_lower*window.step_size <= 0){
        return "Min value reached";
    }

    if(setScroll){
        setScroll = false;

        window.set_lower -= 1;
        window.set_higher -= 1;

        lower_value = (window.set_lower >= 0)?(window.set_lower*window.step_size):0;
        upper_value = (window.set_higher*window.step_size > max_sframe)?max_sframe:(window.set_higher*window.step_size);

        var element = document.getElementsByClassName("header_element");
        if(element.length > 0){
            scrollVal = element[0].innerText;
        }

        window.getRows(lower_value, upper_value);
    }
}

window.updateScrollDown = function updateScrollDown(){
    var lower_value;
    var upper_value;

    if(window.set_higher*window.step_size >= max_sframe){
        return "Max value reached";
    }

    if(setScroll){
        setScroll = false;

        window.set_lower += 1;
        window.set_higher += 1;

        lower_value = (window.set_lower >= 0)?(window.set_lower*window.step_size):0;
        upper_value = (window.set_higher*window.step_size > max_sframe)?max_sframe:(window.set_higher*window.step_size);

        var element = document.getElementsByClassName("header_element");
        if(element.length > 0){
            scrollVal = element[element.length - 1].innerText - 8;
        }

        window.getRows(lower_value, upper_value);
    }
}

setInterval(function(){
            if(setScroll){
            var element = document.getElementById("y-scrollbar");
            if(element != null){
            if(((element.scrollTop+element.offsetHeight)/element.scrollHeight) == 1){
            updateScrollDown();
            }

            if(((element.scrollTop)/element.scrollHeight) == 0){
            updateScrollUp();
            }
            }
            }
            }, 50);


window.cleanImageDictionary = function cleanImageDictionary(){
    for (var key in window.image_dictionary){
        if(parseInt(key, 10) < window.set_lower*window.step_size || parseInt(key, 10) > window.set_higher*window.step_size){
            delete window.image_dictionary[key];
        }
    }
}

window.setImageData = function setImageData(value){
    window.cleanImageDictionary();
    if(value.data.data){
        for(var x = 0; x < value.data.data.length; x++){
            if (!window.image_dictionary[String(value.data.data[x]["idx"])]) {
                window.image_dictionary[String(value.data.data[x]["idx"])] = {};
            }
            if (!window.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]) {
                window.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]] = {};
            }
            window.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]["image"] = value.data.data[x]["image"];
            window.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]["format"] = value.data.data[x]["format"];
        }
    }
}

/**
 *
 * Set the updated spec for the vega-lite parser
 *
 * @param {JSON} vega_lite_spec - The vega lite spec
 *
 * @throws {InvalidJSONError} - If JSON in editor is invalid function throws an InvalidJSONError
 *
 */
window.setSpec = function setSpec(value) {
    if (value.type == "table") {
        document.getElementById('vega_vis').style.display = 'none';
        document.getElementById('table_vis').style.display = 'block';
        tableSpec = value.data;
        vlSpec = null;

        // Let the containing process know we're ready for data updates
        if(window.navigator.platform == 'MacIntel'){
          window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'ready'});
        }

    } else if (value.type == "vega") {
        document.getElementById('table_vis').style.display = 'none';
        document.getElementById('vega_vis').style.display = 'block';
        document.getElementById('vega_vis').style.opacity = '0';
        vlSpec = value.data;
        tableSpec = null;

        vegaLoading = true;

        var opt = {
        mode: ('$schema' in vlSpec && vlSpec['$schema'].indexOf('vega-lite') == -1) ? "vega": "vega-lite",
        renderer: "svg"
        };

        var bubbleOpts = {
        showAllFields: true
        };

        if(vlSpec["metadata"] != null){
            if(vlSpec["metadata"]["bubbleOpts"] != null){
                bubbleOpts = vlSpec["metadata"]["bubbleOpts"];
            }
        }



        vlSpec.autosize = {"type": "pad", "resize": true, "contains": "padding"};

        vega.embed("#vega_vis", vlSpec, opt).then(function(viewInstance) {
                                                  vegaTooltip.vegaLite(viewInstance.view, vlSpec, bubbleOpts);
                                                  vegaView = viewInstance.view;
                                                  window.vegaResult = viewInstance.spec;
                                                  vegaLoading = false;
                                                  if(window.navigator.platform == 'MacIntel'){
                                                    window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'ready'});
                                                  }

                                                  }).catch(function(error) {
                                                           console.error(error);
                                                           });
    }

}

/**
 *
 * Set the updated spec for the vega-lite parser
 *
 * @param {JSON} vega_lite_spec - The vega lite spec
 *
 * @throws {InvalidJSONError} - If JSON in editor is invalid function throws an InvalidJSONError
 *
 */

window.export_png = function export_png(){
    return document.getElementById("hidden_cont").childNodes[0].toDataURL("image/png").replace(/^data:image\/(png|jpg);base64,/, "");
}

var curYPos, curXPos, curDown;

window.addEventListener('mousemove', function(e){
                        if(curDown){
                        window.scrollTo(document.body.scrollLeft + (curXPos - e.pageX), document.body.scrollTop + (curYPos - e.pageY));
                        }
                        });

window.addEventListener('mousedown', function(e){
                        curYPos = e.pageY;
                        curXPos = e.pageX;
                        curDown = true;
                        });

window.addEventListener('mouseup', function(e){
                        curDown = false;
                        });

window.addEventListener('resize', windowResized, false);

window.addEventListener('contextmenu', event => event.preventDefault());
