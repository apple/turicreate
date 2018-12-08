import React, { Component } from 'react';
import './index.css';
import SelectableImages from '../SelectableImages';
import AnnotationImageDisplay from '../AnnotationImageDisplay';
import leftArow from './leftArrow.png';
import rightArrow from './rightArrow.png';

class AnnotationContainer extends Component {
	constructor(props){
		super(props);

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
    return (
    	<div>
	      <div className="AnnotationContainer">
	      	<div className="AnnotationSelection">
	      		<div className="AnnotationText">
	      			Top Similar Images
	      		</div>
	      		<div className="AnnotationScrollable"
	      				 style={{"height": (this.state.height - 193) }}>
		      		<SelectableImages />
		      		<SelectableImages />
		      		<SelectableImages />
		      		<SelectableImages />
		      		<SelectableImages />
		      		<SelectableImages />
		      	</div>
	      	</div>
	      </div>
	      <div className="AnnotationImage"
	      		 style={{"height": (this.state.height - 115),
	      		 				 "width": (this.state.width - 430) }}>
	      	<div className="AnnotationImageLeft"
	      			 style={{"height":this.state.height - 115}}>
      			<img className="AnnotationImageLeftImage" style={{"height":40}} src={leftArow}/>
	      	</div>
	      	<AnnotationImageDisplay />
	      	<div className="AnnotationImageRight"
	      			 style={{"height":this.state.height - 115}}>
	      		<img className="AnnotationImageRightImage" style={{"height":40}} src={rightArrow}/>
	      	</div>
	      </div>
	     </div>
    );
  }
}

export default AnnotationContainer;
