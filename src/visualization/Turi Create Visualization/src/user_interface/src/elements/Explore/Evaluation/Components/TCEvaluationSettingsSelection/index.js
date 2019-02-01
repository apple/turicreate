import React, { Component } from 'react';
import './index.scss';

class TCEvaluationSettingsSelection extends Component {
  render() {
    return (
      <div className="TCEvaluationSettingsSelection">
        <div className="TCEvaluationSettingsSelectionTitle">
          Metrics
        </div>
        <div className="TCEvaluationSettingsSelectionCont">
          <input type="checkbox"
                 name="accuracy_checkbox"
                 checked={this.props.accuracy}
                 onChange={this.props.changeAccuracy.bind(this)} />
          <span className="TCEvaluationSettingsSelectionContText">
           Accuracy
          </span>
          <br/>
          
          <input type="checkbox"
                 name="f1_score_checkbox"
                 checked={this.props.f1_score}
                 onChange={this.props.changeF1Score.bind(this)} />
          <span className="TCEvaluationSettingsSelectionContText">
           F1 Score
          </span>
          <br/>
          
          <input type="checkbox"
                 name="precision_checkbox"
                 checked={this.props.precision}
                 onChange={this.props.changePrecision.bind(this)} />
          <span className="TCEvaluationSettingsSelectionContText">
           Precision
          </span>
          <br/>

          <input type="checkbox"
                 name="recall_checkbox"
                 checked={this.props.recall}
                 onChange={this.props.changeRecall.bind(this)} />
          <span className="TCEvaluationSettingsSelectionContText">
           Recall
          </span>
        </div>
      </div>
    );
  }
}

export default TCEvaluationSettingsSelection;
