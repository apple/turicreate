import React, { Component } from 'react';
import './index.scss';
import right from './assets/right_arrow.png';

class TCEvaluationConfusionMenu extends Component {
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
      <div className="TCEvaluationConfusionMenu">
        <div className="TCEvaluationConfusionMenuContainer">
          {this.props.labels.map((label, index) => {
            if(this.state.selected_element == index){
              return( 
                <div className="TCEvaluationConfusionMenuItemSelected">
                  <div className="TCEvaluationConfusionMenuItemBaseClass">
                    {label.base}
                  </div>
                  <div className="TCEvaluationConfusionMenuItemArrow">
                  <img height={10} src={right}/>
                  </div>
                  <div className="TCEvaluationConfusionMenuItemConfused">
                    {label.confused}
                  </div>
                  <div className="TCEvaluationConfusionMenuItemNumItems">
                    <div className="TCEvaluationConfusionMenuItemNumItemWrapper">
                      {label.num}
                    </div>
                  </div>
                </div>
              );
            } else {
              return( 
                <div className="TCEvaluationConfusionMenuItem"
                     onClick={this.changeIndex.bind(this, index)}>
                  <div className="TCEvaluationConfusionMenuItemBaseClass">
                    {label.base}
                  </div>
                  <div className="TCEvaluationConfusionMenuItemArrow">
                  <img height={10} src={right}/>
                  </div>
                  <div className="TCEvaluationConfusionMenuItemConfused">
                    {label.confused}
                  </div>
                  <div className="TCEvaluationConfusionMenuItemNumItems">
                    <div className="TCEvaluationConfusionMenuItemNumItemWrapper">
                      {label.num}
                    </div>
                  </div>
                </div>
              );
            }
          })}
        </div>
      </div>
    );
  }
}

export default TCEvaluationConfusionMenu;