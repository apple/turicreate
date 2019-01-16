import React, { Component } from 'react';
import './index.scss';
import accordian_icon from './assets/accordian_icon.png';
import drop_down_arrow from './assets/drop_down_arrow.png';

import * as d3 from "d3";

class TCEvaluationTopBar extends Component {
  constructor(props){
    super(props);
    this.formatComma = d3.format(",");
    this.formatPercentTitle = d3.format(",.0%") ;
    this.formatPercentDetails = d3.format(",.2%");
    this.state = {
      "accordian": true
    }
  }

  rotationHandler = () =>{
    this.setState({"accordian": !this.state.accordian});
  }

  renderAccuracy = () => {
    if(!this.props.left_selected && (this.props.selected_label != null)) {
      return (
        <div className="TCEvaluationTopBackButton"
             onClick={this.props.reset.bind(this)}>
            &lt;&nbsp;&nbsp;BACK
        </div>
      );
    }else{
      return (
        <div className="TCEvaluationTopBarAccuracy">
          <span className="TCEvaluationTopBarAccuracyValue">
            {this.formatPercentTitle(this.props.accuracy)}
          </span>
          <span className="TCEvaluationTopBarAccuracyLabel">
            accuracy
          </span>
        </div>
      );
    }
  }

  renderDropDown = () => {
    if(!this.props.left_selected && (this.props.selected_label != null)) {
      return (
        <div className="TCEvaluationTopLabelName">
          {this.props.selected_label}
        </div>
      );
    }else{
      let dropDownClassName = (this.state.accordian)?
                              "TCEvaluationTopBarMetricsAccordianDropDown":
                        ["TCEvaluationTopBarMetricsAccordianDropDown",
                         "TCEvaluationTopBarMetricsAccordianDropDownRotate"]
                         .join(" ");

      let textMetricsClassName = (this.state.accordian)?
                                "TCEvaluationTopBarMetricsTextContainer":
                              ["TCEvaluationTopBarMetricsTextContainer",
                               "TCEvaluationTopBarMetricsTextContainerExtended"]
                               .join(" ");

      return (
        <div className="TCEvaluationTopBarMetrics">
          <div className="TCEvaluationTopBarMetricsAccordian"
               onClick={this.rotationHandler.bind(this)}>
            <img src={accordian_icon}
                 height={22}
                 className="TCEvaluationTopBarMetricsAccordianIcon" />
            <img src={drop_down_arrow}
             height={5}
             className={dropDownClassName} />
          </div>
          <div className={textMetricsClassName}>
            <div className="TCEvaluationTopBarMetricsText">
              {this.formatComma(this.props.iterations)} iterations
            </div>
            <div className="TCEvaluationTopBarMetricsText">
              {this.props.model_type}
            </div>
          </div>
        </div>
      );
    }
  }

  render() {

    let barClassName = (this.state.accordian)?"TCEvaluationTopBar":
                                              ["TCEvaluationTopBar",
                                               "TCEvaluationTopBarExtened"]
                                               .join(" ");

    let accordianTextClassName = (this.state.accordian)?
                              "TCEvaluationTopBarAccuracyAccordianText":
                        ["TCEvaluationTopBarAccuracyAccordianText",
                         "TCEvaluationTopBarAccuracyAccordianTextDisplayed"]
                         .join(" ");

    let accordianTextHolderClassName = (this.state.accordian)?
                              "TCEvaluationTopBarAccuracyAccordianTextHolder":
                      ["TCEvaluationTopBarAccuracyAccordianTextHolder",
                       "TCEvaluationTopBarAccuracyAccordianTextHolderDisplayed"]
                       .join(" ");

    let accordianMetricClassName = (this.state.accordian)?
                              "TCEvaluationTopBarAccuracyAccordianMetricContainer":
                      ["TCEvaluationTopBarAccuracyAccordianMetricContainer",
                       "TCEvaluationTopBarAccuracyAccordianMetricContainerDisplayed"]
                       .join(" ");

    return (
      <div className={barClassName}>
        {this.renderAccuracy()}
        {this.renderDropDown()}
         <div className="TCEvaluationTopBarAccuracyAccordian">
           <div className={accordianTextHolderClassName}>
            <div className={accordianTextClassName}>
              {this.props.correct} out of {this.props.total} predicted correctly
            </div>
            <div className={accordianTextClassName}>
              {this.props.classes} Classes
            </div>
          </div>
          <div className={accordianMetricClassName}>
            <div className="TCEvaluationTopBarAccuracyAccordianMetric">
              <div className="TCEvaluationTopBarAccuracyAccordianMetricKey">
                number of iterations:
              </div>
              <div className="TCEvaluationTopBarAccuracyAccordianMetricValues">
                {this.formatComma(this.props.iterations)} iterations
              </div>
            </div>
            <div className="TCEvaluationTopBarAccuracyAccordianMetric">
              <div className="TCEvaluationTopBarAccuracyAccordianMetricKey">
                model type:
              </div>
              <div className="TCEvaluationTopBarAccuracyAccordianMetricValues">
                {this.props.model_type}
              </div>
            </div>
            <div className="TCEvaluationTopBarAccuracyAccordianMetric">
              <div className="TCEvaluationTopBarAccuracyAccordianMetricKey">
                precision:
              </div>
              <div className="TCEvaluationTopBarAccuracyAccordianMetricValues">
                {this.formatPercentDetails(this.props.precision)}
              </div>
            </div>
            <div className="TCEvaluationTopBarAccuracyAccordianMetric">
              <div className="TCEvaluationTopBarAccuracyAccordianMetricKey">
                recall:
              </div>
              <div className="TCEvaluationTopBarAccuracyAccordianMetricValues">
                {this.formatPercentDetails(this.props.recall)}
              </div>
            </div>
          </div>
        </div>
      </div>
    );
  }
}

export default TCEvaluationTopBar;
