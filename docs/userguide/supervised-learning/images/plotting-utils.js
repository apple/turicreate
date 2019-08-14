// Global constants for size of plots
var width = 800;
var height = 600;
var PADDING = 50;

var GL_ORANGE = "#ff5500";
var GL_GREEN = "#85bd00";
var GL_BLUE = "#0a8cc4";
var GL_PINK = "#b0007f";


function setup_canvas(div_id, xData, yData, vShrink, hShrink){
    div_id = div_id || "body";
    vShrink = vShrink || 1.0;
    hShrink = hShrink || 1.0;

    var desired_width = 0.55 * $('#' + div_id).parent().width();
    var desired_height = desired_width * 600 / 800;
    var scale_factor = desired_width / width;

    var xMin = Math.min.apply(null, xData),
        xMax = Math.max.apply(null, xData);
    var yMin = Math.min.apply(null, yData),
        yMax = Math.max.apply(null, yData);
    var xScale = d3.scale.linear()
                   .domain([xMin, xMax * hShrink])
                   .range([0,width]);
    var yScale = d3.scale.linear()
                   .domain([yMin, yMax * vShrink])
                   .range([height, 0]);

		var svg = d3.select('#' + div_id)
                .append("svg")
		             .attr("width", desired_width + 2 * PADDING)
		             .attr("height", desired_height + 2 * PADDING)
		             .attr("style", "margin-left:auto;margin-right:auto;display:block")
		             .append("g")
		             .attr("transform", "translate("+PADDING+","+PADDING+") scale(" + scale_factor + "," + scale_factor + ")");

		var xAxis = d3.svg.axis().scale(xScale).orient("bottom").ticks([]);
		var yAxis = d3.svg.axis().scale(yScale).orient("left").ticks([]);

		// X-axis Title
		svg.append("g")
		    .attr("class", "x axis")
		    .attr("transform", "translate(0," + height + ")")
		    .call(xAxis);

		// Y-axis Title
		svg.append("g")
		      .attr("class", "y axis")
		      .call(yAxis);

    return {
      svg: svg,
      xScale: xScale,
      yScale: yScale,
      xMin: xMin,
      xMax: xMax * hShrink,
      yMin: yMin,
      yMax: yMax * hShrink
    };

}

function set_title(canvas, title){
    canvas.svg.append("text")
		    .attr("x", width/2)
		    .attr("y", -20)
        .attr("text-anchor", "middle")  
        .style("font-size", 34) 
		    .attr("font-weight", 500)
		    .attr("font-family", "Helvetica Neue")
        .text(title);
    return canvas;
}


function set_yaxis_title(canvas, title){
    canvas.svg.append("text")
		    .attr("x", -height/2)
		    .attr("y", -20)
        .attr("text-anchor", "middle")  
        .style("font-size", 30) 
		    .attr("font-weight", 400)
		    .attr("transform", "rotate(-90)")
		    .attr("font-family", "Helvetica Neue")
        .text(title);
    return canvas;
}


function set_xaxis_title(canvas, title){
    canvas.svg.append("text")
		    .attr("x", width/2)
		    .attr("y", height + 40)
        .attr("text-anchor", "middle")  
        .style("font-size", 30) 
		    .attr("font-weight", 400)
		    .attr("font-family", "Helvetica Neue")
        .text(title);
    return canvas;
}

// Set title
function plot_point(canvas, color, x, y){
		canvas.svg.append("circle")
		   .attr("class", "dot")
		   .attr("fill", color)
		   .attr("opacity", 0.6)
		   .attr("r", 7.0)
		   .attr("cx", canvas.xScale(x))
    	 .attr("cy", canvas.yScale(y));
    return canvas;
}

// Set title
function plot_area_under_function(canvas, xData, f, color, flag_above){
		var areaFunction;
    if (flag_above == true){
      areaFunction = d3.svg.area()
                       .x(function(d) { return  canvas.xScale(d); })
                       .y0(function(d) { return canvas.yScale(f(d)); })
		                   .y1(canvas.yScale(canvas.yMax));
    } else {
      areaFunction = d3.svg.area()
                       .x(function(d) { return  canvas.xScale(d); })
		                   .y0(canvas.yScale(canvas.yMin))
                       .y1(function(d) { return canvas.yScale(f(d)); });
    }
		var areaGraph = canvas.svg.append("path")
	                 .attr("d", areaFunction(xData))
	                 .attr("fill", color)
	                 .attr("opacity", 0.1);
    return canvas;
}

// Set title
function plot_function(canvas, xData, f){
		var lineFunction = d3.svg.line()
                .x(function(d) { return  canvas.xScale(d); })
                .y(function(d) { return  canvas.yScale(f(d)); })
		            .interpolate("monotone");

		var lineGraph = canvas.svg.append("path")
	                         .attr("d", lineFunction(xData))
	                         .attr("stroke", GL_PINK)
	                         .attr("stroke-width", 4)
	                         .attr("fill", "none");
    return canvas;
}


function add_legend_point(canvas, name, color, xloc, yloc, count){
  count = count || 1;
  var legend = canvas.svg.append("g")
	legend.append("text")
	    .attr("x", width * xloc)
	    .attr("y", height * yloc - count * 50)
	    .attr("dy", ".2em")
	    .style("text-anchor", "start")
      .attr("font-size", 28) 
	    .attr("font-weight", 300)
	    .attr("font-family", "Helvetica Neue")
	    .text(name)
	legend.append("circle")
			.attr("class", "dot")
	    .attr("fill", color)
		   .attr("opacity", 0.6)
		   .attr("r", 7.0)
	    .attr("cx", xloc * width - 40)
  		.attr("cy", yloc * height - 50 * count);
  return canvas;
}

function add_legend_line(canvas, name, xloc, yloc){
  var legend = canvas.svg.append("g")
  legend.append("text")
      .attr("x", xloc * width)
      .attr("y", yloc * height )
      .attr("dy", ".2em")
      .style("text-anchor", "start")
      .attr("font-size", 28) 
      .attr("font-weight", 300)
      .attr("font-family", "Helvetica Neue")
      .text(name)
   legend.append("line")
       .attr("stroke", "#b0007f")
       .attr("stroke-width", 4)
       .attr("x1", xloc * width - 60)
       .attr("y1", yloc * height)
       .attr("x2", xloc * width - 20)
       .attr("y2", yloc * height)
  return canvas;
}
