import React, { Component } from 'react';
import './index.scss';

import carret from './assets/carret.svg';

class TCEvaluationTooltip extends Component {
  render() {
    return (
      <div className="TCEvaluationTooltip">
        <div className="TCEvaluationTooltipText">
          {this.props.text}
        </div>
        <div className="TCEvaluationCarret">
          <img src={carret}
               height={9}
               className="TCEvaluationCarretIcon"/>
        </div>
      </div>
    );
  }
}

export default TCEvaluationTooltip;
