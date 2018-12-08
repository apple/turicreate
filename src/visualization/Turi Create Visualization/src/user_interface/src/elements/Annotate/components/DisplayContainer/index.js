import React, { Component } from 'react';
import './index.css';
import AnnotateToolBar from '../AnnotateToolBar';
import ImageInfiniteScroll from '../ImageInfiniteScroll';
import AnnotationContainer from '../AnnotationContainer';

class DisplayContainer extends Component {
	constructor(props){
		super(props)
		this.state = {
			"display_control":true
		}
	}

	changeValue = () => {
		this.setState({"display_control": !this.state.display_control});
	}

	displayContainerControl = () => {
		if(this.state.display_control){
			return(<AnnotationContainer />);
		}else{
			return(<ImageInfiniteScroll />);
		}
	}

  render() {
    return (
      <div class="DisplayContainer">
      	<AnnotateToolBar changeValue={this.changeValue.bind(this)}/>
      	{this.displayContainerControl()}
      </div>
    );
  }
}

export default DisplayContainer;
