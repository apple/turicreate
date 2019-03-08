import React, { Component } from 'react';
import style from './index.module.scss';

import ToggleButton from './ToggleButton/index';

import swap from './assets/swap.svg';

class StatusBar extends Component {
  render(){
    return (
      <div className={style.StatusBar}>
        <div className={style.StatusBarMetrics}>
            Total: {this.props.total} | Annotated: {this.props.annotated}
        </div>
        <div>
            <img src={swap}
                 className={style.SwapButton}
                 onClick={this.props.toggleInfiniteScroll.bind(this)}/>
            <ToggleButton hideAnnotated={this.props.hideAnnotated}
                          toggleHideAnnotated={this.props.toggleHideAnnotated.bind(this)}/>
            <div className={style.SwapButtonHelperText}>
                Hide Annotated image
            </div>

        </div>
      </div>
    );
  }
}

export default StatusBar;