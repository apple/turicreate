import React, { Component } from 'react';
import './index.scss';

class TCEvaluationSummaryStat extends Component {
  render() {
    return (
      <div className="TCEvaluationSummaryStat">
        <div className="TCEvaluationSummaryStatDescription">
          {this.props.key_element}
        </div>
        <div className="TCEvaluationSummaryStatValue">
          {this.props.value}
        </div>
      </div>
    );
  }
}

export default TCEvaluationSummaryStat;

