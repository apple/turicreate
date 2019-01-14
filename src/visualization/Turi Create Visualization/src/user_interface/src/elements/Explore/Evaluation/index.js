import React, { Component } from 'react';
import './index.scss';

import TCEvaluationTopBar from './Components/TCEvaluationTopBar';
import TCEvaluationNavigationBar from './Components/TCEvaluationNavigationBar';
import TCEvaluationScores from './Components/TCEvaluationScores'
import TCEvaluationScoresConsiderations from './Components/TCEvaluationScoresConsiderations';
import TCEvaluationSummary from './Components/TCEvaluationSummary';
import TCEvaluationLabelMenu from './Components/TCEvaluationLabelMenu';
import TCEvaluationLabelStats from './Components/TCEvaluationLabelStats';
import TCEvaluationIncorrectlyClassified from './Components/TCEvaluationIncorrectlyClassified';
import TCEvaluationConfusionMenu from './Components/TCEvaluationConfusionMenu';

class TCEvaluation extends Component {
  render() {
    // TODO: add iterations and model_type to data
    return (
      <div className="TCEvaluation">
            <TCEvaluationTopBar accuracy={this.props.spec.accuracy}
                                iterations={10000}
                                precision={this.props.spec.precision}
                                recall={this.props.spec.recall}
                                model_type={"resnet-50"}
                                correct={this.props.spec.corrects_size}
                                total={this.props.spec.num_examples}
                                classes={this.props.spec.num_classes}/>
      </div>  
    );
  }
}

export default TCEvaluation;
