import React, { Component } from 'react';
import './index.scss';

class TCEvaluationImageCellsHover extends Component {
  render() {
    return (
      <div className="TCEvaluationImageCellsHover">
        <img width={200}
             src={this.props.src} 
	     alt="" />
      </div>
    );
  }
}

export default TCEvaluationImageCellsHover;
