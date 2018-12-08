import React, { Component } from 'react';
import './index.css';

import SelectableImages from '../SelectableImages';

class ImageInfiniteScroll extends Component {
	constructor(props){
		super(props)

		this.state = {
			"width": window.innerWidth,
			"height": window.innerHeight
		}
	}
	componentDidMount() {
	  window.addEventListener('resize', this.resize)
	}

	componentWillUnmount() {
	  window.removeEventListener('resize', this.resize)
	}

	resize = () => {
		this.setState({
			"width": window.innerWidth,
			"height": window.innerHeight
		});
	}

  render() {

  	var selectableElements = [];

  	for(var x = 0; x < 100; x++){
  		selectableElements.push(<SelectableImages key={"select" + x} />);
  	}
    return (
      <div class="ImageInfiniteScroll" 
      		 style={{"height": this.state.height-115}}>
      	<div class="ImageInfiniteScrollImageContainer"
      			 style={{"height": this.state.height-169, "width": this.state.width-245}}>
      			 {selectableElements}
      	</div>
      	<div class="ImageInfiniteScrollHelpText">
      		Press ‘Ctrl + S’ to select multiple images
      	</div>
      </div>
    );
  }
}

export default ImageInfiniteScroll;
