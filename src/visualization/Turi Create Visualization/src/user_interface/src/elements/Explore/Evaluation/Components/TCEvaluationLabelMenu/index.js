import React, { Component } from 'react';
import './index.scss';

class TCEvaluationLabelMenu extends Component {
  render() {
    return (
      <div className="TCEvaluationLabelMenu">
        <div className="TCEvaluationLabelMenuContainer">
          {this.props.labels.map((label, index) => {
            if(label == this.props.selectedLabel){
              return( 
                <div className="TCEvaluationLabelMenuItemSelected">
                  {label}
                </div>
              );
            } else {
              return( 
                <div className="TCEvaluationLabelMenuItem"
                     onClick={this.props.changeLabel.bind(this, label)}>
                  {label}
                </div>
              );
            }
          })}
        </div>
      </div>
    );
  }
}

export default TCEvaluationLabelMenu;