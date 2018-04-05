import Tctable from "./elements/Explore/Table";
import TcPlot from "./elements/Plot/Plot";
import TcSummary from "./elements/Plot/Summary";

import React from 'react';
import ReactDOM from 'react-dom';

var SpecType = Object.freeze({"table":1, "vega":2, "summary":3, "annotate": 4})

var component_rendered = null;
var spec_type = null;

function resetDisplay(){
    document.getElementById("loading_container").style.display = "block";
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
            console.log("table");
            break;
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

window.setImageData = function setImageData(){
    switch(spec_type){
        case SpecType.table:
            // TODO: set image data
            break;
        default:
            break;
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

window.export_png = function export_png(){
    switch(spec_type){
        case SpecType.summary:
        case SpecType.vega:
            return component_rendered.exportPNG();
        default:
            return "";
    }
}

window.addEventListener('contextmenu', event => event.preventDefault());
