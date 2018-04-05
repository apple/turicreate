import React, { Component } from 'react';
import vegaEmbed from 'vega-embed';
import {vega as vegaTooltip} from'vega-tooltip';
import TcPlot from "../Chart";

class TcSummary extends TcPlot {
    render() {
        return (
                <div>
                <div className={["vega_container", "uninitialized"].join(' ')} ref={(vega_container) => { this.vega_container = vega_container; }}>
                </div>
                <div className={["hidden_cont"].join(' ')} ref={(hidden_container) => { this.hidden_container = hidden_container; }}>
                </div>
                </div>
                );
    }
}

export default TcSummary;
