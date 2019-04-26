import React from 'react';
import ReactDOM from 'react-dom';

import TcPlot from './elements/Plot/Chart/index.js';
import TcSummary from './elements/Plot/Summary/index.js';
import TcTable from './elements/Explore/Table/index.js';
import TCEvaluation from './elements/Explore/Evaluation/index.js';

import messageFormat from './format/message';

import { load, Root } from 'protobufjs';

import TCAnnotate from './elements/Annotate';

var command_down = 0;
var body_zoom = 100;

var LEFT_COMMAND_KEY = 91;
var RIGHT_COMMAND_KEY = 93;
var PLUS_KEY = 187;
var MINUS_KEY = 189;


document.onkeydown = function(e) {
    var key_code = e.keyCode || e.charCode;
    if (key_code == LEFT_COMMAND_KEY || key_code == RIGHT_COMMAND_KEY){
        command_down += 1;
    }
    
    if(key_code == PLUS_KEY && command_down > 0){
        body_zoom += 10;
        document.body.style.zoom = body_zoom+"%"
    }
    
    if(key_code == MINUS_KEY && command_down > 0){
        if(body_zoom > 10){
            body_zoom -= 10;
            document.body.style.zoom = body_zoom+"%"
        }
    }
};

document.onkeyup = function(e) {
    var key_code = e.keyCode || e.charCode;
    
    if (key_code == LEFT_COMMAND_KEY || key_code == RIGHT_COMMAND_KEY){
        command_down -= 1;
    }
};

var SpecType = Object.freeze({"table":1, "vega":2, "summary":3, "annotate": 4, "evaluate": 5})
window.flex_type_enum = Object.freeze({"integer":0, "float":1, "string":2, "vector": 3, "list": 4, "dict": 5, "datetime": 6, "undefined": 7, "image": 8, "nd_vector": 9});

var component_rendered = null;
var spec_type = null;


function resetDisplay(){
    document.getElementById('loading_container').style.display = "block";
    document.getElementById('table_vis').style.display = 'none';
    document.getElementById('vega_vis').style.display = 'none';
    document.getElementById('annotate_viz').style.display = 'none';
    component_rendered = null;
    spec_type = null;
}

window.setSpec = function setSpec(value) {
    resetDisplay();
    switch(value.type) {
        case "vega":
            document.getElementById("loading_container").style.display = "none";
            document.getElementById('vega_vis').style.display = 'block';
            component_rendered = ReactDOM.render(<TcPlot vega_spec={value.data} />, document.getElementById('vega_vis'));
            spec_type = SpecType.vega;
            break;
        case "table":
            document.getElementById('table_vis').style.display = 'block';
            component_rendered = ReactDOM.render(<TcTable table_spec={value.data} />, document.getElementById('table_vis'));
            spec_type = SpecType.table;
            break;
        case "summary":
            spec_type = SpecType.summary;
            break;
        case "evaluate":
            document.getElementById("loading_container").style.display = "none";
            document.getElementById('evaluation_vis').style.display = 'block';
            component_rendered = ReactDOM.render(<TCEvaluation spec={value.data} />, document.getElementById('evaluation_vis'));
            spec_type = SpecType.evaluate;
            break;
            
        default:
            break;
    }
}

window.setProtoMessage = function setProtoMessage(value){
    
    document.getElementById("loading_container").style.display = "none";
    document.getElementById('annotate_viz').style.display = 'block';
    
    var root = Root.fromJSON(messageFormat);
    
    const ParcelMessage = root.lookupType("TuriCreate.Annotation.Specification.Parcel");
    const buffer = Uint8Array.from(atob(value), c => c.charCodeAt(0));
    var decoded = ParcelMessage.decode(buffer);
    
    if (decoded.hasOwnProperty('metadata')) {
        component_rendered = ReactDOM.render(<TCAnnotate metadata={decoded.metadata}/>, document.getElementById('annotate_viz'));
        spec_type = SpecType.annotate;
    } else if(decoded.hasOwnProperty('data')) {
        for (var i = 0; i < decoded["data"]["data"].length; i++) {
            const row_index = decoded["data"]["data"][i]["rowIndex"];
            const type = decoded["data"]["data"][i]["images"][0]["type"];
            const data = decoded["data"]["data"][i]["images"][0]["imgData"];
            
            const width = decoded["data"]["data"][i]["images"][0]["width"];
            const height = decoded["data"]["data"][i]["images"][0]["height"];

            const image = "data:image/" + type + ";base64," + data;

            component_rendered.setImageData(row_index, image, width, height);
        }
    } else if(decoded.hasOwnProperty('annotations')) {
        for (var i = 0; i < decoded["annotations"]["annotation"].length; i++) {
            const row_index = decoded["annotations"]["annotation"][i]["rowIndex"][0];
            const annotation = decoded["annotations"]["annotation"][i]["labels"][0];
            component_rendered.setAnnotationData(row_index, annotation);
        }
    }
}

window.updateData = function updateData(data) {
    switch(spec_type){
        case SpecType.table:
        case SpecType.summary:
        case SpecType.vega:
        case SpecType.evaluate:
            document.getElementById("loading_container").style.display = "none";
            component_rendered.updateData(data);
            break;
        default:
            console.log("default");
    }
}

window.getSpec = function getSpec(){
    switch(spec_type){
        case SpecType.summary:
        case SpecType.vega:
            return component_rendered.getSpec();
        default:
            return "";
    }
}

window.getData = function getData(){
    switch(spec_type){
        case SpecType.summary:
        case SpecType.vega:
            return component_rendered.getData();
        default:
            return "";
    }
}

window.setImageData = function setImageData(value){
    switch(spec_type){
        case SpecType.table:
            return component_rendered.setImageData(value);
        default:
            return "";
    }
}

window.setAccordionData = function setAccordionData(value){
    switch(spec_type){
        case SpecType.table:
            return component_rendered.setAccordionData(value);
        default:
            return "";
    }
}

window.export_png = function export_png(){
    switch(spec_type){
        case SpecType.summary:
        case SpecType.vega:
            return component_rendered.exportPNG();
        default:
            return "";
    }
}

window.handleInput = function(data){
  var json_obj = data;

  if(json_obj["table_spec"] != null) {
    var input_data = {};
    input_data["data"] = json_obj["table_spec"];
    input_data["type"] = "table";

    window.setSpec(input_data);
  }

  if(json_obj["vega_spec"] != null) {
    var input_data = {};
    input_data["data"] = json_obj["vega_spec"];
    input_data["type"] = "vega";

    window.setSpec(input_data);
  }

  if(json_obj["data_spec"] != null) {
    window.updateData(json_obj["data_spec"]);
  }

  if(json_obj["image_spec"] != null) {
    var input_data = {};
    input_data["data"] = json_obj["image_spec"];
    window.setImageData(input_data);
  }

  if(json_obj["accordion_spec"] != null) {
    var input_data = {};
    input_data["data"] = json_obj["accordion_spec"];
    window.setAccordionData(input_data);
  }
}

window.addEventListener('contextmenu', event => event.preventDefault());
