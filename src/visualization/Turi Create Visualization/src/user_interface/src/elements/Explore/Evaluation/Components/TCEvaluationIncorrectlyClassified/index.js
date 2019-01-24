import React, { Component } from 'react';
import './index.scss';

import TCEvaluationImageComponent from '../TCEvaluationImageComponent';
import TCEvaluationIncorrectlyClassifiedContainer from './TCEvaluationIncorrectlyClassifiedContainer';

class TCEvaluationIncorrectlyClassified extends Component {
  render() {
    return (
      <div className="TCEvaluationIncorrectlyClassified">
        <div className="TCEvaluationIncorrectlyClassifiedTitle">
          Incorrect Classification
        </div>
        {this.props.incorrect.map((inc, index) => (
            <TCEvaluationIncorrectlyClassifiedContainer images={inc.images}
                                                        label={inc.label}/>
          ))}
      </div>
    );
  }
}

export default TCEvaluationIncorrectlyClassified;