import React, { PureComponent } from 'react';
import PropTypes from 'prop-types';
import Table from './Table';
import Row from './Row';
import Cell from './Cell';

var JSONPretty = require('react-json-pretty');
var elementResizeEvent = require('element-resize-event');

class StickyTable extends PureComponent {
  static propTypes = {
    stickyHeaderCount: PropTypes.number,
    stickyColumnCount: PropTypes.number,
    onScroll: PropTypes.func
  };

  static defaultProps = {
    stickyHeaderCount: 1,
    stickyColumnCount: 1
  };

  constructor(props) {
    super(props);

    this.id = Math.floor(Math.random() * 1000000000) + '';

    this.rowCount = 0;
    this.columnCount = 0;
    this.xScrollSize = 0;
    this.yScrollSize = 0;
    this.stickyHeaderCount = props.stickyHeaderCount === 0 ? 0 : (this.stickyHeaderCount || 1);
    this.container_ref = React.createRef();

    this.isFirefox = navigator.userAgent.toLowerCase().indexOf('firefox') > -1;
  }

  componentDidMount() {
    this.table = document.getElementById('sticky-table-' + this.id);

    if (this.table) {
      this.realTable = this.table.querySelector('#sticky-table-x-wrapper').firstChild;
      this.xScrollbar = this.table.querySelector('#x-scrollbar');
      this.yScrollbar = this.table.querySelector('#y-scrollbar');
      this.xWrapper = this.table.querySelector('#sticky-table-x-wrapper');
      this.yWrapper = this.table.querySelector('#sticky-table-y-wrapper');
      this.stickyHeader = this.table.querySelector('#sticky-table-header');
      this.stickyColumn = this.table.querySelector('#sticky-table-column');
      this.stickyCorner = this.table.querySelector('#sticky-table-corner');
      this.setScrollData();

      elementResizeEvent(this.realTable, this.onResize);

      this.onResize();
      setTimeout(this.onResize);
      this.addScrollBarEventHandlers();
      this.scrollToValue();
    }
  }

  componentDidUpdate() {
    this.scrollToValue();
    this.onResize();
  }

  componentWillUnmount() {
    if (this.table) {
      this.xWrapper.removeEventListener('scroll', this.onScrollX);
      this.xWrapper.removeEventListener('scroll', this.scrollXScrollbar);
      this.xScrollbar.removeEventListener('scroll', this.onScrollBarX);

      this.yWrapper.removeEventListener('scroll', this.scrollYScrollbar);
      this.yScrollbar.removeEventListener('scroll', this.onScrollBarY);

      elementResizeEvent.unbind(this.realTable);
    }
  }

  scrollToValue = () => {
    var element = document.getElementsByClassName("header_element");

    for(var x = 0; x < element.length; x++){
        if(element[x].innerText == this.scrollVal || element[x].innerText == this.props.scrollVal){
            this.yScrollbar.scrollTop = element[x].offsetTop - element[x].offsetHeight - 6;
            this.scrollVal = -1;
            this.props.scrollVal = -1;
            break;
        }
    }

    this.scroll = true;
  }

  addScrollBarEventHandlers() {
    this.xWrapper.addEventListener('scroll', this.onScrollX);

    //X Scrollbars
    this.xWrapper.addEventListener('scroll', this.scrollXScrollbar);
    this.xScrollbar.addEventListener('scroll', this.onScrollBarX);

    //Y Scrollbars
    this.yWrapper.addEventListener('scroll', this.scrollYScrollbar);
    this.yScrollbar.addEventListener('scroll', this.onScrollBarY);
  }

  setScrollData = () => {
    this.suppressScrollX = false;
    this.suppressScrollY = false;

    return this.scrollData = {
      scrollTop: this.yScrollbar.scrollTop,
      scrollHeight: this.yScrollbar.scrollHeight,
      clientHeight: this.yScrollbar.clientHeight,
      scrollLeft: this.xScrollbar.scrollLeft,
      scrollWidth: this.xScrollbar.scrollWidth,
      clientWidth: this.xScrollbar.clientWidth
    };
  }

  onScrollBarX = () => {
    if (!this.suppressScrollX) {
      this.scrollData.scrollLeft = this.xWrapper.scrollLeft = this.xScrollbar.scrollLeft;
      this.suppressScrollX = true;
    } else {
      this.handleScroll();
      this.suppressScrollX = false;
    }
  }

  onScrollBarY = () => {
    if (!this.suppressScrollY) {
      this.scrollData.scrollTop = this.yWrapper.scrollTop = this.yScrollbar.scrollTop;
      this.suppressScrollY = true;
    } else {
      this.handleScroll();
      this.suppressScrollY = false;
    }
  }

