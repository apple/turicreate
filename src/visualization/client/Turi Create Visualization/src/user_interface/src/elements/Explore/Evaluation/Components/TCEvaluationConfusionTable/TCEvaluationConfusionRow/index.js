import React, { Component } from 'react';
import './index.scss';

import TCEvaluationConfusionCell from './TCEvaluationConfusionCell';

class TCEvaluationConfusionRow extends Component {

  render() {
    return (
      <div className="TCEvaluationConfusionRow" onClick={this.props.selectRowConfusions.bind(this, this.props.consideration.actual, this.props.consideration.predicted)}>
        <TCEvaluationConfusionCell type="text"
                                   value={this.props.consideration.actual}/>
        <TCEvaluationConfusionCell type="text"
                                   value={this.props.consideration.predicted}/>
        <TCEvaluationConfusionCell type="amount"
                                   value={this.props.consideration.count}
                                   max={this.props.maxValue}/>
        <TCEvaluationConfusionCell type="images"
                                   value={this.props.consideration.images}/>
      </div>
    );
  }
}

export default TCEvaluationConfusionRow;
