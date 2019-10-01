/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_utf8_h
#define cm_utf8_h

#ifdef __cplusplus
extern "C" {
#endif

/** Decode one UTF-8 character from the input byte range.  On success,
    stores the unicode character number in *pc and returns the first
    position not extracted.  On failure, returns 0.  */
const char* cm_utf8_decode_character(const char* first, const char* last,
                                     unsigned int* pc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
