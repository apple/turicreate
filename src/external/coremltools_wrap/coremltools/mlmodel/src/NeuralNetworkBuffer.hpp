//
//  NeuralNetworkBuffer.hpp
//  CoreML
//
//  Created by Bhushan Sonawane on 11/8/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#ifndef NeuralNetworkBuffer_hpp
#define NeuralNetworkBuffer_hpp

#include <string>
#include <vector>
#include <fstream>

namespace NNBuffer {
    //
    // NeuralNetworkBuffer - Network parameter read-write management to file
    // Current management policy
    // Each parameter is written to binary file in following order.
    // [Length of data (size_t), Data type of data (size_t), data (length of data * size of data type)]
    //

    enum bufferMode {
        write=0,
        append,
        read
    };

    class NeuralNetworkBuffer {
        private:
            std::string bufferFilePath;
            std::fstream bufferStream;

        public:
            // Must be constructed with file path to store parameters
            NeuralNetworkBuffer(const std::string&, bufferMode mode=bufferMode::write);
            ~NeuralNetworkBuffer();

            // Stores given buffer and returns offset in buffer file
            template<class T>
            uint64_t addBuffer(const std::vector<T>&);

            // Reads buffer from given offset and stores in vector
            // passed by reference.
            // Note that, this routine resizes the given vector.
            template<class T>
            void getBuffer(const uint64_t, std::vector<T>&);
    };
}
#endif /* NeuralNetworkBuffer_hpp */
