
#include <jni.h>
#include <stdio.h>

#include "D.h"

JNIEXPORT void JNICALL Java_D_printName(JNIEnv*, jobject)
{
  printf("D\n");
}
