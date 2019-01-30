import React, { Component } from 'react';
import './index.scss';

import export_img from './assets/export.png';

class TCEvaluationImageComponent extends Component {
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
      this.setState({"height" : 130})
    }else{
      this.setState({"width" : 130})
    }
  }

  render() {

    return (
      <div className="TCEvaluationImageComponent">
        <img src={"data:image/"+this.props.src.format+";base64,"+this.props.src.data}
             className="TCEvaluationImageComponentImage"
             ref={this.imgEl}
             width={this.state.width}
             height={this.state.height}
             onLoad={this.setHeight.bind(this)} />
        <div className="TCEvaluationImageComponentExport">
          <img src={export_img}
               width={10}
               height={10} />
        </div>
      </div>
    );
  }
}

export default TCEvaluationImageComponent;




