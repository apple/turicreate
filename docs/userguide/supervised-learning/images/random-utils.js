// Java script doesn't have seeded random numbers.
Math.seed = function(s) {
    return function() {
        s = Math.sin(s) * 10000; return s - Math.floor(s);
    };
};

// Random is now seeded.
var random1 = Math.seed(40);
var random2 = Math.seed(random1());
Math.random = Math.seed(random2());

// Random normal
function randn() {
  return ((Math.random() + Math.random() + Math.random() + Math.random()
         + Math.random() + Math.random()) - 3) / 3;
}


function gaussian_mixture(mean1, mean2, var1, var2){
  if (Math.random() > 0.5){
    return var1 * randn() + mean1;
  } else {
    return var2 * randn() + mean2;
  }
}

