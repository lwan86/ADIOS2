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



#ifdef ADIOS2_HAVE_WAVELET
extern "C" {
#include <wavelib.h>
}
#endif

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
    size_t originalElementsCount = std::accumulate(variable.m_Count.begin(), variable.m_Count.end(),
                        static_cast<size_t>(1), std::multiplies<size_t>());
    std::cout << "original data count: " << originalElementsCount << std::endl;
    std::cout << "shape: [";
    for (auto dim : variable.m_Shape)
    {
        std::cout << dim << ", ";
    }
    std::cout << "]" << std::endl;
    const size_t ndims = variable.m_Shape.size();
    if (ndims > 2)
    {
        throw std::invalid_argument("ERROR: ADIOS2 Wavelet compression: no more "
                                    "than 2 dimension is supported.\n");
    }
    if (variable.m_Type != helper::GetType<double>())
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: ADIOS2 operator "
                "Wavelet only supports double precision, in call to Put\n");
        }
    }   
    const void *input = data;
    wave_object wvObj;
    wvObj = wave_init(m_WaveletName.c_str());
    double *waveletCoeffs;
    if (ndims == 1)
    {
        /* code */
    }
    else if (ndims == 2)
    {
        wt2_object wt;
        wt = wt2_init(wvObj, "dwt", variable.m_Shape[0], variable.m_Shape[1], m_WaveletLevels);
        waveletCoeffs = dwt2(wt, const_cast<double *>(static_cast<const double *>(input)));
/*         double *approxCoeffs;
        double *horizCoeffs;
        double *vertCoeffs;
        double *detailCoeffs;
        int ar, ac, hr, hc, vr, vc, dr, dc;
        approxCoeffs = getWT2Coeffs(wt, waveletCoeffs, 3, "A", &ar, &ac);
        horizCoeffs = getWT2Coeffs(wt, waveletCoeffs, 3, "H", &hr, &hc);
        vertCoeffs = getWT2Coeffs(wt, waveletCoeffs, 3, "V", &vr, &vc);
        detailCoeffs = getWT2Coeffs(wt, waveletCoeffs, 3, "D", &dr, &dc);
        size_t i, j;        
        std::cout << "approximation coefficients: " << std::endl;
	    for (i = 0; i < ar; ++i) 
        {
		    for (j = 0; j < ac; ++j) 
            {
                if (j < ac-1)
                {
                    std::cout << approxCoeffs[i*ac + j] << ", ";
                }
                else
                {
                    std::cout << approxCoeffs[i*ac + j] << std::endl;
                }
	        }
        }     
        std::cout << "horizontal coefficients: " << std::endl;
	    for (i = 0; i < hr; ++i) 
        {
		    for (j = 0; j < hc; ++j) 
            {
                if (j < hc-1)
                {
                    std::cout << horizCoeffs[i*hc + j] << ", ";
                }
                else
                {   
                    std::cout << horizCoeffs[i*hc + j] << std::endl;
                }
		    }
	    }      
        std::cout << "vertical coefficients: " << std::endl;
	    for (i = 0; i < vr; ++i) 
        {
		    for (j = 0; j < vc; ++j) 
            {
                if (j < vc-1)
                {
                    std::cout << vertCoeffs[i*vc + j] << ", ";
                }
                else
                {   
                    std::cout << vertCoeffs[i*vc + j] << std::endl;
                }
		    }
	    }  
        std::cout << "detail coefficients: " << std::endl;
	    for (i = 0; i < dr; ++i) 
        {
		    for (j = 0; j < dc; ++j) 
            {
                if (j < dc-1)
                {
                    std::cout << detailCoeffs[i*dc + j] << ", ";
                }
                else
                {
                    std::cout << detailCoeffs[i*dc + j] << std::endl;
                }
		    }
	    }  */

        size_t s = 0;
        size_t elementsOnCurrentTier = 0;
        for (size_t l = 0; l < m_WaveletLevels; l++)
        {
            if (s > m_StorageTiers)
            {
                break;
            }

            double percentage = std::stod(m_AllLevels[s]["percentage"]);
            std::cout << "storage tier: " << s << ", % of original data: " << percentage << std::endl;


            if (l == 0)
            {
                double *approxCoeffs;
                double *horizCoeffs;
                double *vertCoeffs;
                double *detailCoeffs;
                int ar, ac, hr, hc, vr, vc, dr, dc;
                approxCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "A", &ar, &ac);
                horizCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "H", &hr, &hc);
                vertCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "V", &vr, &vc);
                detailCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "D", &dr, &dc);  

                //Variable<T> var_A(variable.m_Name+"_"+std::to_string(l)+"_A", shape, start, count, constantDims, m_DebugMode);

                elementsOnCurrentTier = ar*ac+hr*hc+vr*vc+dr*dc;

            }
            else
            {
                double *horizCoeffs;
                double *vertCoeffs;
                double *detailCoeffs;
                int hr, hc, vr, vc, dr, dc;
                horizCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "H", &hr, &hc);
                vertCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "V", &vr, &vc);
                detailCoeffs = getWT2Coeffs(wt, waveletCoeffs, m_WaveletLevels-l, "D", &dr, &dc);            
                elementsOnCurrentTier = hr*hc+vr*vc+dr*dc;
            }
            

            if (elementsOnCurrentTier > percentage*originalElementsCount)    
            {
                elementsOnCurrentTier = 0;
                s++;
            }
            
                    // if first timestep Write create a new pg index
            // if (!m_AllBP4Serializers.at(i).m_MetadataSet.DataPGIsOpen)
            // {
            //     m_AllBP4Serializers.at(i).PutProcessGroupIndex(
            //         m_IO.m_Name, m_IO.m_HostLanguage,
            //         m_FileDataManager.GetTransportsTypes());
            // }

            // const size_t dataSize = helper::PayloadSize(blockInfo.Data, blockInfo.Count) +
            //     m_AllBP4Serializers.at(i).GetBPIndexSizeInData(variable.m_Name, blockInfo.Count);

            // const format::BP4Base::ResizeResult resizeResult =
            //     m_AllBP4Serializers.at(i).ResizeBuffer(dataSize, "in call to variable " +
            //                                             variable.m_Name + " Put");

            // if (resizeResult == format::BP4Base::ResizeResult::Flush)
            // {
            //     DoFlush(false, i);
            //     m_AllBP4Serializers.at(i).ResetBuffer(m_AllBP4Serializers.at(i).m_Data);

            //     // new group index for incoming variable
            //     m_AllBP4Serializers.at(i).PutProcessGroupIndex(
            //         m_IO.m_Name, m_IO.m_HostLanguage,
            //         m_FileDataManager.GetTransportsTypes());
            // }

            // // WRITE INDEX to data buffer and metadata structure (in memory)//
            // const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);
            // m_AllBP4Serializers.at(i).PutVariableMetadata(variable, blockInfo, sourceRowMajor);
            // m_AllBP4Serializers.at(i).PutVariablePayload(variable, blockInfo, sourceRowMajor);

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