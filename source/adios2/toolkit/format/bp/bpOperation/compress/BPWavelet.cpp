/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPWavelet.cpp
 *
 *  Created on: Feb 18, 2020
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BPWavelet.h"
#include "BPWavelet.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_WAVELET
#include "adios2/operator/compress/CompressWavelet.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BPWavelet::SetData(                                                     \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPWavelet::SetMetadata(                                                 \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BPWavelet::UpdateMetadata(                                              \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataCommon(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_WAVELET_TYPE_1ARG(declare_type)
#undef declare_type

void BPWavelet::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    const uint16_t levelID = helper::ReadValue<uint16_t>(buffer, position);
    info["LevelID"] = std::to_string(levelID);
    const std::string levelIDStr = std::to_string(levelID);
    if (levelID == 0)
    {
        info["ApproxCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["ApproxCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["ApproxCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["HorizCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["HorizCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["HorizCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    }
    else
    {
        info["HorizCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["HorizCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["HorizCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["VertCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsOffset_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsRows_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["DetailCoeffsCols_" + levelIDStr] = std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    }

}

void BPWavelet::GetData(const char *input,
                      const helper::BlockOperationInfo &blockOperationInfo,
                      char *dataOutput) const
{
#ifdef ADIOS2_HAVE_WAVELET
    core::compress::CompressWavelet op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  blockOperationInfo.Info.at("PreDataType"),
                  blockOperationInfo.Info);

#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with Wavelet, can't read Wavelet compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2