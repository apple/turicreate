import React, { Component } from 'react';
import './index.scss';

import warning from './assets/warning.svg';
import TCEvaluationImageCells from '../../../TCEvaluationImageCells';

import * as d3 from "d3";

class TCEvaluationCells extends Component {
  constructor(props){
    super(props)
    this.formatDecimal = d3.format(".0f")
  }

  renderPercentLeft = () => {
    if(this.props.value >= 0.5){
      return (
        <div className="TCEvaluationCellsPercentShadedText"
             style={{"color": "#FFFFFF"}}>
          {this.formatDecimal(this.props.value*100)+"%"}
        </div>
      );
    }
  }

  renderAmountLeft = () => {
    if((this.props.value/this.props.max) >= 0.5){
      return (
        <div className="TCEvaluationCellsPercentShadedText"
             style={{"color": "#FFFFFF"}}>
          {this.formatDecimal(this.props.value)}
        </div>
      );
    }
  }

  renderPercentRight = () => {
    if(this.props.value < 0.5){
      return (
        <div className="TCEvaluationCellsPercentShadedText"
             style={{"color": "#484856"}}>
          {this.formatDecimal(this.props.value*100)+"%"}
        </div>
      );
    }
  }

  renderAmountRight = () => {
    if((this.props.value/this.props.max) < 0.5){
      return (
        <div className="TCEvaluationCellsPercentShadedText"
             style={{"color": "#484856"}}>
          {this.formatDecimal(this.props.value)}
        </div>
      );
    }
  }

  renderWarning = () => {
    if(this.props.value <= 0.2){
      return (
        <div className="TCEvaluationCellsPercentShadedWarning">
          <img src={warning} height={16}/>
        </div>
      );
    }
  }

  render() {
    if(this.props.type == "images"){
      return (
        <div className="TCEvaluationCellsImages">
          <div className="TCEvaluationCellsImagesContainer">
           {this.props.value.slice(0, 20).map((data, index) => {
            return(
              <TCEvaluationImageCells value={data}/>
            )
           })}
          </div>
        </div>
       );
    }else if(this.props.type == "percent"){
      return (
        <div className="TCEvaluationCellsPercent">
          <div className="TCEvaluationCellsPercentShaded"
               style={{"width": this.props.value*100+"%"}}>
               { this.renderPercentLeft() }
          </div>
          {this.renderPercentRight()}
          {this.renderWarning()}
        </div>
      );
    }else if(this.props.type == "amount"){
      return (
        <div className="TCEvaluationCellsAmount">
          <div className="TCEvaluationCellsAmountShaded"
               style={{"width": (this.props.value/this.props.max)*100+"%"}}>
               { this.renderAmountLeft() }
          </div>
          {this.renderAmountRight()}
        </div>
      );
    }else{
      return (
        <div className="TCEvaluationCellsText">
          <div className="TCEvaluationCellsTextCont">
            {this.props.value}
          </div>
        </div>
      );
    }
  }
}

export default TCEvaluationCells;
