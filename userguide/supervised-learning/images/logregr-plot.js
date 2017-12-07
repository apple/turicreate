// Sample and sort
$(function() {
  var xData = d3.range(1000).map(function (x) {
      return gaussian_mixture(-0.7, 0.7, 1, 1);
  });
  xData.sort(function (a,b){ return b-a;});

  // Define the three functions 
  function target(x){
    if (x > randn()) {
      y = 1.0;
    } else {
      y = 0.0
    }
    return y;
  }

  function logistic_function(x){
     x = 15.0 * x;
     return 0.5 + 0.5 * ((Math.exp(y) - Math.exp(-x)) / (Math.exp(y) + Math.exp(-x)));
  }
  
  function classifier(x, y){
    return  logistic_function(x) > y;
  }

  var yData = xData.map(target);

  // Setup the canvas
  var canvas = setup_canvas("logregr-plot", xData, yData, 1.01);
  canvas = set_title(canvas, "Logistic Regression");
  canvas = set_xaxis_title(canvas, "Feature (x)");
  canvas = set_yaxis_title(canvas, "Target (y)");

  // Draw points
  var color;
  for(var i = 0; i < xData.length; i++) {
    if (classifier(xData[i], yData[i])){
      color = GL_ORANGE;
    } else {
      color = GL_BLUE;
    }
    canvas = plot_point(canvas, color, xData[i], yData[i]);
  }

  canvas = plot_function(canvas, xData, logistic_function);
  canvas = add_legend_line(canvas, "Logistic Function", 0.72, 0.9);
  canvas = add_legend_point(canvas, "Positive examples", GL_BLUE, 0.72, 0.9);
  canvas = add_legend_point(canvas, "Negative examples", GL_ORANGE, 0.72, 0.9, 2);
});
