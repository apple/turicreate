import React, { Component } from 'react';
import './index.scss';

import TCEvaluationHeaderCell from './TCEvaluationHeaderCell';

class TCEvaluationHeader extends Component {

  clickedAccuracy = () =>{
    this.props.updateSortBy("accuracy")
    this.props.updateSortDirection()
  }

  clickedPrecision = () =>{
    this.props.updateSortBy("precision")
    this.props.updateSortDirection()
  }

  clickedRecall = () =>{
    this.props.updateSortBy("recall")
    this.props.updateSortDirection()
  }

  clickedF1Score= () =>{
    this.props.updateSortBy("f1_score")
    this.props.updateSortDirection()
  }

  clickedCell = () => {
    this.props.updateSortBy("class")
    this.props.updateSortDirection()
  }

  clickNumTest = () => {
    this.props.updateSortBy("train")
    this.props.updateSortDirection()
  }

  clickNumTrain = () => {
    this.props.updateSortBy("test")
    this.props.updateSortDirection()
  }

  renderAccuracy = () => {
    if(this.props.accuracy_visible){
      return (
        <TCEvaluationHeaderCell type="percent"
                                name="ACCURACY"
                                onclick={this.clickedAccuracy.bind(this)}
                                enabled={(this.props.sort_by === "accuracy")}
                                direction={this.props.sort_direction}/>
      )
    }
  }

  renderPrecision = () => {
    if(this.props.precision_visible){
      return (
        <TCEvaluationHeaderCell type="percent"
                                name="PRECISION"
                                onclick={this.clickedPrecision.bind(this)}
                                enabled={(this.props.sort_by === "precision")}
                                direction={this.props.sort_direction}/>
      )
    }
  }

  renderRecall = () => {
    if(this.props.recall_visible){
      return (
        <TCEvaluationHeaderCell type="percent"
                                name="RECALL"
                                onclick={this.clickedRecall.bind(this)}
                                enabled={(this.props.sort_by === "recall")}
                                direction={this.props.sort_direction}
                                />
      );
    }
  }

  renderF1Score = () => {
    if(this.props.f1_score_visible){
      return (
        <TCEvaluationHeaderCell type="percent"
                                name="F1 SCORE"
                                onclick={this.clickedF1Score.bind(this)}
                                enabled={(this.props.sort_by === "f1_score")}
                                direction={this.props.sort_direction}
                                />
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationHeader">
        <div className="TCEvaluationHeaderContainer">
          <TCEvaluationHeaderCell name="CLASS NAME"
                                  onclick={this.clickedCell.bind(this)}
                                  enabled={(this.props.sort_by === "class")}
                                  direction={this.props.sort_direction}/>
          <TCEvaluationHeaderCell type="images"
                                  name="EXAMPLE IMAGES"/>
          {this.renderAccuracy()}
          {this.renderPrecision()}
          {this.renderRecall()}
          {this.renderF1Score()}
          <TCEvaluationHeaderCell type="amount"
                                  name="NUM TEST"
                                  onclick={this.clickNumTest.bind(this)}
                                  enabled={(this.props.sort_by === "train")}
                                  direction={this.props.sort_direction}/>
        </div>
      </div>
    );
  }
}

export default TCEvaluationHeader;
