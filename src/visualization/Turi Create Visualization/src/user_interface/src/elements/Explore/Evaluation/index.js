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

// TODO: Use Label Metrics for Precision Recall Graphs (Reshape into plots)
// TODO: Use `confidently_wrong_conf_mat` to display the TCEvaluationConfusionMenu
/*
UNKNOWN:
 INPUT:
 {
 "count":4,
 "target_label":0,
 "prob_default":0,
 "norm_prob":0.6833108822485874,
 "predicted_label":0,
 "prob":2.733243528994349
 }
 
 Look at Yonghoon's implementation and figure out how to use it here
 
 In the meantime just display everything
 
 */

/*
 
 */

export default TCEvaluation;
