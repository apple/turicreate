import React, { Component } from 'react';
import style from './index.module.scss';

class LabelModal extends Component {
  constructor(props) {
    super(props);
    this.labelInput = React.createRef();
  }

  componentDidMount = () => {
    this.labelInput.focus();
  }

  renderError = () => {
    if(this.props.error){
      return this .props.error;
    }else{
      return '\xa0';
    }
  }
  
  render() {
    return (
      <div>
        <div className={style.LabelModal}
              onClick={this.props.closeLabelModal.bind(this)}>
          Label Modal
        </div>
        <div className={style.modalWindow}>
          <div className={style.modalWindowHeader}>
            Add Labels
          </div>
          <div className={style.modalWindowDescription}>
            Add a label to annotate your data with.
          </div>
          <div>
            <div className={style.errorMessage}>
              {this.renderError()}
            </div>
            <input type="text"
                   className={style.modalInput}
                   placeholder="Annotation Label"
                   onChange={this.props.handleEventLabelModalValue.bind(this)}
                   ref={(input) => { this.labelInput = input; }}/>
          </div>
          <div className={style.buttonContainer}>
            <button className={style.createButton}
                    onClick={this.props.createLabel.bind(this, this.props.LabelModalValue)}>
              Create
            </button>
            <button className={style.cancelButton}
                    onClick={this.props.closeLabelModal.bind(this)}>
              Cancel
            </button>
          </div>
        </div>
      </div>
    );
  }
}

export default LabelModal;