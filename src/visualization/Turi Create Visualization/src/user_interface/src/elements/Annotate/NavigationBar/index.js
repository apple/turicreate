import React, { Component } from 'react';
import style from './index.module.css';


class NavigationBar extends Component{

enterPressJumpRow(e) {
    if(e.keyCode == 13){
      if(this.imgNum != null){
      	if (this.props.infiniteScroll)
      	{
      	this.props.toggleInfiniteScroll.bind(this);
        }
      var image_number = this.imgNum.value;
      this.imgNum.value = "";
      this.props.getData(image_number, image_number+1)
      this.props.getAnnotation(image_number, image_number+1)
    }
  }
};
 
	render(){
	return (
		<div className="BottomBar">
		<div className="jumpToImageContainer">
		<input className="imageNumber" id="imgNum" onKeyDown={this.enterPressJumpRow.bind(this)} placeholder="Image #"/>
		
		</div>
		</div>
		);
	}
}

export default NavigationBar;