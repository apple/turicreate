import React, { Component } from 'react';
import './index.scss';
import TCEvaluationSummaryStat from './TCEvaluationSummaryStat';
import TCEvaluationSummaryImage from './TCEvaluationSummaryImage';

import * as d3 from "d3";

class TCEvaluationSummary extends Component {
  constructor(props){
    super(props)
    this.formatPercentDetails = d3.format(",.2%");
  }

  render() {
    return (
      <div className="TCEvaluationSummary" onClick={this.props.onClick.bind(this, this.props.name)}>
        <div className="TCEvaluationSummaryTitle">
          {this.props.name}
        </div>
        <div className="TCEvaluationSummaryContent">
          <div className="TCEvaluationSummaryExamples">
            <div className="TCEvaluationSummaryImageContainer">
            {this.props.images.map((img, index) => (
              <TCEvaluationSummaryImage src={img} />
            ))}
            </div>
          </div>
          <div className="TCEvaluationSummaryStats">
            <div className="TCEvaluationSummaryContainers"> 
              <TCEvaluationSummaryStat key_element={"Number of Examples:"}
                                       value={this.props.num_examples} />
              <TCEvaluationSummaryStat key_element={"Correct:"}
                                       value={this.props.correct} />
              <TCEvaluationSummaryStat key_element={"Incorrect:"}
                                       value={this.props.incorrect} />
            </div>
            <div className="TCEvaluationSummaryContainers">
              <TCEvaluationSummaryStat key_element={"F1 score:"}
                                       value={this.formatPercentDetails(this.props.f1_score)}/>
              <TCEvaluationSummaryStat key_element={"Precision:"}
                                       value={this.formatPercentDetails(this.props.precision)}/>
              <TCEvaluationSummaryStat key_element={"Recall:"}
                                       value={this.formatPercentDetails(this.props.recall)}/>
            </div>
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationSummary;