import React, { Component } from 'react';
import './index.scss';

import * as d3 from "d3";

import TCEvaluationImageCells from '../../../TCEvaluationImageCells';

class TCEvaluationConfusionCell extends Component {
	constructor(props){
    super(props)
    this.formatDecimal = d3.format(".0f")
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

  render() {
	if(this.props.type == "images"){
      return (
        <div className="TCEvaluationConfusionCellImage">
          {this.props.value.slice(0, 100).map((data, index) => {
            return(
              <TCEvaluationImageCells value={data}/>
            )
          })}
        </div>
      );
    }else if(this.props.type == "percent"){
      return (
        <div className="TCEvaluationConfusionCell">

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
        <div className="TCEvaluationConfusionCellText">
		{this.props.value}
        </div>
      );
	}
  }
}

export default TCEvaluationConfusionCell;
