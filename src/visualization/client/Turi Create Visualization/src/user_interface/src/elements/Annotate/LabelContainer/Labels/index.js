import React, { Component } from 'react';
import styles from './index.module.scss';
import ProgressBar from './ProgressBar';

class Label extends Component {
  renderApply = () => {
    if (!this.props.active || this.props.similarSelected.length !== 0) {
      return "Apply";
    } else {
      return "";
    }
  }

  render() {
    const percent_gradient = this.props.num_annotated * 100.0 / this.props.num_expected;

    var classes = [
    styles.Label
    ]

    if(this.props.active){
      classes.push(styles.LabelSelected);
    }

    return (
    <div className={classes.join(" ")}>
      <div className={styles.LabelSpacer}>
      <div className={styles.LabelNameAmount}>
        <div className={styles.LabelName}>
        <span title={this.props.name}>{this.props.name}</span>
        </div>
        <div className={styles.LabelAnnotatedAmount}>
        <span>{this.props.num_annotated}</span>
        </div>
      </div>
      <div className={styles.LabelApplyButton}
           onClick={this.props.onClick.bind(this)}>
        <span>
        {this.renderApply()}
        </span>
      </div>
      <div>
        <ProgressBar percent={percent_gradient}/>
      </div>
      </div>
    </div>
    );
  }
}

export default Label;
