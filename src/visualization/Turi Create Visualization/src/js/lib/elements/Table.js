import { StickyTable, Row, Cell } from 'react-sticky-table';
import React, { Component } from 'react';
var numberWithCommas = d3.format(",");

class Tctable extends Component {
      renderCell(value, type){
        switch (type) {
          case "string":
            return (
              <div className="string_val">
                <span>
                  { value }
                </span>
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
                    <img src={ "data:image/" + value.format + ";base64," + value.data } height={32} data-image-row={value.idx} data-image-column={value.column} alt={value.width + "x" + value.height + " image"} />
              </div>
            );
          case "dictionary":
            return (
              <div className="dictionary_value">
                <span>
                  { value }
                </span>
              </div>
            );
          case "array":
              return (
                <div className="array_value">
                  <span>
                    { value }
                  </span>
                </div>
              );
          case "list":
            return (
                <div className="array_value">
                  <span>
                    { JSON.stringify(value) }
                  </span>
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

      enterPressJumpRow(param) {
        return function(e) {
          if(e.keyCode == 13){
            param(e);
          }
        };
      }


    hoverImage(result){
      return function(e) {
        if(result == "image"){
          document.getElementById("image_source_container").style.display = "block";
          document.getElementById("arrow_left").style.display = "block";
          if(document.getElementById("image_source_container") != null && e.target.getElementsByTagName("img")[0] && e.target.classList.contains('elements')){
            var element_bounding = e.target.getBoundingClientRect();
            var arrow_bounding = document.getElementById("arrow_left").getBoundingClientRect();
            var bound_height = parseInt(((element_bounding.height/2 - arrow_bounding.height/2) + (element_bounding.y)), 10);

            var imageRow = e.target.getElementsByTagName("img")[0].getAttribute("data-image-row");
            var imageColumn = e.target.getElementsByTagName("img")[0].getAttribute("data-image-column");

            if(window.image_dictionary[String(imageRow)]){
                if(window.image_dictionary[String(imageRow)][String(imageColumn)]){
                    var src_values = "data:image/" + window.image_dictionary[String(imageRow)][String(imageColumn)]["format"] + ";base64," + window.image_dictionary[String(imageRow)][String(imageColumn)]["image"];

                    document.getElementById("image_source_container").src = src_values;
                    document.getElementById("arrow_left").style.top = bound_height + "px";

                    var image_bounding = document.getElementById("image_source_container").getBoundingClientRect();
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

                    document.getElementById("image_source_container").style.top = image_bound_height + "px";

                    var right_value = (element_bounding.width + element_bounding.left);
                    var right_image = (element_bounding.width + element_bounding.left + 10);

                    document.getElementById("arrow_left").style.left = right_value + "px";
                    document.getElementById("image_source_container").style.left = right_image +"px";
                }
            }else{
                document.getElementById("image_source_container").style.display = "none";
                document.getElementById("image_loading_container").style.display = "block";

                document.getElementById("arrow_left").style.top = bound_height + "px";

                var image_bounding = document.getElementById("image_loading_container").getBoundingClientRect();
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

                document.getElementById("image_loading_container").style.top = image_bound_height + "px";

                var right_value = (element_bounding.width + element_bounding.left);
                var right_image = (element_bounding.width + element_bounding.left + 10);

                document.getElementById("arrow_left").style.left = right_value + "px";
                document.getElementById("image_loading_container").style.left = right_image +"px";
                
                var target = e.target;
                window.loading_image_timer = setInterval(function(imageRow, imageColumn, target){
                                                             if(window.image_dictionary[String(imageRow)]){
                                                                 if(window.image_dictionary[String(imageRow)][String(imageColumn)]){
                                                         
                                                                   document.getElementById("image_source_container").style.display = "block";
                                                                    document.getElementById("image_loading_container").style.display = "none";
                                                         
                                                                    var element_bounding = target.getBoundingClientRect();
                                                         
                                                         
                                                                    var arrow_bounding = document.getElementById("arrow_left").getBoundingClientRect();
                                                                    var bound_height = parseInt(((element_bounding.height/2 - arrow_bounding.height/2) + (element_bounding.y)), 10);
                                                         
                                                                    var imageRow = target.getElementsByTagName("img")[0].getAttribute("data-image-row");
                                                                    var imageColumn = target.getElementsByTagName("img")[0].getAttribute("data-image-column");
                                                         
                                                                    var src_values = "data:image/" + window.image_dictionary[String(imageRow)][String(imageColumn)]["format"] + ";base64," + window.image_dictionary[String(imageRow)][String(imageColumn)]["image"];

                                                                    document.getElementById("image_source_container").src = src_values;
                                                                    document.getElementById("arrow_left").style.top = bound_height + "px";

                                                                    var image_bounding = document.getElementById("image_source_container").getBoundingClientRect();
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

                                                                    document.getElementById("image_source_container").style.top = image_bound_height + "px";

                                                                    var right_value = (element_bounding.width + element_bounding.left);
                                                                    var right_image = (element_bounding.width + element_bounding.left + 10);

                                                                    document.getElementById("arrow_left").style.left = right_value + "px";
                                                                    document.getElementById("image_source_container").style.left = right_image +"px";
                                                         
                                                                    clearInterval(window.loading_image_timer);
 
                                                                 }
                                                             }
                                                         }, 300, imageRow, imageColumn, target);
            }
          }
        }
      }
    }

    hoverOutImage(result){
      return function(e) {
        if(result == "image"){
          if(document.getElementById("image_source_container") != null && e.target.getElementsByTagName("img")[0] && e.target.classList.contains('elements')){
            document.getElementById("image_source_container").src = ""
          }
        }

        if(window.loading_image_timer){
          clearInterval(window.loading_image_timer);
        }

        document.getElementById("image_loading_container").style.display = "none";
        document.getElementById("image_source_container").style.display = "none";
        document.getElementById("arrow_left").style.display = "none";
      }
    }

  render() {
    var rows = [];
    var cells;

    var spec = this.props.spec;
    var data = this.props.data;

    var num_rows = this.props.size;
    var rowHandler = this.props.enterHandler;
    var tableTitleString = this.props.tableTitle;
    var starting_height = this.props.starting_height;

    var formatted_number = numberWithCommas(num_rows);

    data = data["values"];

    for (var r = 0; r < data.length+1; r++) {
      cells = [];
      for (var c = 0; c < spec["column_names"].length+1; c++) {
          if(c === 0){
            cells.push((r === 0)?<Cell className="header" key={c}></Cell>:<Cell className="header_element" key={c}>{data[r-1]["__idx"]}</Cell>);
          }else{
           cells.push((r === 0)?<Cell className={"header"} key={c}><span>{spec["column_names"][c-1]}</span></Cell>:<Cell className="elements" onMouseEnter={this.hoverImage(spec["column_types"][c-1])} onMouseLeave={this.hoverOutImage(spec["column_types"][c-1])} key={c}>{ this.renderCell(data[r-1][spec["column_names"][c-1]], spec["column_types"][c-1] )}</Cell>);

          }
      }
      rows.push(<Row key={r}>{cells}</Row>);
    }

      var jsx_arr = [];
      var tableTitle = null;

      if(tableTitleString){
          // Take care of table padding OnInit (to prevent jumpy behavior), Value updated later
          starting_height = starting_height - 90;

          tableTitle = (
                            <h1 className="tableTitle"  key="tableTitle">
                                {tableTitleString}
                            </h1>
                        );
          jsx_arr.push(tableTitle);
      }

      var tableBody = (
                       <div className="resize_container" style={{ "height": starting_height}} key="tableBody">
                           <StickyTable>
                               {rows}
                           </StickyTable>
                         </div>
                       );

      var tableFooter = ( <div className="BottomTable" key="tableFooter">
                             &nbsp;
                             <div className="numberOfRows">
                                 {formatted_number} rows
                             </div>
                             <div className="jumpToRowContainer">
                                 <input className="rowNumber" id="rowNum" onKeyDown={this.enterPressJumpRow(rowHandler)} placeholder="Row #"/>
                                 <img className="enterSymbol" onClick={rowHandler.bind(this)} src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEYAAAAxCAYAAABnCd/9AAAAAXNSR0IArs4c6QAAA9RJREFUaAXtmk1oE0EUx80mahCiglKKpSAiHrTUSz1YBL9REEE8BBSqLQrRNm1BvPSk9ODHQSHpRywKwUMvxVIQrccqanvw5qEHD4LooQgeaqGY5svfK01Yt7t1d7Mpm00Wys6+nZl9/1/fm5mdrG9DlR0DAwNn8vn8HWRP9/T03DaS7zO64UV7LBa7jq4EfwHR5/f7d0ej0W9S1h6K1uDVa6D0o+0pf8tQRCdgQnLWO4qV9G56wTYyMrIxlUo9I32uWNHjaTDxeHwrUMaBcsoKFKnr2VRKJBINAHlvB4pnwQClKZ1OzyCwWUTaOTwXMUzHJ4DygUhptAOk0MZTY8zQ0NDebDY7CZTNBYF2z56KGIC0OAFFYHoNjGN6HOvIbsi6tZ1jYwxrhqOIjBDKiz6f7zHvIbNuFW3GL0cihuX2LR42BZRLnK9x/shAeMiMA26tUxIYAChESgxxjyirX0i3MzvccKtoM37ZTqVkMhlkzTAKkIt6D8LeomevFJstMIODgzsWFhZeIr7VSCjjjK2+jfpbb7vlVBoeHt6Ty+Wm14Ky3iLK8TxLYGRAzWQyM0DZVw5n3NSnaTAMsucZUN8Cpc5NAsrliykwDLIyw0wAZUu5HHFbv2uCAYSPNcp9xpQEZb/bnC+nP4Yzx9jY2CbSJ8nDL5fTAbf2rQuGfdJtc3NzEzh93K2Ol9uvVWCYeRrZJ31D6hwo5eG0r2NsuldKH1bb8sz9VtsY1f8HDKlzcGWjZ5dRAwv2nYxNfRbqu6pqcfBlNXsa4rJ57AQUV4m048wyGKBc5b/7mg4Mf4Cy03kltwmQPu2kj8w+tUNFQCF14qrrWnGFgKRSLX10wkFheyClY696k0SMfCtSOzQEFDatHxI1NThaMHINnH6+FWkHUFpzv2oviws8vix6rijKOeD8rgYaMrYGg8EfRlrVO/vLddi6bOZH8UkuGowambHz4HnqvTJTd73rsETJkCEvCAZD/1aBESflRZItTIHTZNdpwMySoiW9iNp9thPtiqmk7qyrq+s7YXYEcVNqezWVdcEIgEgkMl9fX38WOKPVBKSg1RCMVAiHw0vd3d1tFB8UGlTLeU0wAoGIyff29vYxY92knK2B0RAgcp4A5wJwFjW3PHn534hRq5bpjWnuGHB+qu1eLFsCIwCYsT4FAoHDwPniRSAFTZbBSMPOzs6vpFUrcKYLHWnPLKJyWlslXdsCIwJJq1+hUOgkxXEDwZ8N7BVhtg1G1HV0dPxhdRumKB8PqY8loqmit0t1XwnUCs2W+Sm3DRhR6susdRdg78y2dWO9vzv+PVLOdXkdAAAAAElFTkSuQmCC" height={8}/>

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


      jsx_arr.push(tableBody);
      jsx_arr.push(tableFooter);
      jsx_arr.push(imageContainer);

      var jsxTotal = jsx_arr[0];

      for(var x = 1; x < jsx_arr.length; x++){
          jsxTotal = [jsxTotal, jsx_arr[x]];
      }

      return (<div>{jsxTotal}</div>);
  }
}

export default Tctable;
