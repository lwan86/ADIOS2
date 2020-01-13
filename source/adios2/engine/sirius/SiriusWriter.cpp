/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SiriusWriter.cpp
 * Sirius engine from which any engine can be built.
 *
 *  Created on: Dec 04, 2019
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "SiriusWriter.h"
#include "SiriusWriter.tcc"

#include "adios2/helper/adiosFunctions.h"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

SiriusWriter::SiriusWriter(IO &io, const std::string &name, const Mode mode,
                               helper::Comm comm)
: Engine("SiriusWriter", io, name, mode, std::move(comm)), 
  m_FileDataManager(m_Comm, m_DebugMode),
  m_FileMetadataManager(m_Comm, m_DebugMode),
  m_FileMetadataIndexManager(m_Comm, m_DebugMode)
{    
    m_EndMessage = " in call to SiriusWriter " + m_Name + " Open\n";
    m_WriterRank = m_Comm.Rank();
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    for (size_t i = 0; i < m_IO.m_TransportsParameters.size(); i++)
    {
        //std::cout << m_IO.m_TransportsParameters[i]["transport"] << "," << m_IO.m_TransportsParameters[i]["location"] << std::endl;
        m_AllBP4Serializers.emplace(std::piecewise_construct, 
             std::forward_as_tuple(i),
             std::forward_as_tuple(m_Comm, m_DebugMode));
        m_AllBP4Serializers.at(i).Init(m_IO.m_Parameters, "in call to BP4::Open to write");
        
        std::string path(m_Name);

        if (m_IO.m_TransportsParameters[i].count("level id"))
        {
            std::string levelIDStr(m_IO.m_TransportsParameters[i].at("level id"));
            int levelID = std::stoi(levelIDStr);
            std::cout << "level id: " << levelID << std::endl;
            size_t found = path.find(".");
            if (found!=std::string::npos)
            {
                std::string token = path.substr(0, found);
                path = token+"-l"+std::to_string(levelID)+path.substr(found, path.length());
            }
            else
            {
                path = path+"-l"+std::to_string(levelID);
            }
            
            if (m_IO.m_TransportsParameters[i].count("location"))
            {
                const std::string location(m_IO.m_TransportsParameters[i].at("location"));
                if (location.back() != '/')
                {
                    path = location + PathSeparator + path;
                }
                else
                {
                    path = location + path;
                }
            }
        }
        std::string bpSubFileName;
        if (m_AllBP4Serializers.at(i).m_Aggregator.m_IsConsumer)
        {
            bpSubFileName = m_AllBP4Serializers.at(i).GetBPSubFileName(path, m_AllBP4Serializers.at(i).m_RankMPI);
        }
        m_AllBP4Serializers.at(i).m_Profiler.Start("mkdir");
        m_FileDataManager.MkDirsBarrier({bpSubFileName},
                        m_AllBP4Serializers.at(i).m_Parameters.NodeLocal);
        m_AllBP4Serializers.at(i).m_Profiler.Stop("mkdir");

        if (m_AllBP4Serializers.at(i).m_Aggregator.m_IsConsumer)
        {
            m_FileDataManager.OpenFileID(bpSubFileName, i, m_OpenMode, m_IO.m_TransportsParameters[i],
                                        m_AllBP4Serializers.at(i).m_Profiler.m_IsActive);
        }

        if (m_AllBP4Serializers.at(i).m_RankMPI == 0)
        {
            std::string bpMetadataFileName = m_AllBP4Serializers.at(i).GetBPMetadataFileName(path);
            m_FileMetadataManager.OpenFileID(bpMetadataFileName, i, m_OpenMode, m_IO.m_TransportsParameters[i],
                                        m_AllBP4Serializers.at(i).m_Profiler.m_IsActive);
            std::string metadataIndexFileName = m_AllBP4Serializers.at(i).GetBPMetadataIndexFileName(path);
            m_FileMetadataIndexManager.OpenFileID(metadataIndexFileName, i, m_OpenMode, m_IO.m_TransportsParameters[i],
                                        m_AllBP4Serializers.at(i).m_Profiler.m_IsActive);

            if (m_OpenMode != Mode::Append ||
                m_FileMetadataIndexManager.GetFileSize(i) == 0)
            {
                /* Prepare header and write now to Index Table indicating
                * the start of streaming */
                m_AllBP4Serializers.at(i).MakeHeader(m_AllBP4Serializers.at(i).m_MetadataIndex,
                                           "Index Table", true);

                m_FileMetadataIndexManager.WriteFiles(
                    m_AllBP4Serializers.at(i).m_MetadataIndex.m_Buffer.data(),
                    m_AllBP4Serializers.at(i).m_MetadataIndex.m_Position, i);
                m_FileMetadataIndexManager.FlushFiles(i);
                /* clear the metadata index buffer*/
                m_AllBP4Serializers.at(i).ResetBuffer(m_AllBP4Serializers.at(i).m_MetadataIndex, true);
            }
            else
            {
                /* Update header to indicate re-start of streaming */
                const char activeChar = (true ? '\1' : '\0');
                m_FileMetadataIndexManager.WriteFileAt(
                    &activeChar, 1, m_AllBP4Serializers.at(i).m_ActiveFlagPosition, i);
                m_FileMetadataIndexManager.FlushFiles(i);
                m_FileMetadataIndexManager.SeekToFileEnd(i);
            }
        }
                
    }  

    //Init();
    InitParameters();
    InitBPBuffer();
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank << " Open(" << m_Name
                  << ")." << std::endl;
    }
}

