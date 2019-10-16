import React, { Component } from 'react';
import style from './index.module.scss';

import ImageContainer from './ImageContainer';

import imgProperties from './ImageContainer/index.module.scss';

// has to be a multiple of 3 to work properly
const CELL_PADDING = 3;

class InfiniteScroll extends Component {
  constructor(props) {
    super(props);
    this.currentComponent = React.createRef()
    this.state = {
      componentHeight: window.innerHeight,
      componentWidth: window.innerWidth,
      startValue:0,
      enableScroll: true
    }
  }

  componentDidMount() {
    this.updateDimensions();
    const scaling = this.numRenderedbox();
    const tempEndVal = scaling.numX * (scaling.numY + 2 * CELL_PADDING) + this.state.startValue;
    const endVal = (tempEndVal > this.props.numElements)?this.props.numElements:tempEndVal;
    this.props.getData(this.state.startValue, endVal);
    this.props.getAnnotations(this.state.startValue, endVal);
    window.addEventListener("resize", this.updateDimensions.bind(this));
  }

  componentWillUnmount() {
    window.removeEventListener("resize", this.updateDimensions.bind(this));
  }

  updateDimensions = () => {
    const $this = this;
    // This is measuring the DOM so we need to wait for the previous render
    window.requestAnimationFrame(function() {
      $this.setState({
        componentHeight: $this.currentComponent.current.getBoundingClientRect().height,
        componentWidth: $this.currentComponent.current.getBoundingClientRect().width
      });
    });
  }

  handleScroll = (event) => {
    if(this.state.enableScroll){
      const imgSize = parseInt(imgProperties.imgSize, 10);
      const imgMargin = parseInt(imgProperties.imgMargin, 10);
      const imgOffset = 2 * imgMargin + imgSize;

      const scrollOffset = (imgOffset * CELL_PADDING)

      // Paragraph comment on how this works
      const scrollDownTrigger = (imgOffset * (5 / 3) * CELL_PADDING);
      const scrollUpTrigger = (imgOffset * (1 / 3) * CELL_PADDING);

      const scaling = this.numRenderedbox();
      const endValue = scaling.numX * (scaling.numY + 2 * CELL_PADDING) + this.state.startValue;

      if(endValue < this.props.numElements) {
        if(scrollDownTrigger < this.currentComponent.current.scrollTop){
          this.setState({
            startValue: (scaling.numX * CELL_PADDING) + this.state.startValue,
            enableScroll: false
          }, function(){
            const tempEndVal = scaling.numX * (scaling.numY + 2 * CELL_PADDING) + this.state.startValue;
            const endVal = (tempEndVal > this.props.numElements)?this.props.numElements:tempEndVal;
            this.props.getData(this.state.startValue, endVal);
            this.props.getAnnotations(this.state.startValue, endVal);
            this.currentComponent.current.scrollBy(0, -scrollOffset);
            this.setState({
              enableScroll: true
            })
          }, this);
        }
      }

      if(this.state.startValue > 0) {
        if(scrollUpTrigger > this.currentComponent.current.scrollTop){
          const startVal = this.state.startValue - (scaling.numX * CELL_PADDING)
          this.setState({
            startValue: (startVal < 0)?0:startVal,
            enableScroll: false
          }, function(){
            this.currentComponent.current.scrollBy(0, scrollOffset);
            const tempEndVal = scaling.numX * (scaling.numY + 2 * CELL_PADDING) + this.state.startValue;
            const endVal = (tempEndVal > this.props.numElements)?this.props.numElements:tempEndVal;
            this.props.getData(this.state.startValue, endVal);
            this.props.getAnnotations(this.state.startValue, endVal);
            this.setState({
              enableScroll: true
            })
          }, this);
        }
      }
    }
  }

  numRenderedbox = () => {
    const imgSize = parseInt(imgProperties.imgSize, 10);
    const imgMargin = parseInt(imgProperties.imgMargin, 10);
    const imgOffset = 2 * imgMargin + imgSize;

    return {
      numX: Math.floor((this.state.componentWidth)/imgOffset),
      numY: Math.floor((this.state.componentHeight)/imgOffset)
    }
  }

  renderBoxes = () => {
    var boxes = [];
    const scaling = this.numRenderedbox();
    const tempEndVal = scaling.numX * (scaling.numY + 2 * CELL_PADDING) + this.state.startValue;
    const endValue = (tempEndVal > this.props.numElements)?this.props.numElements:tempEndVal;
    const selectedBoxes = Object.keys(this.props.infiniteSelected);

    // TODO: filter annotation  array use `x` as index into the array
    for(var x = this.state.startValue; x < endValue; x++ ){
      boxes.push(<ImageContainer key={`images_${x}`}
                                 selected={selectedBoxes.includes(x.toString())}
                                 src={this.props.imageData[x]}
                                 addToSelected={this.props.addToSelected.bind(this)}
                                 removeSelected={this.props.removeSelected.bind(this)}
                                 annotation={this.props.annotationData[x]}
                                 toggleInfiniteScroll={this.props.toggleInfiniteScroll.bind(this)}
                                 updateIncrementalCurrentIndex={this.props.updateIncrementalCurrentIndex.bind(this)}
                                 type={this.props.type}
                                 value={x}/>)
    }

    return boxes;
  }

  onBackClick = (e) => {
    if (e.shiftKey) {

    }else{
      this.props.removeSelected();
    }
  }

  render() {
    return (
      <div className={`${style.InfiniteScroll} ${style.noselect}`}
           ref={this.currentComponent}
           onScroll={this.handleScroll}
           onClick={this.onBackClick.bind(this)}>
        {this.renderBoxes()}
      </div>
    );
  }
}

export default InfiniteScroll;
