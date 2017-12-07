$(function() {
  // Sample and sort
  var xData = d3.range(250).map(function (x) {
      return randn();
  });
  xData.sort(function (a,b){ return b-a;});

  // Define the three functions 
  function decision_boundary(x){
     return -25 * x * x + 215 * (Math.sin(.08 * x + 1));
  }
  function target(x){
    return decision_boundary(x) + 5. * randn();
  }

  var yData = xData.map(target);

  // Setup the canvas
  var canvas = setup_canvas("supervised-plot", xData, yData, 1.01);
  canvas = set_title(canvas, "Regression");
  canvas = set_xaxis_title(canvas, "Feature (x)");
  canvas = set_yaxis_title(canvas, "Target (y)");

  // Draw points
  var color;
  for(var i = 0; i < xData.length; i++) {
    canvas = plot_point(canvas, GL_BLUE, xData[i], yData[i]);
  }

  canvas = plot_function(canvas, xData, decision_boundary);
  canvas = add_legend_line(canvas, "Predicted function", 0.72, 0.9);
  canvas = add_legend_point(canvas, "Training data", GL_BLUE, 0.72, 0.9);
});
