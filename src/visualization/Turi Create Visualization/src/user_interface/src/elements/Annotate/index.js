import React, { Component } from 'react';
import style from './index.module.scss';

import InfiniteScroll from './InfiniteScroll';
import StatusBar from './StatusBar';
import SingleImage from './SingleImage';
import LabelContainer from './LabelContainer';
import LabelModal from './LabelModal';
import ErrorBar from './ErrorBar';

const DEFAULT_NUM_EXPECTED = 10;

/* TODO: Indicator to Show how many Images were Annotated */
/* TODO: Render Labels on Images */

class Annotate extends Component {
  constructor(props){
    super(props);
    this.state = {
      infiniteScroll: false,
      hideAnnotated: false,
      labelModal: false,
      totalError: "",
      labelCreationError: null,
      incrementalCurrentIndex: 0,
      LabelModalValue: "",
      /* TODO: Labels will be Populated from MetaData */
      labels:[
        {
          name: "dog",
          num_annotated: 0,
          num_expected: DEFAULT_NUM_EXPECTED
        },
        {
          name: "melons",
          num_annotated: 0,
          num_expected: DEFAULT_NUM_EXPECTED
        },
        {
          name: "cats",
          num_annotated: 0,
          num_expected: DEFAULT_NUM_EXPECTED
        },
      ]
    }
  }

  clearError = () => {
    this.setState({
      totalError: ""
    });
  }

  updateIncrementalCurrentIndex = (index) => {
    this.setState({
      incrementalCurrentIndex: index
    })
  }

  handleEventLabelModalValue = (e) => {
    this.setState({
      LabelModalValue: e.target.value
    });
  }
  
  openLabelModal = () => {
    this.setState({
      labelCreationError: null,
      labelModal: true,
      LabelModalValue: ""
    });
  }

  closeLabelModal = () => {
    this.setState({
      labelCreationError: null,
      labelModal: false,
      LabelModalValue: ""
    });
  }

  createLabel = (label) => {
    const notDuplicateLabel = this.state.labels.map(l => (l.name != label))
                                            .reduce((acc, b) => (acc && b), true);

    if(!notDuplicateLabel){
      this.setState({
        labelCreationError: `${label} already exists as a label name`
      });
      return;
    }

    var mutatorLabels = this.state.labels;
    
    mutatorLabels.push({
      name: label,
      num_annotated: 0,
      num_expected: DEFAULT_NUM_EXPECTED
    });

    this.setState({
      labels: mutatorLabels,
      labelCreationError: null,
      labelModal: false,
      LabelModalValue: ""
    });
  }

  toggleInfiniteScroll = () => {
    this.setState({
      infiniteScroll: !this.state.infiniteScroll
    });
  }

  toggleHideAnnotated = () => {
    this.setState({
      hideAnnotated: !this.state.hideAnnotated
    });
  }

  renderMainContent = () => {
    if(this.state.infiniteScroll) {
      return (
        <InfiniteScroll numElements={this.props.total}
                        hideAnnotated={this.state.hideAnnotated}
                        incrementalCurrentIndex={this.state.incrementalCurrentIndex}
                        updateIncrementalCurrentIndex={this.updateIncrementalCurrentIndex.bind(this)} />
      );
    } else {
      return (
        <SingleImage/>
      );
    }
  }

  renderModal = () => {
    if(this.state.labelModal){
      return(
        <LabelModal closeLabelModal={this.closeLabelModal.bind(this)}
                    createLabel={this.createLabel.bind(this)}
                    error={this.state.labelCreationError}
                    LabelModalValue={this.state.LabelModalValue}
                    handleEventLabelModalValue={this.handleEventLabelModalValue.bind(this)}/>
      )
    }
  }

  renderError = () => {
    if(this.state.totalError){
      return (<ErrorBar message={this.state.totalError}
                        clearError={this.clearError.bind(this)}/>);
    }
  }

  render() {
    return (
      <div className={style.Annotate}>
        {this.renderModal()}
        {this.renderError()}
        <div>
            {/* Add Annotated to Protobuf Message */}
            <StatusBar annotated={12}
                       total={this.props.total}
                       infiniteScroll={this.state.infiniteScroll}
                       toggleInfiniteScroll={this.toggleInfiniteScroll.bind(this)}
                       hideAnnotated={this.state.hideAnnotated}
                       toggleHideAnnotated={this.toggleHideAnnotated.bind(this)}/>

            {this.renderMainContent()}
        </div>
        <div className={style.leftBar}>
        <LabelContainer labels={this.state.labels}
                        openLabelModal={this.openLabelModal.bind(this)}
                        closeLabelModal={this.closeLabelModal.bind(this)}/>
        </div>
      </div>
    );
  }
}

export default Annotate;