/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * it->first ==ompanying file Copyright.txt for details.
 *
 * CompressWavelet.cpp
 *
 *  Created on: Feb 18, 2020
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "CompressWavelet.h"

#include <cmath>     //std::ceil
#include <ios>       //std::ios_base::failure
#include <stdexcept> //std::invalid_argument

extern "C" {
#include <wavelib.h>
}

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressWavelet::CompressWavelet(const Params &parameters, const bool debugMode)
: Operator("wavelet", parameters, debugMode)
{
}


size_t CompressWavelet::Compress(const void *dataIn, const Dims &dimensions,
                            const size_t elementSize, const std::string varType,
                            void *bufferOut, const Params &parameters,
                            Params &info) const
{
    const size_t ndims = dimensions.size();
    if (ndims > 2)
    {
        throw std::invalid_argument("ERROR: ADIOS2 Wavelet compression: no more "
                                    "than 2 dimension is supported.\n");
    }
    if (varType != helper::GetType<double>())
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: ADIOS2 operator "
                "Wavelet only supports double precision, in call to Put\n");
        }
    }

    const char *waveletName;
    size_t totalLevels;
    size_t levelID;
    double *waveletCoeffs;
    double *approxCoeffs;
    double *horizCoeffs;
    double *vertCoeffs;
    double *detailCoeffs;
    for (const auto &itParameter : parameters)
    {
        const std::string key = itParameter.first;
        const std::string value = itParameter.second;

        if (key == "wavelet_name" || key == "wvname")
        {
            waveletName = value.c_str();
        }
        else if (key == "total_levels")
        {
            totalLevels = std::stoi(value);
        }
        else if (key == "level_id")
        {
            levelID = std::stoi(value);
        }
        
    }
	wave_object wvObj;
    wvObj = wave_init(waveletName);
    size_t coeffsSize = 0;
    if (ndims == 1)
    {
        /* code */
    }
    else if (ndims == 2)
    {
        wt2_object wt;
        int ar, ac, hr, hc, vr, vc, dr, dc;
        wt = wt2_init(wvObj, "dwt", dimensions[0], dimensions[1], totalLevels);
        waveletCoeffs = dwt2(wt, const_cast<double *>(static_cast<const double *>(dataIn)));
        const std::string levelIDStr = std::to_string(levelID);
        std::cout << "level id: " << levelID << std::endl;
        if (levelID == 0)
        {
            approxCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "A", &ar, &ac);
            horizCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "H", &hr, &hc);
            vertCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "V", &vr, &vc);
            detailCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "D", &dr, &dc);
            size_t i, j;
            std::cout << "approximation coefficients: " << std::endl;
	        for (i = 0; i < ar; ++i) {
		        for (j = 0; j < ac; ++j) {

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
	        for (i = 0; i < hr; ++i) {
		        for (j = 0; j < hc; ++j) {

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
	        for (i = 0; i < vr; ++i) {
		        for (j = 0; j < vc; ++j) {

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
	        for (i = 0; i < dr; ++i) {
		        for (j = 0; j < dc; ++j) {

                    if (j < dc-1)
                    {
                        std::cout << detailCoeffs[i*dc + j] << ", ";
                    }
                    else
                    {
                        std::cout << detailCoeffs[i*dc + j] << std::endl;
                    }
		        }
	        } 
            info["ApproxCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["ApproxCoeffsRows_" + levelIDStr] = std::to_string(ar);
            info["ApproxCoeffsCols_" + levelIDStr] = std::to_string(ac);
            const size_t sizeOutApproxCoeffs = static_cast<size_t>(ar*ac*sizeof(double));
            std::memcpy(bufferOut, approxCoeffs, sizeOutApproxCoeffs);
            coeffsSize += sizeOutApproxCoeffs;
            info["HorizCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["HorizCoeffsRows_" + levelIDStr] = std::to_string(hr);
            info["HorizCoeffsCols_" + levelIDStr] = std::to_string(hc);
            const size_t sizeOutHorizCoeffs = static_cast<size_t>(hr*hc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, horizCoeffs, sizeOutHorizCoeffs);
            coeffsSize += sizeOutHorizCoeffs;
            info["VertCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["VertCoeffsRows_" + levelIDStr] = std::to_string(vr);
            info["VertCoeffsCols_" + levelIDStr] = std::to_string(vc);
            const size_t sizeOutVertCoeffs = static_cast<size_t>(vr*vc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, vertCoeffs, sizeOutVertCoeffs);
            coeffsSize += sizeOutVertCoeffs;
            info["DetailCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["DetailCoeffsRows_" + levelIDStr] = std::to_string(dr);
            info["DetailCoeffsCols_" + levelIDStr] = std::to_string(dc);
            const size_t sizeOutDetailCoeffs = static_cast<size_t>(dr*dc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, detailCoeffs, sizeOutDetailCoeffs);
            coeffsSize += sizeOutDetailCoeffs;
        }
        else
        {
            horizCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "H", &hr, &hc);
            vertCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "V", &vr, &vc);
            detailCoeffs = getWT2Coeffs(wt, waveletCoeffs, totalLevels-levelID, "D", &dr, &dc);  
            size_t i, j;
            std::cout << "horizontal coefficients: " << std::endl;
	        for (i = 0; i < hr; ++i) {
		        for (j = 0; j < hc; ++j) {

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
	        for (i = 0; i < vr; ++i) {
		        for (j = 0; j < vc; ++j) {

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
	        for (i = 0; i < dr; ++i) {
		        for (j = 0; j < dc; ++j) {

                    if (j < dc-1)
                    {
                        std::cout << detailCoeffs[i*dc + j] << ", ";
                    }
                    else
                    {
                        std::cout << detailCoeffs[i*dc + j] << std::endl;
                    }
		        }
	        }  
            info["HorizCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["HorizCoeffsRows_" + levelIDStr] = std::to_string(hr);
            info["HorizCoeffsCols_" + levelIDStr] = std::to_string(hc);
            const size_t sizeOutHorizCoeffs = static_cast<size_t>(hr*hc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, horizCoeffs, sizeOutHorizCoeffs);
            coeffsSize += sizeOutHorizCoeffs;
            info["VertCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["VertCoeffsRows_" + levelIDStr] = std::to_string(vr);
            info["VertCoeffsCols_" + levelIDStr] = std::to_string(vc);
            const size_t sizeOutVertCoeffs = static_cast<size_t>(vr*vc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, vertCoeffs, sizeOutVertCoeffs);
            coeffsSize += sizeOutVertCoeffs;
            info["DetailCoeffsOffset_" + levelIDStr] = std::to_string(coeffsSize);
            info["DetailCoeffsRows_" + levelIDStr] = std::to_string(dr);
            info["DetailCoeffsCols_" + levelIDStr] = std::to_string(dc);
            const size_t sizeOutDetailCoeffs = static_cast<size_t>(dr*dc*sizeof(double));
            std::memcpy(bufferOut+coeffsSize, detailCoeffs, sizeOutDetailCoeffs);
            coeffsSize += sizeOutDetailCoeffs;      
        }
        

    }
    
    return coeffsSize;
}

size_t CompressWavelet::Decompress(const void *bufferIn, const size_t sizeIn,
                              void *dataOut, const Dims &dimensions,
                              const std::string varType,
                              const Params & /*parameters*/) const
{
    if (dimensions.size() > 2)
    {
        throw std::invalid_argument("ERROR: Wavelet decompression doesn't support "
                                    "more than 2 dimension variables.\n");
    }

    return 0;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
