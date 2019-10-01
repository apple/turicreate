import React, { Component } from 'react';
import './index.scss';

import TCEvaluationTooltip from '../TCEvaluationTooltip';

import info from './assets/info.svg';

class TCEvaluationMetrics extends Component {

  renderIcon = () => {
    if(this.props.tooltip){
      return (
        <div className="TCEvaluationMetricsInfoIcon">
          <img src={info}
               height={10}
               width={10}
	       alt="Info"/>
        </div>
      );
    }
  }

  renderTooltip = () => {
    if(this.props.tooltip){
      return (
        <div className="TCEvaluationMetricsTooltipContainer">
          <TCEvaluationTooltip text={this.props.tooltip}/>
        </div>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationMetrics">
        <div className="TCEvaluationMetricsTitleContainer">
          <div className="TCEvaluationMetricsTitle">
            {this.props.title}
          </div>
          {this.renderIcon()}
          {this.renderTooltip()}
        </div>
        <div className="TCEvaluationMetricsValueContainer">
          {this.props.value}
        </div>
      </div>
    );
  }
}

export default TCEvaluationMetrics;
