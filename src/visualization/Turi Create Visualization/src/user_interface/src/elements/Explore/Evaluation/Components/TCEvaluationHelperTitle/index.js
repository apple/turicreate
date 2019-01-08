import React, { Component } from 'react';
import './index.scss';
import drop_down from './assets/drop_down.png';
import {ChartType} from '../TCEvaluationChartType';


class TCEvaluationHelperTitle extends Component {
  changeChart = (event) => {
    this.props.setSelect(event.target.value)
  }

  render() {
    return (
      <div className="TCEvaluationHelperTitle">
        <select onChange={this.changeChart.bind(this)}
                value={this.props.selected}>
          <option value={ChartType.F1Score}>F1 Score</option>
          <option value={ChartType.Recall}>Recall</option>
          <option value={ChartType.Precision}>Precision</option>
          <option value={ChartType.NumTrained}>Num Train</option>
        </select>
        <img height={8} src={drop_down}/>
        <div className="TCEvaluationHelperTitleCalculated">
          How is this Calculated?
        </div>
      </div>
    );
  }
}

export default TCEvaluationHelperTitle;