void SiriusWriter::InitBPBuffer()
{
    for (size_t i = 0; i < m_IO.m_TransportsParameters.size(); i++)
    {
        if (m_OpenMode == Mode::Append)
        {
            // throw std::invalid_argument(
            //    "ADIOS2: OpenMode Append hasn't been implemented, yet");
            // TODO: Get last pg timestep and update timestep counter in
            format::BufferSTL preMetadataIndex;
            size_t preMetadataIndexFileSize;

            if (m_AllBP4Serializers.at(i).m_RankMPI == 0)
            {
                preMetadataIndexFileSize =
                    m_FileMetadataIndexManager.GetFileSize(i);
                preMetadataIndex.m_Buffer.resize(preMetadataIndexFileSize);
                preMetadataIndex.m_Buffer.assign(preMetadataIndex.m_Buffer.size(),
                                                '\0');
                preMetadataIndex.m_Position = 0;
                m_FileMetadataIndexManager.ReadFile(
                    preMetadataIndex.m_Buffer.data(), preMetadataIndexFileSize, i);
            }
            m_Comm.BroadcastVector(preMetadataIndex.m_Buffer);
            preMetadataIndexFileSize = preMetadataIndex.m_Buffer.size();
            if (preMetadataIndexFileSize > 0)
            {
                size_t position = 0;
                position += 28;
                const uint8_t endianness =
                    helper::ReadValue<uint8_t>(preMetadataIndex.m_Buffer, position);
                bool IsLittleEndian = true;
                IsLittleEndian = (endianness == 0) ? true : false;
                if (helper::IsLittleEndian() != IsLittleEndian)
                {
                    throw std::runtime_error(
                        "ERROR: previous run generated BigEndian bp file, "
                        "this version of ADIOS2 wasn't compiled "
                        "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                        "explicitly, in call to Open\n");
                }
                const size_t pos_last_step = preMetadataIndexFileSize - 64;
                position = pos_last_step;
                const uint64_t lastStep = helper::ReadValue<uint64_t>(
                    preMetadataIndex.m_Buffer, position, IsLittleEndian);
                m_AllBP4Serializers.at(i).m_MetadataSet.TimeStep +=
                    static_cast<uint32_t>(lastStep);
                m_AllBP4Serializers.at(i).m_MetadataSet.CurrentStep += lastStep;

                if (m_AllBP4Serializers.at(i).m_Aggregator.m_IsConsumer)
                {

                    m_AllBP4Serializers.at(i).m_PreDataFileLength =
                        m_FileDataManager.GetFileSize(i);
                }

                if (m_AllBP4Serializers.at(i).m_RankMPI == 0)
                {
                    // Set the flag in the header of metadata index table to 0 again
                    // to indicate a new run begins
                    const char activeChar = (true ? '\1' : '\0');
                    m_FileMetadataIndexManager.WriteFileAt(
                        &activeChar, 1, m_AllBP4Serializers.at(i).m_ActiveFlagPosition, i);
                    m_FileMetadataIndexManager.FlushFiles(i);
                    m_FileMetadataIndexManager.SeekToFileEnd(i);

                    // Get the size of existing metadata file
                    m_AllBP4Serializers.at(i).m_PreMetadataFileLength =
                        m_FileMetadataManager.GetFileSize(i);
                }
            }
        }

        if (m_AllBP4Serializers.at(i).m_PreDataFileLength == 0)
        {
            /* This is a new file.
            * Make headers in data buffer and metadata buffer
            */
            if (m_AllBP4Serializers.at(i).m_RankMPI == 0)
            {
                m_AllBP4Serializers.at(i).MakeHeader(m_AllBP4Serializers.at(i).m_Metadata, "Metadata",
                                        false);
            }
            if (m_AllBP4Serializers.at(i).m_Aggregator.m_IsConsumer)
            {
                m_AllBP4Serializers.at(i).MakeHeader(m_AllBP4Serializers.at(i).m_Data, "Data", false);
            }
        }

        m_AllBP4Serializers.at(i).PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            {m_FileDataManager.GetTransportType(i)});
    }
}


