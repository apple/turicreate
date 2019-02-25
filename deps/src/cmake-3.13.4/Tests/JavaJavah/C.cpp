
#include <jni.h>
#include <stdio.h>

#include "C.h"

JNIEXPORT void JNICALL Java_C_printName(JNIEnv*, jobject)
{
  printf("C\n");
}
