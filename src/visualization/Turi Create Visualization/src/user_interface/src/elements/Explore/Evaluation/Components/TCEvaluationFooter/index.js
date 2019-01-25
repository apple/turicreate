import React, { Component } from 'react';
import './index.scss';

import down_arrow from './assets/down.svg';
import cancel from './assets/cancel.svg';
import TCEvaluationConfusionTable from '../TCEvaluationConfusionTable';
import TCEvaluationImageViewerContainer from '../TCEvaluationImageViewerContainer';

class TCEvaluationFooter extends Component {

  cssOpenStyleContainer = () => {
    if(this.props.footer_open){
      return {"min-height": parseInt((window.innerHeight/2), 10) + "px"}
    }
  }

  cssOpenStyleIcon = () => {
    if(this.props.footer_open){
      return {"transform":"rotate(-180deg)"}
    }
  }

  filterData = (element) => {
    return element.actual == this.props.selected_actual && element.predicted == this.props.selected_prediction;
  }

  render_table = () => {
    if((this.props.selected_actual != null) && (this.props.selected_prediction != null)){
      return (
        <div>
          <TCEvaluationImageViewerContainer reset={this.props.selectRowConfusions.bind(this, null, null)}
                                            data={this.props.considerations.filter(this.filterData)[0]}/>
        </div>
      );
    } else {
      return (
        <div>
          <TCEvaluationConfusionTable considerations={this.props.considerations}
                                      filter_confusion={this.props.filter_confusion}
                                      sort_by_confusions={this.props.sort_by_confusions}
                                      sort_direction_confusions={this.props.sort_direction_confusions}
                                      updateSortByConfusion={this.props.updateSortByConfusion.bind(this)}
                                      selectRowConfusions={this.props.selectRowConfusions.bind(this)}/>
        </div>
      );
    }
  }

  resetFilter = () => {
    this.props.row_click(null);
  }

  renderLabel = () => {
    if(this.props.filter_confusion != null){
      return(
        <div className="TCEvaluationFooterLabel">
          <div className="TCEvaluationFooterLabelIcon">
            <span>
              Actual :&nbsp;
            </span>
                
            {this.props.filter_confusion}
            &nbsp;
            &nbsp;
            <img src={cancel} 
                 onClick={this.resetFilter.bind(this)}/>
          </div>
        </div>
      );
    }
  }

  render() {
    return (
      <div className="TCEvaluationFooter"
           style={this.cssOpenStyleContainer()}>
        <div className="TCEvaluationFooterContainer"
             onClick={this.props.updateFooterOpen.bind(this)}>
          <div className="TCEvaluationFooterText">
            <div>
              Errors
            </div>
            {this.renderLabel()}
          </div>
          <div className="TCEvaluationFooterCarret">
            <img src={down_arrow}
                 className="TCEvaluationFooterCarretImage"
                 style={this.cssOpenStyleIcon()}/>
          </div>
        </div>
        {this.render_table()}
      </div>
    );
  }
}

export default TCEvaluationFooter;
