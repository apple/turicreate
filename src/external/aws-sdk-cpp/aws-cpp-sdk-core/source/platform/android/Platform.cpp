/*
* Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#include <aws/core/platform/Android.h>

#include <iostream>
#include <chrono>
#include <thread>

namespace Aws
{
namespace Platform
{

JavaVM* g_javaVM(nullptr);

Aws::String g_cacheDirectory;

static void InitCacheDirectory(JNIEnv* env, jobject context)
{
    jclass contextClass = env->FindClass( "android/content/Context" );
    jmethodID getCacheDir = env->GetMethodID( contextClass, "getCacheDir", "()Ljava/io/File;" );
    jobject cache_dir = env->CallObjectMethod( context, getCacheDir );

    jclass fileClass = env->FindClass( "java/io/File" );
    jmethodID getPath = env->GetMethodID( fileClass, "getPath", "()Ljava/lang/String;" );
    jstring path_string = (jstring)env->CallObjectMethod( cache_dir, getPath );

    const char *path_chars = env->GetStringUTFChars( path_string, NULL );
    g_cacheDirectory = path_chars;
    g_cacheDirectory.append("/");

    env->ReleaseStringUTFChars( path_string, path_chars );
}

/*
We intentionally don't use JNI_OnLoad to do this because that wouldn't work if the sdk is linked statically into external user code.
 */
void InitAndroid(JNIEnv* env, jobject context)
{
    // save off the vm
    env->GetJavaVM(&g_javaVM);

    // cache anything else we need
    InitCacheDirectory(env, context);
}

JavaVM* GetJavaVM()
{
    return g_javaVM;
}

const char* GetCacheDirectory()
{
    return g_cacheDirectory.c_str();
}

} // namespace Platform
} // namespace Aws
