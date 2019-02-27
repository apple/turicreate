var assert = require("assert")
var test  = {
  version: "1.0.0"
}
function covTest(p1,p2) {
  if (p1 > 3) {
    return 1;
  }
  else {
    return p1 + p2;
  }
}

function covTest2(p1,p2) {
  return 0;
}

function covTest3(p1) {
  for(i=0;i < p1;i++){
  }
  return i;
}
function covTest4(p1) {
  i=0;
  while(i < p1){
  i++;
  }
  return i;
}

describe('Array', function(){
  describe('CovTest', function(){
    it('should return when the value is not present', function(){
      assert.equal(4,covTest(2,2));
    })
  })

  describe('CovTest>3', function(){
    it('should return when the value is not present', function(){
      assert.equal(1,covTest(4,2));
    })
  })
  describe('covTest4', function(){
    it('should return when the value is not present', function(){
      assert.equal(5,covTest4(5));
    })
  })
  describe('covTest3', function(){
    it('should return when the value is not present', function(){
      assert.equal(5,covTest3(5));
    })
  })
})
