import React, { Component } from 'react';
import './index.scss';

import TCEvaluationImageComponent from '../TCEvaluationImageComponent';
import TCEvaluationIncorrectlyClassifiedContainer from './TCEvaluationIncorrectlyClassifiedContainer';

class TCEvaluationIncorrectlyClassified extends Component {

  renderIncorrectlyClassified = () => {
    if(this.props.incorrect != null && this.props.incorrect != "loading"){
      return this.props.incorrect.map((inc, index) => (
            <TCEvaluationIncorrectlyClassifiedContainer images={inc.images}
                                                        label={inc.label}/>
      ))
    }
  }

  render() {
    return (
      <div className="TCEvaluationIncorrectlyClassified">
        <div className="TCEvaluationIncorrectlyClassifiedTitle">
          Incorrect Classification
        </div>
        {this.renderIncorrectlyClassified()}
      </div>
    );
  }
}

export default TCEvaluationIncorrectlyClassified;