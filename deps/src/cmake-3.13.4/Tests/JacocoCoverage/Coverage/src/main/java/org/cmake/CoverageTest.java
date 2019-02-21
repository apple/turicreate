package org.cmake.Coverage;

import java.io.Serializable;
import java.util.Map;
import java.util.List;
import java.awt.*;

public class CoverageTest {

  public static String VarOne = "test1";
  public static String VarTwo = "test2";
  private Integer IntOne = 4;

  public static Boolean equalsVarOne(String inString) {

    if(VarOne.equals(inString)){
      return true;
    }
    else {
      return false;
    }
  }

  public static boolean equalsVarTwo(String inString){

    if(VarTwo.equals(inString)){
      return true;
    }
    else {
      return false;
    }
  }

  private Integer timesIntOne(Integer inVal){

    return inVal * IntOne;
  }

  public static boolean whileLoop(Integer StopInt){

    Integer i = 0;
    while(i < StopInt){
      i=i+1;
    }
    if (i.equals(5)){
     return true;
    }
    else {
     return false;
    }
  }
}
