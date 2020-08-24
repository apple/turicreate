#ifndef AWS_CHECKSUMS_CRC_JNI_H
#define AWS_CHECKSUMS_CRC_JNI_H

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

#ifdef BUILD_JNI_BINDINGS
#    include <jni.h>

#    ifdef __cplusplus
extern "C" {
#    endif
JNIEXPORT jint JNICALL
    Java_software_amazon_awschecksums_AWSCRC32C_crc32c(JNIEnv *, jobject, jbyteArray, jint, jint, jint);
JNIEXPORT jint JNICALL
    Java_software_amazon_awschecksums_AWSCRC32C_crc32cDirect(JNIEnv *, jobject, jobject, jint, jint, jint);

JNIEXPORT jint JNICALL
    Java_software_amazon_awschecksums_AWSCRC32_crc32(JNIEnv *, jobject, jbyteArray, jint, jint, jint);
JNIEXPORT jint JNICALL
    Java_software_amazon_awschecksums_AWSCRC32_crc32Direct(JNIEnv *, jobject, jobject, jint, jint, jint);

#    ifdef __cplusplus
}
#    endif

#endif
#endif /* AWS_CHECKSUMS_CRC_JNI_H */
