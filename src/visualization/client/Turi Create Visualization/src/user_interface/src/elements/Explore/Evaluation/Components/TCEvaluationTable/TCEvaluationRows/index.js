import React, { Component } from 'react';
import './index.scss';
import TCEvaluationCells from './TCEvaluationCells';

class TCEvaluationRows extends Component {

  constructor(props){
    super(props)
    this.state = {
      "hovered":false
    }
  }

  renderSelected = () => {
    if(this.props.selected){
      return (
        {"background": "#D6E7FB"}
      );
    }
  }

  renderAccuracy = () => {
    if(this.props.accuracy_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.correct/(this.props.data.correct+this.props.data.incorrect)}
                           selected={this.props.selected}
                           hovered={this.state.hovered}/>
      )
    }
  }

  renderPrecision = () => {
    if(this.props.precision_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.precision}
                           selected={this.props.selected}
                           hovered={this.state.hovered}/>
      )
    }
  }

  renderRecall = () => {
    if(this.props.recall_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.recall}
                           selected={this.props.selected}
                           hovered={this.state.hovered}/>
      );
    }
  }

  renderF1Score = () => {
    if(this.props.f1_score_visible){
      return (
        <TCEvaluationCells type="percent"
                           value={this.props.data.f1_score}
                           selected={this.props.selected}
                           hovered={this.state.hovered}/>
      );
    }
  }

  mouse_enter = () => {
    this.setState({"hovered":true});
  }

  mouse_leave = () => {
    this.setState({"hovered":false});
  }

  render() {
    return (
      <div className="TCEvaluationRows"
           style={this.renderSelected()}
           onClick={this.props.onclick.bind(this, this.props.data.name)}
           onMouseEnter={this.mouse_enter.bind(this)}
           onMouseLeave={this.mouse_leave.bind(this)}>
        <div className="TCEvaluationRowContainer">
          <TCEvaluationCells value={this.props.data.name}/>
          <TCEvaluationCells type="images"
                             value={this.props.data.correct_images}/>
          {this.renderAccuracy()}
          {this.renderPrecision()}
          {this.renderRecall()}
          {this.renderF1Score()}
          <TCEvaluationCells type="amount"
                             value={this.props.data.num_examples}
                             max={this.props.max_value}
                             selected={this.props.selected}
                             hovered={this.state.hovered}/>
        </div>
      </div>
    );
  }
}

export default TCEvaluationRows;
