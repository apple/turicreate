import React, { Component } from 'react';

import './index.css';

var vega = require('vega');
var vl = require('vega-lite');
var vegaTooltip = require('vega-tooltip');

class TcPlot extends Component {

    componentDidMount(){
        this.addSpec(this.props.vega_spec);
    }

    checkLoadedFlag(data, $this) {
        if($this.vegaLoading) {
            window.setTimeout($this.checkLoadedFlag, 100, data, $this);
        } else {
            if($this.vegaLoading){
                debugger;
            }

            if (!data || !data.values) {
                debugger;
            }

            var newData = data.values.slice();

            var changeSet = vega.changeset();

            if(data.progress != null){
                var progress = parseFloat(data.progress);
                if(parseInt(progress*100, 10) >= 100){
                    document.getElementById('progress_bar').style.width = parseInt(progress*100)+"%";
                    setTimeout(function(){
                               document.getElementById('progress_bar').style.opacity = "0.0";
                               }, 1000);
                }else{
                    document.getElementById('progress_bar').style.width = parseInt(progress*100)+"%";
                }
            }

            var prevData = $this.vegaView.data("source_2");
            if(prevData.length > 0){
                changeSet = changeSet.remove(prevData);
            }

            changeSet = changeSet.insert(newData);
            $this.vegaView.change("source_2", changeSet).runAfter(function(viewInstance) {

                                                                  });

            $this.vegaView.toCanvas().then(function(result){
                                           $this.hidden_container.innerHTML = '';
                                           $this.hidden_container.appendChild(result);
                                           });
        }
    }

    updateData(data){
        this.checkLoadedFlag(data, this);
    }

    addSpec(spec){
        this.bubbleOpts = {
            showAllFields: true,
        };

        if (typeof(spec) === 'string') {
            spec = JSON.parse(spec);
        }
        // If size is not specified, default to our default window size, 720x550
        if (!spec.width && !spec.height) {
            spec.width = 720;
            spec.height = 550;
        }
        if (spec['$schema'].startsWith('https://vega.github.io/schema/vega-lite/')) {
            spec = vl.compile(spec).spec;
        }

        this.vega_json = spec;
        this.vega_json.autosize = {"type": "fit", "resize": true, "contains": "padding"};

        if(this.vega_json["metadata"] != null){
            if(this.vega_json["metadata"]["bubbleOpts"] != null){
                this.bubbleOpts = this.vega_json["metadata"]["bubbleOpts"];
            }
        }

        this.vegaLoading = true;
        this.vegaView = new vega.View(vega.parse(this.vega_json), {'renderer': 'svg'})
                                .initialize(this.vega_container)
                                .hover()
                                .run();
        this.vegaLoading = false;
        vegaTooltip.vega(this.vegaView, this.bubbleOpts);

        if(window.navigator.platform === 'MacIntel' && !window.tcvizBrowserMode){
            window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'ready'});
        }
    }

    getSpec(){
        var vegaResult = this.vega_json
        var previousData = this.vegaView.data("source_2");

        for(var x = 0; x < vegaResult.data.length; x++){
            if(vegaResult.data[x]["name"] === "source_2"){
                vegaResult.data[x]["values"] = previousData;
            }
        }

        return JSON.stringify(vegaResult);
    }

    getData(){
        var previousData = this.vegaView.data("source_2");

        var array = typeof previousData != 'object' ? JSON.parse(previousData) : previousData;
        var str = '';

        for (var i = 0; i < array.length; i++) {
            var line = '';
            for (var index in array[i]) {
                if (line !== '') line += ','

                    line += array[i][index];
            }

            str += line + '\r\n';
        }

        return str;
    }

    exportPNG(){
        return this.hidden_container.childNodes[0].toDataURL("image/png").replace(/^data:image\/(png|jpg);base64,/, "");
    }

    render() {
        return (
                <div>
                <div className={["vega_container"].join(' ')} ref={(vega_container) => { this.vega_container = vega_container; }}>
                </div>
                <div className={["hidden_cont"].join(' ')} ref={(hidden_container) => { this.hidden_container = hidden_container; }}>
                </div>
                </div>
                );
    }
}

export default TcPlot;
