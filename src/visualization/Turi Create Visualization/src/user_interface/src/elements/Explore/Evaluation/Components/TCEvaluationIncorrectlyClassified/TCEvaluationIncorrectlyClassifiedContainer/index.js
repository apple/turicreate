import React, { Component } from 'react';
import './index.scss';
import dropdown from './assets/dropdown.png';

import TCEvaluationImageComponent from '../../TCEvaluationImageComponent';

class TCEvaluationIncorrectlyClassifiedContainer extends Component {
  constructor(props){
    super(props)
    this.state ={
      "open":false
    }
  }

  changeState = () => {
    this.setState({"open": !this.state.open});
  }

  displayImageContainer = () => {
    if(this.state.open){
      return(
        <div className="TCEvaluationIncorrectlyClassifiedContainerImages">
          <div className="TCEvaluationIncorrectlyClassifiedContainerFlex">
            {this.props.images.map((image, index) => {
              return(
                <div>
                  <TCEvaluationImageComponent src={image}/>
                </div>
              ); 
            })}
          </div>
        </div>
      )
    }
  }

  displayArrow(){
    if(this.state.open){
      return (
        <div className="TCEvaluationIncorrectlyClassifiedContainerTitle"
             onClick={this.changeState.bind(this)}>
          <span className="TCEvaluationIncorrectlyClassifiedContainerNumber">
            {this.props.images.length} &nbsp;
          </span>images incorrectly classified as {this.props.label} 
          <img className="TCEvaluationIncorrectlyClassifiedContainerDropDown"
               height={6}
               src={dropdown}
               onClick={this.changeState.bind(this)}/>
        </div>
      );
    }else{
      return (
        <div className="TCEvaluationIncorrectlyClassifiedContainerTitle"
             onClick={this.changeState.bind(this)}>
          <span className="TCEvaluationIncorrectlyClassifiedContainerNumber">
            {this.props.images.length} &nbsp;
          </span>images incorrectly classified as {this.props.label}
          <img className="TCEvaluationIncorrectlyClassifiedContainerDropUp"
               height={6}
               src={dropdown}/>
        </div>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationIncorrectlyClassifiedContainer">
        {this.displayArrow()}
        {this.displayImageContainer()}
      </div>
    );
  }
}

export default TCEvaluationIncorrectlyClassifiedContainer;