import React, { Component } from 'react';
import './index.scss';

class TCEvaluationSummaryImage extends Component {
  constructor(props){
  	super(props);
  	this.imgEl = React.createRef();
  	this.state = {
  		"width": "auto",
  		"height": "auto"
  	}
  }

  setHeight = () =>{
  	const imageWidth = this.imgEl.current.clientWidth;
  	const imageHeight = this.imgEl.current.clientHeight;
  	if(imageWidth > imageHeight){
  		this.setState({"height" : 47})
  	}else{
  		this.setState({"width" : 47})
  	}
  }

  render() {
    return (
      <div className="TCEvaluationSummaryImage">
       <img src={this.props.src}
       		ref={this.imgEl}
       		width={this.state.width}
       		height={this.state.height}
       		onLoad={this.setHeight.bind(this)} />
      </div>
    );
  }
}

export default TCEvaluationSummaryImage;