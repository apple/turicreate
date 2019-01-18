import React, { Component } from 'react';
import './index.scss';

import TCEvaluationImageCellsHover from './TCEvaluationImageCellsHover';

class TCEvaluationImageCells extends Component {
  constructor(props){
    super(props)
    this.state = {
      "hover": false
    }
  }

  onHoverState = () => {
    this.setState({"hover": true});
  }

  offHoverState = () => {
    this.setState({"hover": false});
  }

  renderImage = () => {
    if(this.state.hover){
      return (
        <TCEvaluationImageCellsHover src={"data:image/"+this.props.value.format+";base64,"+this.props.value.data}/>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationImageCellsWrapper"
           onMouseEnter={this.onHoverState.bind(this)}
           onMouseLeave={this.offHoverState.bind(this)}>
        <div className="TCEvaluationImageCells">
          <img width={30}
               src={"data:image/"+this.props.value.format+";base64,"+this.props.value.data} />
        </div>
        {this.renderImage()}
      </div>
    );
  }
}

export default TCEvaluationImageCells;
