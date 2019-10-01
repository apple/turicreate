import React, { Component } from 'react';
import './index.scss';

import TCEvaluationImageComponent from '../TCEvaluationImageComponent';

class TCEvaluationImageViewerContainer extends Component {
  render() {
    return (
      <div className="TCEvaluationImageViewerContainer">
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
