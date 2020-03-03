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
#    include <aws/checksums/crc.h>
#    include <aws/checksums/crc_jni.h>

jint JNICALL Java_software_amazon_awschecksums_AWSCRC32C_crc32c(
    JNIEnv *env,
    jobject obj,
    jbyteArray data,
    jint offset,
    jint length,
    jint previous_crc) {
    jbyte *buffer_ptr = (*env)->GetPrimitiveArrayCritical(env, data, 0);
    int ret_val = (int)aws_checksums_crc32c((const uint8_t *)(buffer_ptr + offset), length, previous_crc);
    (*env)->ReleasePrimitiveArrayCritical(env, data, buffer_ptr, 0);

    return ret_val;
}

jint JNICALL Java_software_amazon_awschecksums_AWSCRC32C_crc32cDirect(
    JNIEnv *env,
    jobject obj,
    jobject data,
    jint offset,
    jint length,
    jint previous_crc) {
    jbyte *buf_ptr = (*env)->GetDirectBufferAddress(env, data);
    return aws_checksums_crc32c((const uint8_t *)(buf_ptr + offset), length, previous_crc);
}

jint JNICALL Java_software_amazon_awschecksums_AWSCRC32_crc32(
    JNIEnv *env,
    jobject obj,
    jbyteArray data,
    jint offset,
    jint length,
    jint previous_crc) {
    jbyte *buffer_ptr = (*env)->GetPrimitiveArrayCritical(env, data, 0);
    int ret_val = (int)aws_checksums_crc32((const uint8_t *)(buffer_ptr + offset), length, previous_crc);
    (*env)->ReleasePrimitiveArrayCritical(env, data, buffer_ptr, 0);

    return ret_val;
}

jint JNICALL Java_software_amazon_awschecksums_AWSCRC32_crc32Direct(
    JNIEnv *env,
    jobject obj,
    jobject data,
    jint offset,
    jint length,
    jint previous_crc) {
    jbyte *buf_ptr = (*env)->GetDirectBufferAddress(env, data);
    return aws_checksums_crc32((const uint8_t *)(buf_ptr + offset), length, previous_crc);
}

#endif
