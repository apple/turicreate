import React, { Component } from 'react';
import './index.scss';
import sort_icon from './assets/sort_icon.png';
import {SortType} from '../TCEvaluationSortType'

class TCEvaluationHelperSort extends Component {
  changeSelected = (event) => {
    this.props.setSelect(event.target.value)
  }

  sortIconHandler = () => {
    if(this.props.selected == SortType.HighToLow){
      this.props.setSelect(SortType.LowToHight)
    }else if(this.props.selected == SortType.LowToHight){
      this.props.setSelect(SortType.HighToLow)
    }

    if(this.props.selected == SortType.AlphabeticalAscending){
      this.props.setSelect(SortType.AlphabeticalDescending)
    }else if(this.props.selected == SortType.AlphabeticalDescending){
      this.props.setSelect(SortType.AlphabeticalAscending)
    }
  }

  sortIconOrientation  = () => {
    if(this.props.selected == SortType.HighToLow ||
       this.props.selected == SortType.AlphabeticalAscending){
      return {"transform": "rotateX(0deg)"}
    }else{
      return {"transform": "rotateX(180deg)"}
    }
  }

  render() {
    return (
      <div className="TCEvaluationHelperSort">
        <img height={7}
             onClick={this.sortIconHandler.bind(this)}
             src={sort_icon}
             style={this.sortIconOrientation()}/>
        <span>Sort By:</span>
        <select onChange={this.changeSelected.bind(this)}
                value={this.props.selected}>
          <option value={SortType.HighToLow}>High to Low</option>
          <option value={SortType.LowToHight}>Low to High</option>
          <option value={SortType.AlphabeticalAscending}>A - Z</option>
          <option value={SortType.AlphabeticalDescending}>Z - A</option>
        </select>
      </div>
    );
  }
}

export default TCEvaluationHelperSort;