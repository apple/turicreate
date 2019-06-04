import React, { Component } from 'react';

import './index.css';

var vega = require('vega');
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

            for(var y = 0; y < newData.length; y++ ){
                if(newData[y]["a"] != null){
                    for(var x = 0; x < prevData.length; x++ ){
                        if(prevData[x]["a"] === newData[y]["a"]){
                            changeSet = changeSet.remove(prevData[x]);
                        }
                    }
                }else{
                    if(prevData.length > 0){
                        changeSet = changeSet.remove(prevData);
                        break;
                    }
                }
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
        
        if(window.navigator.platform === 'MacIntel'){
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
                if (line != '') line += ','

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
