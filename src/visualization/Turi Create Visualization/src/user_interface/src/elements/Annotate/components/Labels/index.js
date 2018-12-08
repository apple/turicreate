import React, { Component } from 'react';
import './index.css';
import ProgressBar from '../ProgressBar';

class Label extends Component {
  render() {
    const percent_gradient 
      = this.props.num_annotated * 100.0 / this.props.num_expected;

    var classes = [
      "Label"
    ]

    if(this.props.active){
      classes.push("LabelSelected");
    }

    return (
      <div className={classes.join(" ")}>
        <div className="LabelSpacer">
          <div className="LabelNameAmount">
            <div className="LabelName">
              <span title={this.props.name}>{this.props.name}</span>
            </div>
            <div className="LabelAnnotatedAmount">
              <span>{this.props.num_annotated}/{this.props.num_expected}</span>
            </div>
          </div>
          <div className="LabelApplyButton">
            <span>
              Apply
            </span>
          </div>
          <div>
            <ProgressBar percent={percent_gradient}/>
          </div>
        </div>
      </div>
    );
  }
}

export default Label;
