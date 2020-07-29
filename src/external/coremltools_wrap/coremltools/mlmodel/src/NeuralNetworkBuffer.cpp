//
//  NeuralNetworkBuffer.cpp
//  CoreML
//
//  Created by Bhushan Sonawane on 11/8/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "NeuralNetworkBuffer.hpp"
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace NNBuffer {

    /*
     * getOpenMode - Returns open model as per the mode provided
     */
    static std::ios_base::openmode getOpenMode(bufferMode mode)
    {
        return (mode == bufferMode::read)
            ? (std::fstream::in | std::ios::binary)
            : (std::fstream::in | std::fstream::out | std::ios::binary
                                | (mode == bufferMode::write ? std::ios::trunc : std::ios::app));

    }

    /*
     * NeuralNetworkBuffer - NeuralNetworkBuffer
     */
    NeuralNetworkBuffer::NeuralNetworkBuffer(const std::string& bufferFilePath, bufferMode mode)
        : bufferFilePath(bufferFilePath),
          bufferStream(bufferFilePath, getOpenMode(mode))
    {
        if (!bufferStream) {
            throw std::runtime_error(std::string("Could not open buffer file '" + bufferFilePath + "': ") + std::strerror(errno) + '.');
        }
    }

    /*
     * NeuralNetworkBuffer - NeuralNetworkBuffer
     */
    NeuralNetworkBuffer::~NeuralNetworkBuffer() = default;

    /*
     * NeuralNetworkBuffer - addBuffer
     * Writes given data into buffer file
     * Writes in following order
     * [Length of data, data type, data]
     * Number of bytes written = Length_Of_Data * Size_Of_Data_Type
     */
    template<class T>
    uint64_t NeuralNetworkBuffer::addBuffer(const std::vector<T>& buffer) {
        bufferStream.seekp(0, std::ios::end);
        if (!bufferStream.good()) {
            throw std::runtime_error(std::string("Could not seek to end of data file: ") + std::strerror(errno) + '.');
        }

        // Get offset
        auto offset = bufferStream.tellp();

        // Write length, size of data type and buffer
        int64_t lenOfBuffer = static_cast<int64_t>(buffer.size());
        int64_t sizeOfBlock = sizeof(T);

        bufferStream.write((char*)&lenOfBuffer, sizeof(lenOfBuffer));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not write length of data file: ") + std::strerror(errno) + '.');
        }

        bufferStream.write((char*)&sizeOfBlock, sizeof(sizeOfBlock));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not write size of data block: ") + std::strerror(errno) + '.');
        }

        bufferStream.write((char*)&buffer[0], static_cast<std::streamsize>(sizeOfBlock * lenOfBuffer));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not write data to data file: ") + std::strerror(errno) + '.');
        }

        return static_cast<uint64_t>(offset);
    }

    /*
     * NeuralNetworkBuffer - getBuffer
     * Reads data from given offset
     */
    template<class T>
    void NeuralNetworkBuffer::getBuffer(const uint64_t offset, std::vector<T>& buffer) {
        int64_t lenOfBuffer = 0;
        int64_t sizeOfBlock = 0;

        bufferStream.seekg(static_cast<std::istream::off_type>(offset), std::ios::beg);
        if (!bufferStream.good()) {
            throw std::runtime_error(std::string("Could not seek to beginning of data file: ") + std::strerror(errno) + '.');
        }

        // Read length of buffer and size of each block
        bufferStream.read((char*)&lenOfBuffer, sizeof(lenOfBuffer));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not read length of data file: ") + std::strerror(errno) + '.');
        }

        bufferStream.read((char*)&sizeOfBlock, sizeof(sizeOfBlock));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not read size of data block: ") + std::strerror(errno) + '.');
        }

        // TODO: assert if sizeOfBlock != sizeof(T) or resize accordingly.
        // Resize buffer to fit buffer
        buffer.resize(static_cast<typename std::vector<T>::size_type>(lenOfBuffer));

        // Read buffer
        bufferStream.read((char*)&buffer[0], static_cast<std::streamsize>(sizeOfBlock * lenOfBuffer));
        if (bufferStream.fail()) {
            throw std::runtime_error(std::string("Could not read data from data file: ") + std::strerror(errno) + '.');
        }
    }

    // Explicit include templated functions
    template uint64_t NeuralNetworkBuffer::addBuffer(const std::vector<int32_t>&);
    template uint64_t NeuralNetworkBuffer::addBuffer(const std::vector<int64_t>&);
    template uint64_t NeuralNetworkBuffer::addBuffer(const std::vector<float>&);
    template uint64_t NeuralNetworkBuffer::addBuffer(const std::vector<double>&);

    template void NeuralNetworkBuffer::getBuffer(const uint64_t, std::vector<int32_t>&);
    template void NeuralNetworkBuffer::getBuffer(const uint64_t, std::vector<int64_t>&);
    template void NeuralNetworkBuffer::getBuffer(const uint64_t, std::vector<float>&);
    template void NeuralNetworkBuffer::getBuffer(const uint64_t, std::vector<double>&);
}
