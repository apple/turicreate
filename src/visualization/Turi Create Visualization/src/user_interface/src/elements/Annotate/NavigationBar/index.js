import React, { Component } from 'react';
import style from './index.module.css';


class NavigationBar extends Component{

constructor(props){
  super(props);
  this.state = {value:''};
  this.handleChange = this.handleChange.bind(this);
  this.enterPressJumpRow = this.enterPressJumpRow.bind(this);
}

handleChange(event){
  this.setState({value: event.target.value});
};

enterPressJumpRow(e) {
    if(e.keyCode == 13){

      if(this.state.value != null){
      var image_number = parseInt(this.state.value,10);
      this.setState({value:''});
      
      this.getIndex(image_number)
      if (this.props.infiniteScroll)
      	{
      	this.props.toggleInfiniteScroll();
        }

      
      }
    }
  }

  getIndex = (index) => {
    if (index >= 0) {
      this.props.updateIncrementalCurrentIndex(index);
      this.props.getData((index-1 ), index + 1);
      this.props.getAnnotations((index - 1), index + 1);
    }
  }
 
render(){
	return (
		<div className="BottomBar">
		<div className="jumpToImageContainer">
		<input className="imageNumber" type="text" value={this.state.value} onChange={this.handleChange} id="imgNum" onKeyDown={this.enterPressJumpRow} placeholder="Image #"/>
		</div>
		</div>
		);
	}
};

export default NavigationBar;