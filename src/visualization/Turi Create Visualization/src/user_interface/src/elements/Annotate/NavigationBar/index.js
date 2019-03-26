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
      var image_number = this.state.value;
      this.setState({value:''});
        
      if (this.props.infiniteScroll)
      	{
      	this.props.toggleInfiniteScroll(this);
        }
      this.props.updateIncrementalCurrentIndex(image_number-1);
      this.props.getData(image_number-1, image_number+1);
      this.props.getAnnotation(image_number-1, image_number+1);
      }
    }
  }
};
 
render(){
	return (
		<div className="BottomBar">
		<div className="jumpToImageContainer">
		<input className="imageNumber" type="text" value={this.state.value} onChange={this.handleChange} id="imgNum" onKeyDown={this.enterPressJumpRow} placeholder="Image #"/>
		</div>
		</div>
		);
	}
}

export default NavigationBar;