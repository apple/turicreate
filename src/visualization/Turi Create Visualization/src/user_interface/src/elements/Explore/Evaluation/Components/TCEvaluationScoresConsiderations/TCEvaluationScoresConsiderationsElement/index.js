import React, { Component } from 'react';
import './index.scss';

import error from './assets/error.png';
import warning from './assets/warning.png';

class TCEvaluationScoresConsiderationsElement extends Component {
  renderComponentType = () => {
    if(this.props.error){
      return (
        <img src={error} height={13}/>
      );
    }else{
      return (
        <img src={warning} height={13}/>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationScoresConsiderationsElement"
           onClick={this.props.onClick.bind(this)}>
        <div className="TCEvaluationScoresConsiderationsIcon">
          {this.renderComponentType()}
        </div>
        <div className="TCEvaluationScoresConsiderationsElementText">
           {this.props.content}
        </div>
      </div>
    );
  }
}

export default TCEvaluationScoresConsiderationsElement;