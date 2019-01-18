import React, { Component } from 'react';
import './index.scss';
import TCEvaluationCells from './TCEvaluationCells';

class TCEvaluationRows extends Component {

  renderSelected = () => {
    if(this.props.selected){
      return (
        {"background": "#D6E7FB"}
      );
    }
  }

  renderAccuracy = () => {
    if(this.props.accuracy_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.correct/this.props.data.num_examples}/>
      )
    }
  }

  renderPrecision = () => {
    if(this.props.precision_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.precision}/>
      )
    }
  }

  renderRecall = () => {
    if(this.props.recall_visible){
      return (
        <TCEvaluationCells type="percent"
                             value={this.props.data.recall}/>
      ); 
    }
  }

  renderF1Score = () => {
    if(this.props.f1_score_visible){
      return (
        <TCEvaluationCells type="percent"
                             value={this.props.data.f1_score}/>
      ); 
    }
  }

  render() {
    return (
      <div className="TCEvaluationRows"
           style={this.renderSelected()}
           onClick={this.props.onclick.bind(this, this.props.data.name)}>
        <div className="TCEvaluationRowContainer">
          <TCEvaluationCells value={this.props.data.name}/>
          <TCEvaluationCells type="images"
                             value={this.props.data.correct_images}/>
          {this.renderAccuracy()}
          {this.renderPrecision()}
          {this.renderRecall()}
          {this.renderF1Score()}
          <TCEvaluationCells type="amount"
                             value={this.props.data.num_examples}
                             max={this.props.max_value}/>
          <TCEvaluationCells type="amount"
                             value={this.props.data.num_examples}
                             max={this.props.max_value}/>
        </div>
      </div>
    );
  }
}

export default TCEvaluationRows;
