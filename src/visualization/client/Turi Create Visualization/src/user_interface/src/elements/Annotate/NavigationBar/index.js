import React, { Component } from 'react';
import style from './index.module.scss';
import ErrorBar from '../ErrorBar';
import * as d3 from "d3";

class NavigationBar extends Component{
  constructor(props){
    super(props);
    this.formatDecimal = d3.format(".0f")
    this.state = {value:''};
    this.handleChange = this.handleChange.bind(this);
    this.enterPressJumpRow = this.enterPressJumpRow.bind(this);
  }

  handleChange(event){
    this.setState({value: event.target.value});
  };

  enterPressJumpRow(e) {
    if(e.keyCode == 13){
      var image_number = parseInt(this.state.value,10);
      this.setState({value:''});
      if(this.state.value != null){
        if (image_number < 0 || image_number >= this.props.numElements || isNaN(image_number)) {
          this.props.setError(`Input '${this.state.value}', is invalid.`)
        } else {
          this.props.updateIncrementalCurrentIndex(image_number);
          this.props.getData((image_number-1 ), image_number + 1);
          this.props.getAnnotations((image_number - 1), image_number + 1);
          this.props.getSimilar(image_number);

          if (this.props.infiniteScroll){
            this.props.toggleInfiniteScroll();
          }
        }
      }
    }
  }

  renderExtractingFeatures = () => {
    if (this.props.percentage < 1) {
      return (
        <div className={style.extractingFeatures}>
          Extracting Features: {this.formatDecimal(this.props.percentage*100)}%
        </div>
      );
    }else if(this.props.percentage < 2) {
      return (
        <div className={style.extractingFeatures}>
          Initializing Similarity Model...
        </div>
      );
    }
  }

  render(){
    return (
      <div className={style.BottomBar}>
        <div className={style.jumpToImageContainer}>
          <input className={style.imageNumber}
                 id={"imgNum"}
                 onChange={this.handleChange}
                 onKeyDown={this.enterPressJumpRow}
                 placeholder={"Jump to image"}
                 type={"text"}
                 value={this.state.value}/>
        </div>
        {this.renderExtractingFeatures()}
      </div>
    );
  }
};

export default NavigationBar;