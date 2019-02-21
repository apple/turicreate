
#include <jni.h>
#include <stdio.h>

#include "E.h"

JNIEXPORT void JNICALL Java_E_printName(JNIEnv*, jobject)
{
  printf("E\n");
}
