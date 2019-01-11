import React, { Component } from 'react';
import './index.scss';

import {PieChart, F1Score} from '../TCEvaluationChartType';
import { Handler } from 'vega-tooltip';

import * as d3 from "d3";

class TCEvaluationLabelStats extends Component {
  constructor(props){
    super(props);
    
    this.tooltip_options = {
      theme: 'dark'
    }
    this.handleHover = this.handleHover.bind(this);
  }

  handleHover(...args) {
    this.setState({
      info: JSON.stringify(args)
    });
  }

  render() {
    return (
      <div className="TCEvaluationLabelStats">
        <div className="TCEvaluationLabelStatsTitle">
          Stats
        </div>
        <div className="TCEvaluationLabelStatsGraphs">
          <div className="TCEvaluationLabelStatsPieChart">
            <PieChart data={{
                            "table": [
                              { "id": 1,
                                "field": this.props.incorrect,
                                "color": "#D0021B"},
                              { "id": 2,
                                "field": this.props.correct,
                                "color": "#7ED321"}
                            ]
                          }} />
            <div className="TCEvaluationLabelStatsPieChartLegendContianer">
              <div className="TCEvaluationLabelStatsPieChartLegend">
                <div className="TCEvaluationLabelStatsPieChartLegendCorrect">
                </div>
                <div className="TCEvaluationLabelStatsPieChartLegendText">
                  Correct
                </div>
              </div>
              <div className="TCEvaluationLabelStatsPieChartLegend">
                <div className="TCEvaluationLabelStatsPieChartLegendIncorrect">
                </div>
                <div className="TCEvaluationLabelStatsPieChartLegendText">
                  Incorrect
                </div>
              </div>
            </div>
          </div>
          <div className="TCEvaluationLabelStatsFScore">
            <F1Score data={this.props.data}
                     onSignalHover={this.handleHover}
                     tooltip={new Handler(this.tooltip_options).call}/>
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationLabelStats;