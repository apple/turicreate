import React, { Component } from 'react';
import './index.scss';

import TCEvaluationScoresConsiderationsElement from './TCEvaluationScoresConsiderationsElement'

class TCEvaluationScoresConsiderations extends Component {
  render() {
    return (
      <div className="TCEvaluationScoresConsiderationsParent">
        <div className="TCEvaluationScoresConsiderationsTitle">
          Top Confusions
        </div>
        <div className="TCEvaluationScoresConsiderations">
          <div className="TCEvaluationScoresConsiderationsBox">
            {this.props.considerations.map((consideration, index) => (
              <TCEvaluationScoresConsiderationsElement error={consideration.error}
                                                       content={consideration.message}/>
            ))}
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationScoresConsiderations;