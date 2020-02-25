/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressWavelet.h :
 *
 *  Created on: Feb 18, 2020
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_OPERATOR_COMPRESS_COMPRESSWAVELET_H_
#define ADIOS2_OPERATOR_COMPRESS_COMPRESSWAVELET_H_

#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

class CompressWavelet : public Operator
{

public:
    /**
     * Unique constructor
     * @param debugMode
     */
    CompressWavelet(const Params &parameters, const bool debugMode);

    ~CompressWavelet() = default;

    /**
     * Compression signature for legacy libraries that use void*
     * @param dataIn
     * @param dimensions
     * @param type
     * @param bufferOut
     * @param parameters
     * @return size of compressed buffer in bytes
     */
    size_t Compress(const void *dataIn, const Dims &dimensions,
                    const size_t elementSize, const std::string type,
                    void *bufferOut, const Params &parameters,
                    Params &info) const final;

    /**
     *
     * @param bufferIn
     * @param sizeIn
     * @param dataOut
     * @param dimensions
     * @param varType
     * @param
     * @return
     */
    size_t Decompress(const void *bufferIn, const size_t sizeIn, void *dataOut,
                      const Dims &dimensions, const std::string varType,
                      const Params & /*parameters*/) const final;
};

} // end namespace compress
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_OPERATOR_COMPRESS_COMPRESSWAVELET_H_ */