  onScrollX = () => {
    var scrollLeft = Math.max(this.xWrapper.scrollLeft, 0);
    this.stickyHeader.style.transform = 'translate(' + (-1 * scrollLeft) + 'px, 0)';
  }

  scrollXScrollbar = () => {
    if (!this.suppressScrollX) {
      this.scrollData.scrollLeft = this.xScrollbar.scrollLeft = this.xWrapper.scrollLeft;
      this.suppressScrollX = true;
    } else {
      this.handleScroll();
      this.suppressScrollX = false;
    }
  }

  scrollYScrollbar = () => {
    if (!this.suppressScrollY) {
      this.scrollData.scrollTop = this.yScrollbar.scrollTop = this.yWrapper.scrollTop;
      this.suppressScrollY = true;
    } else {
      this.handleScroll();
      this.suppressScrollY = false;
    }
  }

  handleScroll = () => {

    if (this.props.onScroll) {
      this.props.onScroll(this.scrollData);
    }
    var scroll_up = this.getScrollBottom();
    var scroll_down = this.getScrollTop();

    if(scroll_up == 1){
      this.updateScrollDown();
    }

    if(scroll_down == 0){
      this.updateScrollUp();
    }
  }

  updateScrollDown = () => {
    if(this.props.set_higher*this.props.step_size >= this.props.size){
      return "Max value reached";
    }else if(this.scroll){
      this.props.set_lower += 1;
      this.props.set_higher+= 1;

      var lower_value = (this.props.set_lower >= 0)?(this.props.set_lower*this.props.step_size):0;
      var upper_value = (this.props.set_higher*this.props.step_size > this.props.size)?this.props.size:(this.props.set_higher*this.props.step_size);

      var element = document.getElementsByClassName("header_element");

      if(element.length > 0){
        this.scrollVal = element[element.length - 1].innerText - 8;
      }

      this.getRows(lower_value, upper_value);
    }
  }

  updateScrollUp = () => {
    if(this.props.set_lower*this.props.step_size <= 0){
      return "Min value reached";
    }else if(this.scroll){
      this.props.set_lower -= 1;
      this.props.set_higher -= 1;

      var lower_value = (this.props.set_lower >= 0)?(this.props.set_lower*this.props.step_size):0;
      var upper_value = (this.props.set_higher*this.props.step_size > this.props.size)?this.props.size:(this.props.set_higher*this.props.step_size);

      var element = document.getElementsByClassName("header_element");

      if(element.length > 0){
        this.scrollVal = element[0].innerText;
      }

      this.getRows(lower_value, upper_value);
    }
  }

  getScrollBottom = () => {
    return (this.yScrollbar.scrollTop + this.yScrollbar.clientHeight)/this.yScrollbar.scrollHeight;
  }

  getScrollTop = () => {
    return (this.yScrollbar.scrollTop)/this.yScrollbar.scrollHeight;
  }

