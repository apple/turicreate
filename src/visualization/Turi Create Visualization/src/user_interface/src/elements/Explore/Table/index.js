import React, { Component } from 'react';
import { StickyTable, Row, Cell } from './sticky-table'
import './index.css';

import FontAwesomeIcon from '@fortawesome/react-fontawesome'
import faAngleUp from '@fortawesome/fontawesome-free-solid/faAngleUp'
import faAngleDown from '@fortawesome/fontawesome-free-solid/faAngleDown'

var d3 = require("d3");

var numberWithCommas = d3.format(",");

class TcTable extends Component {

  renderCell(value, type){
    switch (type) {
      case "string":
        return (
          <div style={{"position":"relative"}}>
            <div className="string_val" onClick={(e) => this.divClick(e)}>
              <span className="default_span_color" onClick={(e) => this.spanClick(e)}>
                { value }
              </span>
            </div>
            <div className="arrow_right" onClick={(e) => this.arrowClick(e)}>
              <FontAwesomeIcon icon={faAngleDown} style={{"display": "block"}} onClick={(e) => this.spanClick(e)} />
              <FontAwesomeIcon icon={faAngleUp} style={{"display": "none"}} onClick={(e) => this.spanClick(e)} />
            </div>
          </div>
        );
      case "float":
        return (
          <div className="float_val">
            { value }
          </div>
        );
      case "integer":
        return (
          <div className="int_val">
            { value }
          </div>
        );
      case "image":
        return (
          <div className="image_val">
                <img src={ "data:image/" + value.format + ";base64," + value.data }  height={32} data-image-row={value.idx} data-image-column={value.column} alt={value.width + "x" + value.height + " image"} />
          </div>
        );

      case "dictionary":
        return (
          <div style={{"position":"relative"}}>
            <div className="dictionary_value" onClick={(e) => this.divClick(e)}>
              <span className="default_span_color" onClick={(e) => this.spanClick(e)}>
                { value }
              </span>
            </div>
            <div className="arrow_right" onClick={(e) => this.arrowClick(e)}>
              <FontAwesomeIcon icon={faAngleDown} style={{"display": "block"}} onClick={(e) => this.spanClick(e)} />
              <FontAwesomeIcon icon={faAngleUp} style={{"display": "none"}} onClick={(e) => this.spanClick(e)} />
            </div>
          </div>
        );
      case "array":
          return (
            <div style={{"position":"relative"}}>
              <div className="array_value" onClick={(e) => this.divClick(e)}>
                <span className="default_span_color" onClick={(e) => this.spanClick(e)}>
                  { value }
                </span>
              </div>
              <div className="arrow_right" onClick={(e) => this.arrowClick(e)}>
                <FontAwesomeIcon icon={faAngleDown} style={{"display": "block"}} onClick={(e) => this.spanClick(e)} />
                <FontAwesomeIcon icon={faAngleUp} style={{"display": "none"}} onClick={(e) => this.spanClick(e)} />
              </div>
            </div>
              );
      case "list":
        return (
          <div style={{"position":"relative"}}>
            <div className="array_value" onClick={(e) => this.divClick(e)}>
              <span className="default_span_color" onClick={(e) => this.spanClick(e)}>
                { JSON.stringify(value) }
              </span>
            </div>
            <div className="arrow_right" onClick={(e) => this.arrowClick(e)}>
              <FontAwesomeIcon icon={faAngleDown} style={{"display": "block"}} onClick={(e) => this.spanClick(e)} />
              <FontAwesomeIcon icon={faAngleUp} style={{"display": "none"}} onClick={(e) => this.spanClick(e)} />
            </div>
          </div>
                );
      default:
        return (
          <div>
            { value }
          </div>
        );
    }
  }

  constructor(props){
    super(props);
    this.image_array = [];
    this.image_dictionary = {};
    this.data = undefined;
    this.step_size = (this.props.step_size)?this.props.step_size:50;
    this.test_this = "element";
    this.scrollVal = -1;
    this.state = {
      windowHeight: window.innerHeight,
      windowWidth: window.innerWidth,
      table: []
    };
    this.loading_image_timer = undefined;
    this.y = -1;
    this.column_name = undefined;
    this.data_sent = undefined;
  }

