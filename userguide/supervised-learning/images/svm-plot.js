// Sample and sort
$(function() {
  var xData = d3.range(1000).map(function (x) {
      return gaussian_mixture(-0.5, 0.5, 1, 2);
  });
  xData.sort(function (a,b){ return b-a;});

  // Define the three functions 

  function target(x){
    if (x > randn()) {
      y = 10 + 10 * randn();
    } else {
      y = 10 * randn();
    }
    return y;
  }

  function decision_boundary(x){
    return -2.5 * x + 5.0;
  }

  function classifier(x, y){
    return  decision_boundary(x) + 7 * randn()  >= y;
  }

  var yData = xData.map(target);

  // Setup the canvas
  var canvas = setup_canvas("svm-plot", xData, yData, 1.01);

  canvas = set_title(canvas, "Support Vector Machines");

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

  canvas = plot_function(canvas, xData, decision_boundary);
  canvas = plot_area_under_function(canvas, xData, decision_boundary,
                                                             GL_ORANGE, true);
  canvas = plot_area_under_function(canvas, xData, decision_boundary,
                                                             GL_BLUE, false);

  canvas = add_legend_line(canvas, "Decision boundary", 0.70, 0.9);
  canvas = add_legend_point(canvas, "Positive examples", GL_BLUE, 0.72, 0.9);
  canvas = add_legend_point(canvas, "Negative examples", GL_ORANGE, 0.72, 0.9, 2);
});
