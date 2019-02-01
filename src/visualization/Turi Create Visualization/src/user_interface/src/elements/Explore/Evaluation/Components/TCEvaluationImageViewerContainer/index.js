import React, { Component } from 'react';
import './index.scss';

import right_arrow from './assets/right.svg';

import TCEvaluationImageComponent from '../TCEvaluationImageComponent';

class TCEvaluationImageViewerContainer extends Component {
  render() {
    return (
      <div className="TCEvaluationImageViewerContainer">
        <div className="TCEvaluationImageViewerContainerTitle">
          <img src={right_arrow}
               className="TCEvaluationImageViewerContainerPointer"
               height={13}
               onClick={this.props.reset.bind(this)} />
          &nbsp;
          {this.props.data.count} &nbsp; {this.props.data.actual}s confused as {this.props.data.predicted}s
        </div>
        <div className="TCEvaluationImageViewerContainerImages">
          { 
            this.props.data.images.map((data, index) => {
              return(
                <div className="TCEvaluationImageComponentWrapper">
                  <TCEvaluationImageComponent src={data} />
                </div>
              )
            })
          }
        </div>
      </div>
    );
  }
}

export default TCEvaluationImageViewerContainer;
