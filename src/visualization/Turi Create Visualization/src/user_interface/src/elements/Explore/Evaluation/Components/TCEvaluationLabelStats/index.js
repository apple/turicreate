import React, { Component } from 'react';
import './index.scss';

import {PieChart, F1Score} from '../TCEvaluationChartType';
/*
import { Handler } from 'vega-tooltip';
*/

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

  chartData = () => {
    return {
      "f1_score": [
        {"a": "F1 Score", "b": this.props.data[this.props.selectedLabel]["f1_score"]},
        {"a": "Precision", "b": this.props.data[this.props.selectedLabel]["precision"]},
        {"a": "Recall", "b": this.props.data[this.props.selectedLabel]["recall"]},
        {"a": "Percent of Data", "b": this.props.data[this.props.selectedLabel]["num_examples"]/this.props.total},
      ]
    }
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
                                "field": this.props.data[this.props.selectedLabel]["incorrect"],
                                "color": "#D0021B"},
                              { "id": 2,
                                "field": this.props.data[this.props.selectedLabel]["correct"],
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
            <F1Score data={this.chartData()} />
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationLabelStats;