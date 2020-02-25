/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPWavelet.tcc
 *
 *  Created on: Feb 22, 2020
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPWAVELET_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPWAVELET_TCC_

#include "BPWavelet.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BPWavelet::SetMetadataCommon(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t inputSize = static_cast<uint64_t>(
        helper::GetTotalSize(blockInfo.Count) * sizeof(T));
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);
    info["InputSize"] = std::to_string(inputSize);

    uint16_t levelID = static_cast<uint16_t>(std::stoi(operation.Parameters.at("level_id")));
    uint16_t metadataSize;
    size_t coeffsMetadataSize;
    if (levelID == 0)
    {
        coeffsMetadataSize = 4*3*8;  
    }
    else
    {
        coeffsMetadataSize = 3*3*8;
    }
    
    metadataSize = 8 + 8 + 2+ coeffsMetadataSize;
    helper::InsertToBuffer(buffer, &metadataSize);
    helper::InsertToBuffer(buffer, &inputSize);
    info["OutputSizeMetadataPosition"] = std::to_string(buffer.size());

    constexpr uint64_t outputSize = 0;
    // dummy
    helper::InsertToBuffer(buffer, &outputSize);
    helper::InsertToBuffer(buffer, &levelID);
    info["CoeffsMetadataPosition"] = std::to_string(buffer.size());
    // inserting dummies to preallocate, updated in UpdateMetadataCommon
    buffer.resize(buffer.size() + coeffsMetadataSize);
}

template <class T>
void BPWavelet::UpdateMetadataCommon(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t inputSize = static_cast<uint64_t>(
        helper::GetTotalSize(blockInfo.Count) * sizeof(T));

    const uint64_t outputSize =
        static_cast<uint64_t>(std::stoll(operation.Info.at("OutputSize")));

    size_t backPosition = static_cast<size_t>(
        std::stoull(operation.Info.at("OutputSizeMetadataPosition")));

    helper::CopyToBuffer(buffer, backPosition, &outputSize);
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);

    backPosition = static_cast<size_t>(
        std::stoull(operation.Info.at("CoeffsMetadataPosition")));
    
    size_t levelID = std::stoi(operation.Parameters.at("level_id"));
    if (levelID == 0)
    {   
        const std::string levelIDStr = std::to_string(levelID);
        const uint64_t approxCoeffsOffset =
            std::stoull(info["ApproxCoeffsOffset_" + levelIDStr]);
        const uint64_t approxCoeffsRows =
            std::stoull(info["ApproxCoeffsRows_" + levelIDStr]);
        const uint64_t approxCoeffsCols =
            std::stoull(info["ApproxCoeffsCols_" + levelIDStr]);
        const uint64_t horizCoeffsOffset =
            std::stoull(info["HorizCoeffsOffset_" + levelIDStr]);   
        const uint64_t horizCoeffsRows =
            std::stoull(info["HorizCoeffsRows_" + levelIDStr]); 
        const uint64_t horizCoeffsCols =
            std::stoull(info["HorizCoeffsCols_" + levelIDStr]);   
        const uint64_t vertCoeffsOffset =
            std::stoull(info["VertCoeffsOffset_" + levelIDStr]);   
        const uint64_t vertCoeffsRows =
            std::stoull(info["VertCoeffsRows_" + levelIDStr]); 
        const uint64_t vertCoeffsCols =
            std::stoull(info["VertCoeffsCols_" + levelIDStr]);  
        const uint64_t detailCoeffsOffset =
            std::stoull(info["DetailCoeffsOffset_" + levelIDStr]);     
        const uint64_t detailCoeffsRows =
            std::stoull(info["DetailCoeffsRows_" + levelIDStr]);  
        const uint64_t detailCoeffsCols =
            std::stoull(info["DetailCoeffsCols_" + levelIDStr]);   
        helper::CopyToBuffer(buffer, backPosition, &approxCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &approxCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &approxCoeffsCols);
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsCols);    
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsCols);      
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsCols);    
    }
    else
    {
        const std::string levelIDStr = std::to_string(levelID);
        const uint64_t horizCoeffsOffset =
            std::stoull(info["HorizCoeffsOffset_" + levelIDStr]);   
        const uint64_t horizCoeffsRows =
            std::stoull(info["HorizCoeffsRows_" + levelIDStr]); 
        const uint64_t horizCoeffsCols =
            std::stoull(info["HorizCoeffsCols_" + levelIDStr]);   
        const uint64_t vertCoeffsOffset =
            std::stoull(info["VertCoeffsOffset_" + levelIDStr]);   
        const uint64_t vertCoeffsRows =
            std::stoull(info["VertCoeffsRows_" + levelIDStr]); 
        const uint64_t vertCoeffsCols =
            std::stoull(info["VertCoeffsCols_" + levelIDStr]);  
        const uint64_t detailCoeffsOffset =
            std::stoull(info["DetailCoeffsOffset_" + levelIDStr]);     
        const uint64_t detailCoeffsRows =
            std::stoull(info["DetailCoeffsRows_" + levelIDStr]);  
        const uint64_t detailCoeffsCols =
            std::stoull(info["DetailCoeffsCols_" + levelIDStr]); 
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &horizCoeffsCols);    
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &vertCoeffsCols);      
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsOffset);
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsRows);
        helper::CopyToBuffer(buffer, backPosition, &detailCoeffsCols); 
    }    

    info.erase("OutputSizeMetadataPosition");
    info.erase("CoeffsMetadataPosition");
}

} // end namespace format
} // end namespace adios2

#endif /** ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPWAVELET_TCC_ */
