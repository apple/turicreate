import React, { Component } from 'react';
import './index.scss';

import TCEvaluationSettings from '../TCEvaluationSettings';
import TCEvaluationMetrics from '../TCEvaluationMetrics';

import * as d3 from "d3";

class TCEvaluationOverview extends Component {
  constructor(props){
    super(props)

    this.formatDecimal = d3.format(".2f")
    this.state={
      "accuracy": true,
      "f1_score": false,
      "precision": false,
      "recall": false
    }
  }

  renderMetrics(){
    var returnMetrics = []
    if(this.props.accuracy_visible){
      returnMetrics.push(
        <TCEvaluationMetrics title={"Accuracy"}
                             value={this.formatDecimal(this.props.accuracy*100) + "%"}
                             tooltip={this.props.total_correct + " out of " + this.props.total_num + " predicted correctly"}/>
      )
    }

    if(this.props.precision_visible){
      returnMetrics.push(
        <TCEvaluationMetrics title={"Precision"}
                             value={this.formatDecimal(this.props.precision*100) + "%"}
                             tooltip={"Out of the labels predicted by the model, which were correct"}/>
      )
    }

    if(this.props.recall_visible){
      returnMetrics.push(
        <TCEvaluationMetrics title={"Recall"}
                             value={this.formatDecimal(this.props.recall*100) + "%"}
                             tooltip={"Out of the correct labels, which were predicted by the model"}/>
      )
    }

    if(this.props.f1_score_visible){
      returnMetrics.push(
        <TCEvaluationMetrics title={"F1 Score"}
                             value={this.formatDecimal(((2*(this.props.recall * this.props.precision))/(this.props.recall + this.props.precision))*100) + "%"}
                             tooltip={"A harmonic average of precision and recall"}/>
      )
    }

    returnMetrics.push(
      <TCEvaluationMetrics title={"Total Tested"}
                           value={this.props.total_num}
                           tooltip={"Total number of examples tested"}/>
    )

    return returnMetrics;
  }


  render() {
    return (
      <div className="TCEvaluationOverview">
        <div className="TCEvaluationOverviewContainer">
          <div className="TCEvaluationOverviewTitleContainer">
            <div className="TCEvaluationOverviewTitle">
              Overview
            </div>
            <TCEvaluationSettings accuracy={this.props.accuracy_visible}
                                  changeAccuracy={this.props.changeAccuracy.bind(this)}
                                  precision={this.props.precision_visible}
                                  changePrecision={this.props.changePrecision.bind(this)}
                                  recall={this.props.recall_visible}
                                  changeRecall={this.props.changeRecall.bind(this)}
                                  f1_score={this.props.f1_score_visible}
                                  changeF1Score={this.props.changeF1Score.bind(this)}/> 
          </div>
          <div className="TCEvaluationOverviewMetricsContainer">
            <TCEvaluationMetrics title={"Model"}
                                 value={this.props.model_type}/>
            <TCEvaluationMetrics title={"Number of Iterations"}
                                 value={this.props.iterations}/>
            {this.renderMetrics()}
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationOverview;
