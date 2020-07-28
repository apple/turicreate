//
//  ResultReason.hpp
//  CoreML
//
//  Created by Jeff Kilpatrick on 12/16/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#pragma once

namespace CoreML {

/**
 Super specific reasons for non-good Results.
 */
enum class ResultReason {
    UNKNOWN,

    // -----------------------------------------
    // Model validation
    MODEL_INPUT_TYPE_INVALID,
    MODEL_OUTPUT_TYPE_INVALID,

    // -----------------------------------------
    // Program validation
    MODEL_MAIN_IMAGE_INPUT_SIZE_BAD,
    MODEL_MAIN_IMAGE_INPUT_TYPE_BAD,
    MODEL_MAIN_IMAGE_OUTPUT_SIZE_BAD,
    MODEL_MAIN_IMAGE_OUTPUT_TYPE_BAD,
    MODEL_MAIN_INPUT_COUNT_MISMATCHED,
    MODEL_MAIN_INPUT_OUTPUT_MISSING,
    MODEL_MAIN_INPUT_OUTPUT_TYPE_INVALID,
    MODEL_MAIN_INPUT_RANK_MISMATCHED,
    MODEL_MAIN_INPUT_SHAPE_MISMATCHED,
    MODEL_MAIN_INPUT_TYPE_MISMATCHED,
    MODEL_MAIN_OUTPUT_COUNT_MISMATCHED,
    MODEL_MAIN_OUTPUT_RANK_MISMATCHED,
    MODEL_MAIN_OUTPUT_SHAPE_MISMATCHED,
    MODEL_MAIN_OUTPUT_TYPE_MISMATCHED,

    OP_INVALID_IN_CONTEXT,
    
    PROGRAM_MAIN_FUNCTION_MISSING,
    PROGRAM_NULL,
    PROGRAM_PARSE_THREW,
    PROGRAM_VALIDATION_FAILED
};

}