  componentDidMount(){
    this.table_spec = this.props.table_spec;
    this.size = this.props.table_spec["size"];
    this.title = this.props.table_spec["title"];

    this.image_source_container = document.getElementById("image_source_container");
    this.arrow_left = document.getElementById("arrow_left");
    this.image_loading_container = document.getElementById("image_loading_container");
    this.rowNum = document.getElementById("rowNum");

    window.addEventListener('resize', this.handleResize.bind(this));

    if(window.navigator.platform === 'MacIntel'){
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'ready'});
    }

    var string_elements = document.getElementsByClassName("string_val");
    for(var x = 0; x < string_elements.length; x++){
      if(string_elements[x].getBoundingClientRect().width < string_elements[x].children[0].getBoundingClientRect().width){
        string_elements[x].parentElement.parentElement.classList.remove("hoverable_element");
        string_elements[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }

    var dictionary_value = document.getElementsByClassName("dictionary_value");
    for(var x = 0; x < dictionary_value.length; x++){
      if(dictionary_value[x].getBoundingClientRect().width < dictionary_value[x].children[0].getBoundingClientRect().width){
        dictionary_value[x].parentElement.parentElement.classList.remove("hoverable_element");
        dictionary_value[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }

    var array_value = document.getElementsByClassName("array_value");
    for(var x = 0; x < array_value.length; x++){
      if(array_value[x].getBoundingClientRect().width < array_value[x].children[0].getBoundingClientRect().width){
        array_value[x].parentElement.parentElement.classList.remove("hoverable_element");
        array_value[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }
  }

  componentDidUpdate(prevProps, prevState, snapshot){

    this.image_source_container = document.getElementById("image_source_container");
    this.arrow_left = document.getElementById("arrow_left");
    this.image_loading_container = document.getElementById("image_loading_container");
    this.rowNum = document.getElementById("rowNum");

    var string_elements = document.getElementsByClassName("string_val");
    for(var x = 0; x < string_elements.length; x++){
      if(string_elements[x].getBoundingClientRect().width < string_elements[x].children[0].getBoundingClientRect().width){
        string_elements[x].parentElement.parentElement.classList.remove("hoverable_element");
        string_elements[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }

    var dictionary_value = document.getElementsByClassName("dictionary_value");
    for(var x = 0; x < dictionary_value.length; x++){
      if(dictionary_value[x].getBoundingClientRect().width < dictionary_value[x].children[0].getBoundingClientRect().width){
        dictionary_value[x].parentElement.parentElement.classList.remove("hoverable_element");
        dictionary_value[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }

    var array_value = document.getElementsByClassName("array_value");
    for(var x = 0; x < array_value.length; x++){
      if(array_value[x].getBoundingClientRect().width < array_value[x].children[0].getBoundingClientRect().width){
        array_value[x].parentElement.parentElement.classList.remove("hoverable_element");
        array_value[x].parentElement.parentElement.classList.add("hoverable_element");
      }
    }
  }

  componentWillMount(){
    window.removeEventListener('resize', this.handleResize.bind(this));
  }

  enterPressJumpRow(e) {
    if(e.keyCode == 13){
      if(this.rowNum != null){
        var row_number = this.rowNum.value;
        this.rowNum.value = "";
        if(!isNaN(parseInt(row_number, 10))){
          this.jump_to(row_number);
        }
      }
    };
  }

  rowHandler(e){
    if(this.rowNum != null){
      var row_number = this.rowNum.value;
      this.rowNum.value = "";
      if(!isNaN(parseInt(row_number, 10))){
        this.jump_to(row_number);
      }
    }
  }

  jump_to(value){
    var lower_value = 0;
    var upper_value = 0;
    var lower_bound;
    var upper_bound;

    if(value > this.size || value < 0){
        return "err: out of bounds";
    }

    if(value%this.step_size  != 0){
      lower_bound = Math.floor(value/this.step_size);
      upper_bound = Math.ceil(value/this.step_size);
    }else{
      lower_bound = Math.floor(value/this.step_size);
      upper_bound = lower_bound + 1;
    }

    this.set_lower = (lower_bound-1 >= 0)?(lower_bound-1):0;
    this.set_higher = upper_bound;

    lower_value = (lower_bound-1 >= 0)?((lower_bound-1)*this.step_size):0;
    upper_value = (upper_bound*this.step_size > this.size)?this.size:(upper_bound*this.step_size);

    this.getRows(lower_value, upper_value);

    this.scrollVal = value;
  }

  getRows(start_index, end_index) {
    if(window.navigator.platform == 'MacIntel'){
      window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'getRows', start: start_index, end: end_index});
    }else{
      window.postMessageToNativeClient('{"method":"get_rows", "start":' + start_index + ', "end": ' + end_index + '}');
    }
  }

  setAccordionData(data){
    this.data_sent = data["data"];
    this.drawTable();
  }

  handleResize(e) {
    var $this = this;
    this.setState((state) => ({
      windowHeight: window.innerHeight,
      windowWidth: window.innerWidth
    }), function(){
      $this.drawTable();
    });
  }

  hoverImage(result){
    var $this = this;
    return function(e) {
      if(result == "image"){
        $this.image_source_container.style.display = "block";
        $this.arrow_left.style.display = "block";
        if($this.image_source_container != null && e.target.getElementsByTagName("img")[0] && e.target.classList.contains('elements')){
          var element_bounding = e.target.getBoundingClientRect();
          var arrow_bounding = $this.arrow_left.getBoundingClientRect();
          var bound_height = parseInt(((element_bounding.height/2 - arrow_bounding.height/2) + (element_bounding.y)), 10);
          var imageRow = e.target.getElementsByTagName("img")[0].getAttribute("data-image-row");
          var imageColumn = e.target.getElementsByTagName("img")[0].getAttribute("data-image-column");

          if($this.image_dictionary[String(imageRow)]){
            if($this.image_dictionary[String(imageRow)][String(imageColumn)]){
              var src_values = "data:image/" + $this.image_dictionary[String(imageRow)][String(imageColumn)]["format"] + ";base64," + $this.image_dictionary[String(imageRow)][String(imageColumn)]["image"];
              $this.image_source_container.src = src_values;
              $this.arrow_left.style.top = bound_height + "px";

              var image_bounding = $this.image_source_container.getBoundingClientRect();
              var image_bound_height = parseInt(((element_bounding.height/2 - image_bounding.height/2) + (element_bounding.y)), 10);

              var header_positon = document.getElementsByClassName("header")[0].getBoundingClientRect();
              var header_top = header_positon.y + header_positon.height;

              var footer_position = document.getElementsByClassName("BottomTable")[0].getBoundingClientRect();
              var footer_top = footer_position.y;

              if(header_top > image_bound_height){
                image_bound_height = header_top;
              }

              if((image_bound_height+image_bounding.height) > footer_top){
                image_bound_height = footer_top - image_bounding.height;
              }

              $this.image_source_container.style.top = image_bound_height + "px";

              var right_value = (element_bounding.width + element_bounding.left);
              var right_image = (element_bounding.width + element_bounding.left + 10);

              $this.arrow_left.style.left = right_value + "px";
              $this.image_source_container.style.left = right_image +"px";
            }
          }else{
              $this.image_source_container.style.display = "none";
              $this.image_loading_container.style.display = "block";

              $this.arrow_left.style.top = bound_height + "px";

              var image_bounding = $this.image_loading_container.getBoundingClientRect();
              var image_bound_height = parseInt(((element_bounding.height/2 - image_bounding.height/2) + (element_bounding.y)), 10);

              var header_positon = document.getElementsByClassName("header")[0].getBoundingClientRect();
              var header_top = header_positon.y + header_positon.height;

              var footer_position = document.getElementsByClassName("BottomTable")[0].getBoundingClientRect();
              var footer_top = footer_position.y;

              if(header_top > image_bound_height){
                image_bound_height = header_top;
              }

              if((image_bound_height+image_bounding.height) > footer_top){
                image_bound_height = footer_top - image_bounding.height;
              }

              $this.image_loading_container.style.top = image_bound_height + "px";

              var right_value = (element_bounding.width + element_bounding.left);
              var right_image = (element_bounding.width + element_bounding.left + 10);

              $this.arrow_left.style.left = right_value + "px";
              $this.image_loading_container.style.left = right_image +"px";

              var target = e.target;
              $this.loading_image_timer = setInterval(function(imageRow, imageColumn, target){
                                                             if($this.image_dictionary[String(imageRow)]){
                                                                 if($this.image_dictionary[String(imageRow)][String(imageColumn)]){

                                                                    $this.image_source_container.style.display = "block";
                                                                    $this.image_loading_container.style.display = "none";

                                                                    var element_bounding = target.getBoundingClientRect();


                                                                    var arrow_bounding = $this.arrow_left.getBoundingClientRect();
                                                                    var bound_height = parseInt(((element_bounding.height/2 - arrow_bounding.height/2) + (element_bounding.y)), 10);

                                                                    var imageRow = target.getElementsByTagName("img")[0].getAttribute("data-image-row");
                                                                    var imageColumn = target.getElementsByTagName("img")[0].getAttribute("data-image-column");

                                                                    var src_values = "data:image/" + $this.image_dictionary[String(imageRow)][String(imageColumn)]["format"] + ";base64," + $this.image_dictionary[String(imageRow)][String(imageColumn)]["image"];

                                                                    $this.image_source_container.src = src_values;
                                                                    $this.arrow_left.style.top = bound_height + "px";

                                                                    var image_bounding = $this.image_source_container.getBoundingClientRect();
                                                                    var image_bound_height = parseInt(((element_bounding.height/2 - image_bounding.height/2) + (element_bounding.y)), 10);

                                                                    var header_positon = document.getElementsByClassName("header")[0].getBoundingClientRect();
                                                                    var header_top = header_positon.y + header_positon.height;

                                                                    var footer_position = document.getElementsByClassName("BottomTable")[0].getBoundingClientRect();
                                                                    var footer_top = footer_position.y;

                                                                    if(header_top > image_bound_height){
                                                                        image_bound_height = header_top;
                                                                    }

                                                                    if((image_bound_height+image_bounding.height) > footer_top){
                                                                        image_bound_height = footer_top - image_bounding.height;
                                                                    }

                                                                    $this.image_source_container.style.top = image_bound_height + "px";

                                                                    var right_value = (element_bounding.width + element_bounding.left);
                                                                    var right_image = (element_bounding.width + element_bounding.left + 10);

                                                                    $this.arrow_left.style.left = right_value + "px";
                                                                    $this.image_source_container.style.left = right_image +"px";

                                                                    clearInterval($this.loading_image_timer);

                                                                 }
                                                             }
                                                         }, 300, imageRow, imageColumn, target);
          }
        }
      }
    }
  }

  hoverOutImage(result){
    var $this = this;
    return function(e) {
      if(result == "image"){
        if($this.image_source_container != null && e.target.getElementsByTagName("img")[0] && e.target.classList.contains('elements')){
          $this.image_source_container.src = "";
        }
      }
      if($this.loading_image_timer){
        clearInterval($this.loading_image_timer);
      }

      $this.image_source_container.style.display = "none";
      $this.image_loading_container.style.display = "none";
      $this.arrow_left.style.display = "none";
    }
  }

  divClick(e){
    var dict_elements = {};
    dict_elements["target"] = (e.target.parentElement.parentElement);
    this.cellClick(dict_elements);
  }

  spanClick(e){
    var dict_elements = {};
    dict_elements["target"] = (e.target.parentElement.parentElement.parentElement);
    this.cellClick(dict_elements);
  }

  arrowClick(e){
    var dict_elements = {};
    dict_elements["target"] = (e.target.parentElement.parentElement);
    this.cellClick(dict_elements);
  }

  cellClick(e){
    var row_number = e.target.getAttribute("x");
    var column_name = e.target.getAttribute("y");
    var type = e.target.getAttribute("flex_type");

    switch (type) {
      case "string":
      case "dictionary":
      case "array":
      case "list":
        var active_element_loop = e.target.parentElement.parentElement.getElementsByClassName("active_element");

        for(var ele = 0; ele < active_element_loop.length; ele++){
          if(active_element_loop[ele] != e.target){
            active_element_loop[ele].children[0].children[1].children[0].style.display = "block";
            active_element_loop[ele].children[0].children[1].children[1].style.display = "none";
            active_element_loop[ele].children[0].children[1].classList.remove("active_arrow");
            active_element_loop[ele].classList.remove("active_element");
          }
        }


        if(e.target.children[0].children[0].getBoundingClientRect().width < e.target.children[0].children[0].children[0].getBoundingClientRect().width){

          if(e.target.children[0].children[1].children[0].style.display == "none"){
            e.target.children[0].children[1].children[0].style.display = "block";
            e.target.children[0].children[1].children[1].style.display = "none";
            e.target.classList.remove("active_element");
            e.target.children[0].children[1].classList.remove("active_arrow");
            this.data_sent = undefined;
            this.column_name = undefined;
            this.y = -1;
            this.drawTable();
          }else{
            e.target.children[0].children[1].children[0].style.display = "none";
            e.target.children[0].children[1].children[1].style.display = "block";
            e.target.classList.add("active_element");
            e.target.children[0].children[1].classList.add("active_arrow");
            this.data_sent = undefined;
            this.column_name = undefined;
            this.y = -1;

            var column_name_str = column_name.replace('"', "\\\"");

            this.drawTable(true, row_number, column_name, column_name_str);
          }
        }

        break;
      case "image":
        break;
      default:
        break;
    }
  }

  drawTable(callback_accordion = false, y = -1, column_name = undefined, column_name_str = undefined){

    var rows = [];
    var row_ids = [];

    this.table_array = [];

    for (var r = 0; r < this.data.values.length+1; r++) {
      var cells = [];
      for (var c = 0; c < this.table_spec["column_names"].length+1; c++) {
        if(c === 0){
          if(r === 0){
            cells.push(<Cell className="header" key={c+"_"+r}></Cell>);
          }else{
            cells.push(<Cell className="header_element" key={c+"_"+r}>{this.data.values[r-1]["__idx"]}</Cell>);
            row_ids.push(this.data.values[r-1]["__idx"]);
          }
        }else{
          if(r === 0){
            cells.push(<Cell className={"header"} key={c+"_"+r}><span>{this.table_spec["column_names"][c-1]}</span></Cell>);
          }else{
            var element_type = this.table_spec["column_types"][c-1];
            var element_column_name = this.table_spec["column_names"][c-1];
            cells.push(<Cell className="elements" onMouseEnter={this.hoverImage(this.table_spec["column_types"][c-1])} onMouseLeave={this.hoverOutImage(this.table_spec["column_types"][c-1])} onClick={(e) => this.cellClick(e)} x={r} x_c={c} y={element_column_name} flex_type={element_type} key={c+"_"+r}>{ this.renderCell(this.data.values[r-1][element_column_name], element_type )}</Cell>);
          }
        }
      }
      rows.push(<Row key={r}>{cells}</Row>);

      if(this.y == r){
        var empty_cells = [];
        empty_cells.push(<Cell className={"header_element accordion_helper"} key={"0_"+r+"modal"}>&nbsp;</Cell>);

        for(var x = 1; x < cells.length;x++){
          empty_cells.push(<Cell className={"elements accordion_helper"} key={x+"_"+r+"modal"}>&nbsp;</Cell>);
        }

        rows.push(<Row key={"modal"} accordion={true}>
                    {empty_cells}
                  </Row>);

        var empty_cells_1 = [];
        empty_cells_1.push(<Cell className={"header_element accordion_helper"} key={"0_"+r+"spacer1"}>&nbsp;</Cell>);

        for(var x = 1; x < cells.length;x++){
          empty_cells_1.push(<Cell className={"elements accordion_helper"} key={x+"_"+r+"spacer1"}>&nbsp;</Cell>);
        }

        rows.push(<Row key={"spacer1"} spacers={true}>
                    {empty_cells_1}
                  </Row>);

        var empty_cells_2 = [];
        empty_cells_2.push(<Cell className={"header_element accordion_helper"} key={"0_"+r+"spacer2"}>&nbsp;</Cell>);

        for(var x = 1; x < cells.length;x++){
          empty_cells_2.push(<Cell className={"elements accordion_helper"} key={x+"_"+r+"spacer2"}>&nbsp;</Cell>);
        }

        rows.push(<Row key={"spacer2"} spacers={true}>
                    {empty_cells_2}
                  </Row>);
      }
    }


    this.set_higher = ((Math.max(...row_ids)+1)/this.step_size);
    this.set_lower = (Math.min(...row_ids)/this.step_size);

    if(this.title != ""){
      var tableTitle = (
                          <h1 className="tableTitle"  key="tableTitle">
                            {this.title}
                          </h1>
                        );
      this.table_array.push(tableTitle);
    }

    var tableBody = (
                      <div className="resize_container" key="tableBody" style={{ "height": this.state.windowHeight-44, "width": this.state.windowWidth}}>
                        <StickyTable scrollVal={this.scrollVal} size={this.size} step_size={this.step_size} set_lower={this.set_lower} set_higher={this.set_higher}  y={this.y} data={this.data_sent} column_name={this.column_name} style={{ "height": this.state.windowHeight-44, "width": this.state.windowWidth}}>
                          {rows}
                        </StickyTable>
                      </div>
                    );

    var tableFooter = (
                        <div className="BottomTable" key="tableFooter">
                          &nbsp;
                          <div className="numberOfRows">
                            {numberWithCommas(this.size)} rows
                          </div>
                          <div className="jumpToRowContainer">
                            <input className="rowNumber" id="rowNum" onKeyDown={this.enterPressJumpRow.bind(this)} placeholder="Row #"/>
                            <img className="enterSymbol" onClick={this.rowHandler.bind(this)} src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEYAAAAxCAYAAABnCd/9AAAAAXNSR0IArs4c6QAAA9RJREFUaAXtmk1oE0EUx80mahCiglKKpSAiHrTUSz1YBL9REEE8BBSqLQrRNm1BvPSk9ODHQSHpRywKwUMvxVIQrccqanvw5qEHD4LooQgeaqGY5svfK01Yt7t1d7Mpm00Wys6+nZl9/1/fm5mdrG9DlR0DAwNn8vn8HWRP9/T03DaS7zO64UV7LBa7jq4EfwHR5/f7d0ej0W9S1h6K1uDVa6D0o+0pf8tQRCdgQnLWO4qV9G56wTYyMrIxlUo9I32uWNHjaTDxeHwrUMaBcsoKFKnr2VRKJBINAHlvB4pnwQClKZ1OzyCwWUTaOTwXMUzHJ4DygUhptAOk0MZTY8zQ0NDebDY7CZTNBYF2z56KGIC0OAFFYHoNjGN6HOvIbsi6tZ1jYwxrhqOIjBDKiz6f7zHvIbNuFW3GL0cihuX2LR42BZRLnK9x/shAeMiMA26tUxIYAChESgxxjyirX0i3MzvccKtoM37ZTqVkMhlkzTAKkIt6D8LeomevFJstMIODgzsWFhZeIr7VSCjjjK2+jfpbb7vlVBoeHt6Ty+Wm14Ky3iLK8TxLYGRAzWQyM0DZVw5n3NSnaTAMsucZUN8Cpc5NAsrliykwDLIyw0wAZUu5HHFbv2uCAYSPNcp9xpQEZb/bnC+nP4Yzx9jY2CbSJ8nDL5fTAbf2rQuGfdJtc3NzEzh93K2Ol9uvVWCYeRrZJ31D6hwo5eG0r2NsuldKH1bb8sz9VtsY1f8HDKlzcGWjZ5dRAwv2nYxNfRbqu6pqcfBlNXsa4rJ57AQUV4m048wyGKBc5b/7mg4Mf4Cy03kltwmQPu2kj8w+tUNFQCF14qrrWnGFgKRSLX10wkFheyClY696k0SMfCtSOzQEFDatHxI1NThaMHINnH6+FWkHUFpzv2oviws8vix6rijKOeD8rgYaMrYGg8EfRlrVO/vLddi6bOZH8UkuGowambHz4HnqvTJTd73rsETJkCEvCAZD/1aBESflRZItTIHTZNdpwMySoiW9iNp9thPtiqmk7qyrq+s7YXYEcVNqezWVdcEIgEgkMl9fX38WOKPVBKSg1RCMVAiHw0vd3d1tFB8UGlTLeU0wAoGIyff29vYxY92knK2B0RAgcp4A5wJwFjW3PHn534hRq5bpjWnuGHB+qu1eLFsCIwCYsT4FAoHDwPniRSAFTZbBSMPOzs6vpFUrcKYLHWnPLKJyWlslXdsCIwJJq1+hUOgkxXEDwZ8N7BVhtg1G1HV0dPxhdRumKB8PqY8loqmit0t1XwnUCs2W+Sm3DRhR6susdRdg78y2dWO9vzv+PVLOdXkdAAAAAElFTkSuQmCC" height={8}/>
                          </div>
                        </div>
                      );

      var imageContainer = (
                            <div id="image_container" key="image_container">
                              <div id="arrow_left">
                                <div id="diamond">
                                </div>
                                <div id="diamond_top">
                                </div>
                              </div>
                              <img id="image_source_container" src="" />
                              <div id="image_loading_container">
                                <div className="table_loading_parent">
                                    <div className="table_loader_container">
                                        <div className="table_loader"></div>
                                    </div>
                                </div>
                              </div>
                            </div>
                            );

      this.table_array.push(tableBody);
      this.table_array.push(tableFooter);
      this.table_array.push(imageContainer);

      var temp_table = this.table_array[0];

      for(var x = 1; x < this.table_array.length; x++){
        temp_table = [temp_table, this.table_array[x]];
      }

      var $this = this;

      this.setState({table: temp_table}, function() {
        if(callback_accordion){
            $this.y = y;
            $this.column_name = column_name;
            $this.drawTable()

            var element_index = String(parseInt(y, 10) - 1);

            if(window.navigator.platform == 'MacIntel'){
                window.webkit.messageHandlers["scriptHandler"].postMessage({status: 'getAccordion', column: column_name_str, index: element_index});
            }else{
                window.postMessageToNativeClient('{"method":"get_accordion", "column": "' + column_name_str + '", "index": ' + element_index + '}');
            }
        }
      });
  }

  updateData(data){
    this.data = data;

    this.data_sent = undefined;
    this.column_name = undefined;
    this.y = -1;

    var active_element_loop = document.getElementsByClassName("active_element");

    for(var ele = 0; ele < active_element_loop.length; ele++){
      active_element_loop[ele].children[0].children[1].children[0].style.display = "block";
      active_element_loop[ele].children[0].children[1].children[1].style.display = "none";
      active_element_loop[ele].children[0].children[1].classList.remove("active_arrow");
      active_element_loop[ele].classList.remove("active_element");
    }

    this.drawTable()
  }

  cleanImageDictionary(){
    for (var key in this.image_dictionary){
      if(parseInt(key, 10) < this.set_lower*this.step_size || parseInt(key, 10) > this.set_higher*this.step_size){
        delete this.image_dictionary[key];
      }
    }
  }

  setImageData(value){
    this.cleanImageDictionary();
    if(value.data){
      if(value.data.data){
        for(var x = 0; x < value.data.data.length; x++){
          if (!this.image_dictionary[String(value.data.data[x]["idx"])]) {
            this.image_dictionary[String(value.data.data[x]["idx"])] = {};
          }
          if (!this.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]) {
            this.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]] = {};
          }
          this.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]["image"] = value.data.data[x]["image"];
          this.image_dictionary[String(value.data.data[x]["idx"])][value.data.data[x]["column"]]["format"] = value.data.data[x]["format"];
        }
      }
    }
  }

  render() {
    return (
      <div>
        {this.state.table}
      </div>
    );
  }
}

export default TcTable;
