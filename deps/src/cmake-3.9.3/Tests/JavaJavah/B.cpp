
#include <jni.h>
#include <stdio.h>

#include "B.h"

JNIEXPORT void JNICALL Java_B_printName(JNIEnv*, jobject)
{
  printf("B\n");
}
