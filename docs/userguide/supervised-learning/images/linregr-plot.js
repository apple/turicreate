$(function() {
  // Sample and sort
  var xData = d3.range(250).map(function (x) {
      return randn();
  });
  xData.sort(function (a,b){ return b-a;});

  // Define the three functions 
  function decision_boundary(x){
     return 15.0 * x;
  }
  function target(x){
    return decision_boundary(x) + 9. * randn();
  }

  var yData = xData.map(target);

  // Setup the canvas
  var canvas = setup_canvas("linregr-plot", xData, yData, 1.01);
  canvas = set_title(canvas, "Linear Regression");

  // Draw points
  var color;
  for(var i = 0; i < xData.length; i++) {
    canvas = plot_point(canvas, GL_BLUE, xData[i], yData[i]);
  }

  canvas = plot_function(canvas, xData, decision_boundary);
  canvas = set_xaxis_title(canvas, "Feature (x)");
  canvas = set_yaxis_title(canvas, "Target (y)");
  canvas = add_legend_line(canvas, "Predicted function", 0.72, 0.9);
  canvas = add_legend_point(canvas, "Training data", GL_BLUE, 0.72, 0.9);
});