  getRows = (start_index, end_index) => {
    this.scroll = false;
    if(window.navigator.platform == 'MacIntel'){
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'getRows', start: start_index, end: end_index});
    }else{
      window.postMessageToNativeClient('{"method":"get_rows", "start":' + start_index + ', "end": ' + end_index + '}');
    }
  }

  onResize = () => {
    this.setScrollBarDims();
    this.setScrollBarWrapperDims();
    this.setRowHeights();
    this.setColumnWidths();
    this.setScrollData();
    this.handleScroll();
  }

  setScrollBarPaddings() {
    var scrollPadding = '0px 0px ' + this.xScrollSize + 'px 0px';
    this.table.style.padding = scrollPadding;

    var scrollPadding = '0px ' + this.yScrollSize + 'px 0px 0px';
    this.xWrapper.firstChild.style.padding = scrollPadding;
  }

  setScrollBarWrapperDims = () => {
    this.xScrollbar.style.width = 'calc(100% - ' + this.yScrollSize + 'px)';
    this.yScrollbar.style.height = 'calc(100% - ' + this.stickyHeader.offsetHeight + 'px)';
    this.yScrollbar.style.top = this.stickyHeader.offsetHeight + 'px';
  }

  setScrollBarDims() {
    this.xScrollSize = this.xScrollbar.offsetHeight - this.xScrollbar.clientHeight;
    this.yScrollSize = this.yScrollbar.offsetWidth - this.yScrollbar.clientWidth;

    if (!this.isFirefox) {
      this.setScrollBarPaddings();
    }

    var width = this.getSize(this.realTable.firstChild).width;
    this.xScrollbar.firstChild.style.width = width + 'px';

    var height = this.getSize(this.realTable).height + this.xScrollSize - this.stickyHeader.offsetHeight;
    this.yScrollbar.firstChild.style.height = height + 'px';

    if (this.xScrollSize) this.xScrollbar.style.height = this.xScrollSize + 1 + 'px';
    if (this.yScrollSize)  this.yScrollbar.style.width = this.yScrollSize + 1  + 'px';
  }

  getModeHeights(){
    var mode = function mode(arr) {
      var numMapping = {};
      var greatestFreq = 0;
      var mode;
      arr.forEach(function findMode(number) {
          numMapping[number] = (numMapping[number] || 0) + 1;

          if (greatestFreq < numMapping[number]) {
              greatestFreq = numMapping[number];
              mode = number;
          }
      });
      return +mode;
    }

    var heights = []
    if (this.props.stickyColumnCount) {
      for (var r = 0; r < this.rowCount; r++) {
        heights.push(this.getSize(this.realTable.childNodes[r].childNodes[0]).height)
      }
    }

    return mode(heights);
  }

  setRowHeights() {
    var r, c, cellToCopy, height;

    var row_value = (this.props.y != undefined)?this.props.y:-1;
    var column_offset_top = 0;
      
    if (this.props.stickyColumnCount) {
      for (r = 1; r < this.rowCount-1; r++) {
        cellToCopy = this.realTable.childNodes[r].firstChild;
        if (cellToCopy) {
          this.realTable.childNodes[r].childNodes[1].style.height = this.getModeHeights() + "px";
          this.stickyColumn.firstChild.childNodes[r].style.height = this.getModeHeights() + "px";
          height = this.getSize(cellToCopy).height;
          this.stickyColumn.firstChild.childNodes[r].firstChild.style.height = height + 'px';
        
          if (r == 0 && this.stickyCorner.firstChild.childNodes[r]) {
            this.stickyCorner.firstChild.firstChild.firstChild.style.height = height + 'px';
          }
        }
        
        if(row_value == r){
            column_offset_top  = this.stickyColumn.firstChild.childNodes[r].offsetTop;
        }
      }
      this.stickyCorner.firstChild.firstChild.firstChild.style.height = this.stickyHeader.offsetHeight + 'px';
      this.stickyColumn.firstChild.childNodes[0].firstChild.style.height = this.stickyHeader.offsetHeight+'px';
    
      
        if(document.getElementById("data_container")){
            document.getElementById("data_container").style.height =  this.getModeHeights()*3 - 30 + "px"
            document.getElementById("data_container").style.width = (this.xScrollbar.clientWidth - 30) + "px";
            document.getElementById("data_container").style.left = 15 + "px";
            document.getElementById("data_container").style.top = column_offset_top + this.getModeHeights() + 15 + "px";
        }
    }
  }

  setColumnWidths() {
    var c, cellToCopy, cellStyle, width, cell, stickyWidth;

    if (this.stickyHeaderCount) {
      stickyWidth = 0;

      for (c = 0; c < this.columnCount; c++) {
        cellToCopy = this.realTable.firstChild.childNodes[c];

        if (cellToCopy) {
          width = this.getSize(cellToCopy).width;

          cell = this.table.querySelector('#sticky-header-cell-' + c);

          cell.style.width = width + 'px';
          cell.style.minWidth = width + 'px';

          const fixedColumnsHeader = this.stickyCorner.firstChild.firstChild;
          if (fixedColumnsHeader && fixedColumnsHeader.childNodes[c]) {
            cell = fixedColumnsHeader.childNodes[c];
            cell.style.width = width + 'px';
            cell.style.minWidth = width + 'px';

            cell = fixedColumnsHeader.childNodes[c];
            cell.style.width = width + 'px';
            cell.style.minWidth = width + 'px';
            stickyWidth += width;
          }
        }
      }

      this.stickyColumn.firstChild.style.width = stickyWidth + 'px';
      this.stickyColumn.firstChild.style.minWidth = stickyWidth + 'px';
    }
  }

  getStickyColumns(rows) {
    const columnsCount = this.props.stickyColumnCount;
    var cells;
    var stickyRows = [];
    var row_value = (this.props.y != undefined)?parseInt(this.props.y, 10):-1;
      
      
    if(this.props.scrollVal != -1){
      this.scroll = false;
    }

    rows.forEach((row, r) => {
      cells = React.Children.toArray(row.props.children);
      if(row.props.accordion && row_value > 0) {
        if(this.props.data){
          var one_indexed = this.props.data.index  + 1;
          if(one_indexed == parseInt(this.props.y, 10) && this.props.data.column == this.props.column_name && this.props.column_name != undefined){
            var data_entries = [];
            switch (this.props.data.type) {
              case window.flex_type_enum.string:
                data_entries.push(<div style={{"word-wrap": "break-word"}}>{this.props.data.data}</div>);
                break;
              case window.flex_type_enum.dict:
              case window.flex_type_enum.vector:
              case window.flex_type_enum.list:
                data_entries.push(<JSONPretty className="json-pretty" json={this.props.data.data}></JSONPretty>);
                break;
              default:
                break;
            }

            stickyRows.push(
              <Row {...row.props}>
                  <div id="data_container" style={{"position": "absolute", "overflow": "scroll", "width": 100, "left": 3, "zIndex": 99, "textAlign": "left", "background": "#F7F7F7"}}>
                  <div style={{"padding": 10}}>
                    {data_entries}
                  </div>
                </div>
              </Row>
            );
          }else{
            stickyRows.push(
              <Row {...row.props}>
                <div id="data_container" style={{"position": "absolute", "overflow": "scroll", "width": 100, "left": 3, "zIndex": 99, "textAlign": "left", "background": "#F7F7F7"}}>
                  <div style={{"padding": 10}}>
                    Loading...
                  </div>
                </div>
              </Row>
            );
          }
        }else{
          stickyRows.push(
            <Row {...row.props}>
              <div id="data_container" style={{"position": "absolute", "overflow": "scroll", "width": 100, "left": 3, "zIndex": 99, "textAlign": "left", "background": "#F7F7F7"}}>
                <div style={{"padding": 10}}>
                  Loading...
                </div>
              </div>
            </Row>
          );
        }
      } else if(row.props.spacers && row_value > 0) {
        stickyRows.push(
          <Row {...row.props} key={r}>
            {cells.slice(0, columnsCount)}
          </Row>
        );
      }else{
        stickyRows.push(
          <Row {...row.props} id='' key={r}>
            {cells.slice(0, columnsCount)}
          </Row>
        );
      }
    });

    return stickyRows;
  }

  getStickyHeader(rows) {
    var row = rows[0];
    var cells = [];

    React.Children.toArray(row.props.children).forEach((cell, c) => {
      cells.push(React.cloneElement(cell, {id: 'sticky-header-cell-' + c, key: c}));
    });

    return (
      <Row {...row.props} id='sticky-header-row'>
        {cells}
      </Row>
    );
  }

  getStickyCorner(rows) {
    const columnsCount = this.props.stickyColumnCount;
    var cells;
    var stickyCorner = [];

    rows.forEach((row, r) => {
      if (r === 0) {
        cells = React.Children.toArray(row.props.children);

        stickyCorner.push(
          <Row {...row.props} id='' key={r}>
            {cells.slice(0, columnsCount)}
          </Row>
        );
      }
    });
    return stickyCorner;
  }

  getSize(node) {
    var width = node ? node.getBoundingClientRect().width : 0;
    var height = node ? node.getBoundingClientRect().height : 0;

    return {width, height};
  }

  render() {
    var rows = React.Children.toArray(this.props.children);
    var stickyColumn, stickyHeader, stickyCorner;

    this.rowCount = rows.length;
    this.columnCount = (rows[0] && React.Children.toArray(rows[0].props.children).length) || 0;
      
    if (rows.length) {
      if (this.props.stickyColumnCount > 0 && this.stickyHeaderCount > 0) {
        stickyCorner = this.getStickyCorner(rows);
      }
      if (this.props.stickyColumnCount > 0) {
        stickyColumn = this.getStickyColumns(rows);
      }
      if (this.stickyHeaderCount > 0) {
        stickyHeader = this.getStickyHeader(rows);
      }
    }
      
    return (
      <div className={'sticky-table ' + (this.props.className || '')} id={'sticky-table-' + this.id}>
        <div id='x-scrollbar'><div></div></div>
        <div id='y-scrollbar'><div></div></div>
        <div className='sticky-table-corner' id='sticky-table-corner'>
          <Table>{stickyCorner}</Table>
        </div>
        <div className='sticky-table-header' id='sticky-table-header'>
          <Table>{stickyHeader}</Table>
        </div>
        <div className='sticky-table-y-wrapper' id='sticky-table-y-wrapper'>
          <div className='sticky-table-column' id='sticky-table-column'>
            <Table>{stickyColumn}</Table>
          </div>
          <div className='sticky-table-x-wrapper' id='sticky-table-x-wrapper'>
            <Table>{rows}</Table>
          </div>
        </div>
      </div>
    );
  }
}

export {StickyTable, Table, Row, Cell};