StepStatus SiriusWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_CurrentStep++; // 0 is the first step
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank
                  << "   BeginStep() new step " << m_CurrentStep << "\n";
    }
    return StepStatus::OK;
}

size_t SiriusWriter::CurrentStep() const
{
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank
                  << "   CurrentStep() returns " << m_CurrentStep << "\n";
    }
    return m_CurrentStep;
}

/* PutDeferred = PutSync, so nothing to be done in PerformPuts */
void SiriusWriter::PerformPuts()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank
                  << "     PerformPuts()\n";
    }
    m_NeedPerformPuts = false;
}

void SiriusWriter::EndStep()
{
    if (m_NeedPerformPuts)
    {
        PerformPuts();
    }
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank << "   EndStep()\n";
    }
}
void SiriusWriter::Flush(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank << "   Flush()\n";
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void SiriusWriter::DoPutSync(Variable<T> &variable, const T *data)       \
    {                                                                          \
        PutSyncCommon(variable, data);   \
    }                                                                          \
    void SiriusWriter::DoPutDeferred(Variable<T> &variable, const T *data)   \
    {                                                                          \
        PutSyncCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SiriusWriter::Init()
{
    InitParameters();
    InitTransports();
}

void SiriusWriter::InitParameters()
{
    for (const auto &pair : m_IO.m_Parameters)
    {
        std::string key(pair.first);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        std::string value(pair.second);
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);

        if (key == "verbose")
        {
            m_Verbosity = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_Verbosity < 0 || m_Verbosity > 5)
                    throw std::invalid_argument(
                        "ERROR: Method verbose argument must be an "
                        "integer in the range [0,5], in call to "
                        "Open or Engine constructor\n");
            }
        }
        else if (key == "levels")
        {
            m_Levels = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_Levels < 1)
                    throw std::invalid_argument(
                        "ERROR: Method levels argument must be an "
                        "integer in the range [1,inf], in call to "
                        "Open or Engine constructor\n");
            }
        }
        
        
    }

    
}

void SiriusWriter::InitTransports()
{
    // Nothing to process from m_IO.m_TransportsParameters
}

void SiriusWriter::DoClose(const int transportIndex)
{
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer " << m_WriterRank << " Close(" << m_Name
                  << ")\n";
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
