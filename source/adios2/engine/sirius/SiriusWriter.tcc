/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SiriusWriter.tcc implementation of template functions with known type
 *
 *  Created on: Dec 04, 2019
 *      Author: Lipeng Wan wanl@ornl.gov
 */
#ifndef ADIOS2_ENGINE_SIRIUSWRITER_TCC_
#define ADIOS2_ENGINE_SIRIUSWRITER_TCC_

#include "SiriusWriter.h"

// compress
#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif

#ifdef ADIOS2_HAVE_WAVELET
#include "adios2/operator/compress/CompressWavelet.h"
#endif

#include "adios2/core/Operator.h"

#include <iostream>
#include <cmath>

namespace adios2
{
namespace core
{
namespace engine
{

template <class T>
void SiriusWriter::PutSyncCommon(Variable<T> &variable, const T *data)
{
    // size_t preCount = 0;
    for (size_t i = 0; i < m_Levels; i++)
    {
        std::cout << "level " << i << std::endl;
        // size_t originalDataSize = helper::PayloadSize(data, variable.m_Count);
        size_t originalElementsCount = std::accumulate(variable.m_Count.begin(), variable.m_Count.end(),
                            static_cast<size_t>(1), std::multiplies<size_t>());
        double percentage = std::stod(m_AllLevels[i]["percentage"]);
        std::cout << "% of original data: " << percentage << std::endl;

        //Params parameters = {};
        Params parameters = {{"wavelet_name", "haar"}, {"total_levels", "3"}, {"level_id", std::to_string(i)}};
        //adios2::core::compress::CompressBZIP2 op(parameters, m_DebugMode);
        adios2::core::compress::CompressWavelet op(parameters, m_DebugMode);
        std::vector<adios2::core::VariableBase::Operation> levelOperations;
        levelOperations.push_back(adios2::core::VariableBase::Operation{&op, parameters, Params()});
        std::cout << levelOperations.back().Op->m_Type << std::endl;
 
        const typename Variable<T>::Info blockInfo = variable.SetLevelBlockInfo(data, i, levelOperations, CurrentStep(i), 1);
        std::cout << "  level id has been set to " << variable.m_AllLevels[i].LevelID << std::endl;
        for (auto info : variable.m_AllLevels[i].LevelBlocksInfo)
        {
            std::cout << "    block info: " << std::endl;
            std::cout << "      block id: " << info.BlockID << std::endl;
            std::cout << "      block shape: [";
            for (auto dim : info.Shape)
            {
                std::cout << dim << ", ";
            }
            std::cout << "]" << std::endl;
            std::cout << "      block start: [";
            for (auto dim : info.Start)
            {
                std::cout << dim << ", ";
            }
            std::cout << "]" << std::endl;            
            std::cout << "      block shape: [";
            for (auto dim : info.Count)
            {
                std::cout << dim << ", ";
            }
            std::cout << "]" << std::endl;   
            std::cout << "      operations: [";
            for (auto operation : info.Operations)
            {
                std::cout << operation.Op->m_Type << ", ";
            }
            std::cout << "]" << std::endl;          
            std::cout << "      steps start: " << info.StepsStart << std::endl;
            std::cout << "      steps count: " << info.StepsStart << std::endl;
        }
                // if first timestep Write create a new pg index
        if (!m_AllBP4Serializers.at(i).m_MetadataSet.DataPGIsOpen)
        {
            m_AllBP4Serializers.at(i).PutProcessGroupIndex(
                m_IO.m_Name, m_IO.m_HostLanguage,
                m_FileDataManager.GetTransportsTypes());
        }

        const size_t dataSize = helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
            m_AllBP4Serializers.at(i).GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);

        const format::BP4Base::ResizeResult resizeResult =
            m_AllBP4Serializers.at(i).ResizeBuffer(dataSize, "in call to variable " +
                                                    variable.m_Name + " Put");

        if (resizeResult == format::BP4Base::ResizeResult::Flush)
        {
            DoFlush(false, i);
            m_AllBP4Serializers.at(i).ResetBuffer(m_AllBP4Serializers.at(i).m_Data);

            // new group index for incoming variable
            m_AllBP4Serializers.at(i).PutProcessGroupIndex(
                m_IO.m_Name, m_IO.m_HostLanguage,
                m_FileDataManager.GetTransportsTypes());
        }

        // WRITE INDEX to data buffer and metadata structure (in memory)//
        const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
        m_AllBP4Serializers.at(i).PutVariableMetadata(variable, blockInfo, sourceRowMajor);
        m_AllBP4Serializers.at(i).PutVariablePayload(variable, blockInfo, sourceRowMajor);

    }

}

// template <class T>
// void SiriusWriter::PutDeferredCommon(Variable<T> &variable, const T *data)
// {
//     variable.SetBlockInfo(data, CurrentStep());
    
//     if (m_Verbosity == 5)
//     {
//         std::cout << "Sirius Writer " << m_WriterRank << "     PutDeferred("
//                   << variable.m_Name << ")\n";
//     }
//     m_NeedPerformPuts = true;
// }

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SIRIUSWRITER_TCC_ */
