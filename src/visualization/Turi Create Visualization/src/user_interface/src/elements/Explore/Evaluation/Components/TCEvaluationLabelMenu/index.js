import React, { Component } from 'react';
import './index.scss';

class TCEvaluationLabelMenu extends Component {
  constructor(props){
    super(props)
    this.state = {
      "selected_element": 0
    }
  }

  changeIndex = (index) => {
    this.setState({"selected_element": index});
  }

  render() {
    return (
      <div className="TCEvaluationLabelMenu">
        <div className="TCEvaluationLabelMenuContainer">
          {this.props.labels.map((label, index) => {
            if(this.state.selected_element == index){
              return( 
                <div className="TCEvaluationLabelMenuItemSelected">
                  {label}
                </div>
              );
            } else {
              return( 
                <div className="TCEvaluationLabelMenuItem"
                     onClick={this.changeIndex.bind(this, index)}>
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