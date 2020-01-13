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
    for (size_t i = 0; i < m_Levels; i++)
    {
        std::cout << "level " << i << std::endl;
        size_t originalDataSize = helper::PayloadSize(data, variable.m_Count);
        size_t originalDataCount = std::accumulate(variable.m_Count.begin(), variable.m_Count.end(),
                           static_cast<size_t>(1), std::multiplies<size_t>());
        size_t actualCount = originalDataCount/std::pow(2, i);
        std::cout << "original data count: " << originalDataCount << ", actual data count: " << actualCount << std::endl; 
        T *actualData = new T[actualCount];
        std::cout << "original data: " << std::endl;
        std::cout << "[";
        size_t count = 0;
        for (size_t j = 0; j < originalDataCount; j++)
        {
            std::cout << data[j] << ", ";
            if (j%int(std::pow(2, i)) == 0)
            {
                if (count == actualCount)
                {
                    break;
                }
                *actualData = data[j];
                actualData++;
                count++;
            }        
        }
        std::cout << "]" << std::endl;
        size_t actualDataSize = actualCount*sizeof(T);
        std::cout << "original data size: " << originalDataSize << ", actual data size: " << actualDataSize << std::endl;
        variable.SetLevelBlockInfo(actualData, i, actualDataSize, CurrentStep());
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
            std::cout << "      steps start: " << info.StepsStart << std::endl;
            std::cout << "      steps count: " << info.StepsStart << std::endl;
        }
        
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
