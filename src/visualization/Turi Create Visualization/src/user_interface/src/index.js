import React from 'react';
import ReactDOM from 'react-dom';

import TcPlot from './elements/Plot/Chart/index.js';
import TcSummary from './elements/Plot/Summary/index.js';
import TcTable from './elements/Explore/Table/index.js';

var SpecType = Object.freeze({"table":1, "vega":2, "summary":3, "annotate": 4})
window.flex_type_enum = Object.freeze({"integer":0, "float":1, "string":2, "vector": 3, "list": 4, "dict": 5, "datetime": 6, "undefined": 7, "image": 8, "nd_vector": 9});

var component_rendered = null;
var spec_type = null;


function resetDisplay(){
    document.getElementById('loading_container').style.display = "block";
    document.getElementById('table_vis').style.display = 'none';
    document.getElementById('vega_vis').style.display = 'none';
    component_rendered = null;
    spec_type = null;
}

window.setSpec = function setSpec(value) {
    resetDisplay();
    switch(value.type) {
        case "vega":
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
        case "annotate":
            spec_type = SpecType.annotate;
            break;
        default:
            break;
    }
}

window.updateData = function updateData(data) {
    switch(spec_type){
        case SpecType.table:
        case SpecType.summary:
        case SpecType.vega:
            document.getElementById("loading_container").style.display = "none";
            component_rendered.updateData(data);
            break;
        case SpecType.annotate:
            console.log("annotate");
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
    input_data["data"] = json_obj["vega_spec"];json_obj["data_spec"]
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
