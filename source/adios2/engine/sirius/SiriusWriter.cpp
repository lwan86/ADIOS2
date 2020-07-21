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
  m_FileMetadataIndexManager(m_Comm, m_DebugMode),
  m_FileDataLocationManager(m_Comm, m_DebugMode)
{    
    // m_EndMessage = " in call to SiriusWriter " + m_Name + " Open\n";
    // m_WriterRank = m_Comm.Rank();
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
        m_AllBP4Serializers.at(i).Init(m_IO.m_Parameters, "in call to Sirius::Open to write");
        
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
            std::string percentage;
            if (m_IO.m_TransportsParameters[i].count("percentage"))
            {
                percentage = m_IO.m_TransportsParameters[i].at("percentage");
            }
            std::string persistent;
            if (m_IO.m_TransportsParameters[i].count("persistent"))
            {
                persistent = m_IO.m_TransportsParameters[i].at("persistent");
            }
            m_AllLevels[levelID]["location"] = path;
            m_AllLevels[levelID]["percentage"] = percentage;
            m_AllLevels[levelID]["persistent"] = persistent;
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


    std::vector<size_t> levelids;
    levelids.reserve(m_AllLevels.size());
    for (auto const &pair : m_AllLevels)
    {
        levelids.push_back(pair.first);
    }
    std::sort(levelids.begin(), levelids.end());

    std::vector<size_t> persistentLevelIDs;
    std::string leveldata;
    for (auto l : levelids)
    {
        std::cout << "level " << l << ": " << m_AllLevels[l]["location"] << ", " << m_AllLevels[l]["percentage"] << ", " << m_AllLevels[l]["persistent"] << std::endl;
        std::string primarC = m_AllLevels[l]["persistent"];
        std::transform(primarC.begin(), primarC.end(),
                   primarC.begin(), ::tolower);
        if (primarC == "true")
        {
            persistentLevelIDs.push_back(l);
        }
        
        std::string row(std::to_string(l)+","+m_AllLevels[l]["location"]+","+m_AllLevels[l]["percentage"]+","+m_AllLevels[l]["persistent"]+'\n');
        std::cout << "  " << row << std::endl;
        leveldata = leveldata+row;  
    }
    m_LevelIndex.Resize(leveldata.size(), "level index buffer");
    auto &buffer = m_LevelIndex.m_Buffer;
    auto &position = m_LevelIndex.m_Position;
    helper::CopyToBuffer(buffer, position, leveldata.data(), leveldata.size());

    std::string dataLocationFileName;
    for (auto l : persistentLevelIDs)
    {
        if (m_AllLevels[l]["location"].back() != '/')
        {
            dataLocationFileName = m_AllLevels[l]["location"]+PathSeparator+"md.loc";
        } 
        else
        {
            dataLocationFileName = m_AllLevels[l]["location"]+"md.loc";
        }
        
        m_FileDataLocationManager.OpenFileID(dataLocationFileName, l, m_OpenMode, m_IO.m_TransportsParameters[l],
                                            m_AllBP4Serializers.at(l).m_Profiler.m_IsActive);

        m_FileDataLocationManager.WriteFiles(m_LevelIndex.m_Buffer.data(), m_LevelIndex.m_Position, l);
        m_FileDataLocationManager.FlushFiles(l);
        m_FileDataLocationManager.CloseFiles(l);        
    }
    


    //Init();
    InitParameters();
    InitBPBuffer();
    // if (m_Verbosity == 5)
    // {
    //     std::cout << "Sirius Writer " << m_WriterRank << " Open(" << m_Name
    //               << ")." << std::endl;
    // }
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

void SiriusWriter::WriteData(const bool isFinal, const int transportIndex)
{
    size_t dataSize;

    // write data without footer
    if (isFinal)
    {
        dataSize = m_AllBP4Serializers.at(transportIndex).CloseData(m_IO);
    }
    else
    {
        dataSize = m_AllBP4Serializers.at(transportIndex).CloseStream(m_IO, false);
    }

    m_FileDataManager.WriteFiles(m_AllBP4Serializers.at(transportIndex).m_Data.m_Buffer.data(),
                                 dataSize, transportIndex);

    m_FileDataManager.FlushFiles(transportIndex);
}

void SiriusWriter::AggregateWriteData(const bool isFinal, const int transportIndex)
{
    m_AllBP4Serializers.at(transportIndex).CloseStream(m_IO, false);

    // async?
    for (int r = 0; r < m_AllBP4Serializers.at(transportIndex).m_Aggregator.m_Size; ++r)
    {
        aggregator::MPIAggregator::ExchangeRequests dataRequests =
            m_AllBP4Serializers.at(transportIndex).m_Aggregator.IExchange(m_AllBP4Serializers.at(transportIndex).m_Data, r);

        aggregator::MPIAggregator::ExchangeAbsolutePositionRequests
            absolutePositionRequests =
                m_AllBP4Serializers.at(transportIndex).m_Aggregator.IExchangeAbsolutePosition(
                    m_AllBP4Serializers.at(transportIndex).m_Data, r);

        if (m_AllBP4Serializers.at(transportIndex).m_Aggregator.m_IsConsumer)
        {
            const format::Buffer &bufferSTL =
                m_AllBP4Serializers.at(transportIndex).m_Aggregator.GetConsumerBuffer(
                    m_AllBP4Serializers.at(transportIndex).m_Data);
            if (bufferSTL.m_Position > 0)
            {
                m_FileDataManager.WriteFiles(
                    bufferSTL.Data(), bufferSTL.m_Position, transportIndex);

                m_FileDataManager.FlushFiles(transportIndex);
            }
        }

        m_AllBP4Serializers.at(transportIndex).m_Aggregator.WaitAbsolutePosition(
            absolutePositionRequests, r);

        m_AllBP4Serializers.at(transportIndex).m_Aggregator.Wait(dataRequests, r);
        m_AllBP4Serializers.at(transportIndex).m_Aggregator.SwapBuffers(r);
    }

    m_AllBP4Serializers.at(transportIndex).UpdateOffsetsInMetadata();

    if (isFinal) // Write metadata footer
    {
        m_AllBP4Serializers.at(transportIndex).m_Aggregator.Close();
    }

    m_AllBP4Serializers.at(transportIndex).m_Aggregator.ResetBuffers();
}

void SiriusWriter::DoFlush(const bool isFinal, const int transportIndex)
{
    if (m_AllBP4Serializers.at(transportIndex).m_Aggregator.m_IsActive)
    {
        AggregateWriteData(isFinal, transportIndex);
    }
    else
    {
        WriteData(isFinal, transportIndex);
    }
}

void SiriusWriter::Flush(const int transportIndex)
{
    DoFlush(false, transportIndex);
    m_AllBP4Serializers.at(transportIndex).ResetBuffer(m_AllBP4Serializers.at(transportIndex).m_Data);

    if (m_AllBP4Serializers.at(transportIndex).m_Parameters.CollectiveMetadata)
    {
        WriteCollectiveMetadataFile(false, transportIndex);
    }
}

void SiriusWriter::UpdateActiveFlag(const bool active, const int transportIndex)
{
    const char activeChar = (active ? '\1' : '\0');
    m_FileMetadataIndexManager.WriteFileAt(
        &activeChar, 1, m_AllBP4Serializers.at(transportIndex).m_ActiveFlagPosition, transportIndex);
    m_FileMetadataIndexManager.FlushFiles(transportIndex);
    m_FileMetadataIndexManager.SeekToFileEnd(transportIndex);
}

void SiriusWriter::PopulateMetadataIndexFileContent(
    format::BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp)
{
    auto &buffer = b.m_Buffer;
    auto &position = b.m_Position;
    helper::CopyToBuffer(buffer, position, &currentStep);
    helper::CopyToBuffer(buffer, position, &mpirank);
    helper::CopyToBuffer(buffer, position, &pgIndexStart);
    helper::CopyToBuffer(buffer, position, &variablesIndexStart);
    helper::CopyToBuffer(buffer, position, &attributesIndexStart);
    helper::CopyToBuffer(buffer, position, &currentStepEndPos);
    helper::CopyToBuffer(buffer, position, &currentTimeStamp);
    position += 8;
}

void SiriusWriter::WriteCollectiveMetadataFile(const bool isFinal, const int transportIndex)
{

    if (isFinal && m_AllBP4Serializers.at(transportIndex).m_MetadataSet.DataPGCount == 0)
    {
        // If data pg count is zero, it means all metadata
        // has already been written, don't need to write it again.

        if (m_AllBP4Serializers.at(transportIndex).m_RankMPI == 0)
        {
            // But the flag in the header of metadata index table needs to
            // be modified to indicate current run is over.
            UpdateActiveFlag(false, transportIndex);
        }
        return;
    }
    m_AllBP4Serializers.at(transportIndex).AggregateCollectiveMetadata(
        m_Comm, m_AllBP4Serializers.at(transportIndex).m_Metadata, true);

    if (m_AllBP4Serializers.at(transportIndex).m_RankMPI == 0)
    {

        m_FileMetadataManager.WriteFiles(
            m_AllBP4Serializers.at(transportIndex).m_Metadata.m_Buffer.data(),
            m_AllBP4Serializers.at(transportIndex).m_Metadata.m_Position, transportIndex);
        m_FileMetadataManager.FlushFiles(transportIndex);

        std::time_t currentTimeStamp = std::time(nullptr);

        std::vector<size_t> timeSteps;
        timeSteps.reserve(
            m_AllBP4Serializers.at(transportIndex).m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI]
                .size());
        for (auto const &pair :
             m_AllBP4Serializers.at(transportIndex).m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI])
        {
            timeSteps.push_back(pair.first);
        }
        std::sort(timeSteps.begin(), timeSteps.end());

        size_t rowsInMetadataIndexTable = timeSteps.size() + 1;
        m_AllBP4Serializers.at(transportIndex).m_MetadataIndex.Resize(rowsInMetadataIndexTable * 64,
                                               "BP4 Index Table");
        for (auto const &t : timeSteps)
        {
            /*if (t == 1)
            {
                m_BP4Serializer.MakeHeader(m_BP4Serializer.m_MetadataIndex,
                                           "Index Table", true);
            }*/
            const uint64_t pgIndexStartMetadataFile =
                m_AllBP4Serializers.at(transportIndex)
                    .m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI][t][0] +
                m_AllBP4Serializers.at(transportIndex).m_MetadataSet.MetadataFileLength +
                m_AllBP4Serializers.at(transportIndex).m_PreMetadataFileLength;
            const uint64_t varIndexStartMetadataFile =
                m_AllBP4Serializers.at(transportIndex)
                    .m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI][t][1] +
                m_AllBP4Serializers.at(transportIndex).m_MetadataSet.MetadataFileLength +
                m_AllBP4Serializers.at(transportIndex).m_PreMetadataFileLength;
            const uint64_t attrIndexStartMetadataFile =
                m_AllBP4Serializers.at(transportIndex)
                    .m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI][t][2] +
                m_AllBP4Serializers.at(transportIndex).m_MetadataSet.MetadataFileLength +
                m_AllBP4Serializers.at(transportIndex).m_PreMetadataFileLength;
            const uint64_t currentStepEndPosMetadataFile =
                m_AllBP4Serializers.at(transportIndex)
                    .m_MetadataIndexTable[m_AllBP4Serializers.at(transportIndex).m_RankMPI][t][3] +
                m_AllBP4Serializers.at(transportIndex).m_MetadataSet.MetadataFileLength +
                m_AllBP4Serializers.at(transportIndex).m_PreMetadataFileLength;
            PopulateMetadataIndexFileContent(
                m_AllBP4Serializers.at(transportIndex).m_MetadataIndex, t, m_AllBP4Serializers.at(transportIndex).m_RankMPI,
                pgIndexStartMetadataFile, varIndexStartMetadataFile,
                attrIndexStartMetadataFile, currentStepEndPosMetadataFile,
                currentTimeStamp);
        }

        m_FileMetadataIndexManager.WriteFiles(
            m_AllBP4Serializers.at(transportIndex).m_MetadataIndex.m_Buffer.data(),
            m_AllBP4Serializers.at(transportIndex).m_MetadataIndex.m_Position);
        m_FileMetadataIndexManager.FlushFiles(transportIndex);

        m_AllBP4Serializers.at(transportIndex).m_MetadataSet.MetadataFileLength +=
            m_AllBP4Serializers.at(transportIndex).m_Metadata.m_Position;

        if (isFinal)
        {
            // Only one step of metadata is generated at close.
            // The flag in the header of metadata index table
            // needs to be modified to indicate current run is over.
            UpdateActiveFlag(false, transportIndex);
        }
    }
    /*Clear the local indices buffer at the end of each step*/
    m_AllBP4Serializers.at(transportIndex).ResetBuffer(m_AllBP4Serializers.at(transportIndex).m_Metadata, true);

    /* clear the metadata index buffer*/
    m_AllBP4Serializers.at(transportIndex).ResetBuffer(m_AllBP4Serializers.at(transportIndex).m_MetadataIndex, true);

    /* reset the metadata index table*/
    m_AllBP4Serializers.at(transportIndex).ResetMetadataIndexTable();
    m_AllBP4Serializers.at(transportIndex).ResetAllIndices();
}

void SiriusWriter::DoClose(const int transportIndex)
{

    DoFlush(true, transportIndex);

    if (m_AllBP4Serializers.at(transportIndex).m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.CloseFiles(transportIndex);
    }

    if (m_AllBP4Serializers.at(transportIndex).m_Parameters.CollectiveMetadata &&
        m_FileDataManager.TransportClosed(transportIndex))
    {
        WriteCollectiveMetadataFile(true, transportIndex);
    }

    if (m_AllBP4Serializers.at(transportIndex).m_Aggregator.m_IsActive)
    {
        m_AllBP4Serializers.at(transportIndex).m_Aggregator.Close();
    }

    if (m_AllBP4Serializers.at(transportIndex).m_RankMPI == 0)
    {
        // close metadata file
        m_FileMetadataManager.CloseFiles(transportIndex);

        // close metadata index file
        m_FileMetadataIndexManager.CloseFiles(transportIndex);
    }
}

StepStatus SiriusWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    //m_CurrentStep++; // 0 is the first step
    // if (m_Verbosity == 5)
    // {
    //     std::cout << "Sirius Writer " << m_WriterRank
    //               << "   BeginStep() new step " << m_CurrentStep << "\n";
    // }
    m_IO.m_ReadStreaming = false;
    return StepStatus::OK;
}


/* PutDeferred = PutSync, so nothing to be done in PerformPuts */
void SiriusWriter::PerformPuts()
{
    if (m_Verbosity == 5)
    {
        std::cout << "Sirius Writer PerformPuts()\n";
    }
}

size_t SiriusWriter::CurrentStep(const int transportIndex)
{
    std::cout << "transportIndex: " << transportIndex << ", current step: " << m_AllBP4Serializers.at(transportIndex).m_MetadataSet.CurrentStep << std::endl;
    return m_AllBP4Serializers.at(transportIndex).m_MetadataSet.CurrentStep;
}

void SiriusWriter::EndStep()
{
    for (size_t i = 0; i < m_IO.m_TransportsParameters.size(); i++)
    {
        // true: advances step
        m_AllBP4Serializers.at(i).SerializeData(m_IO, true);

        const size_t currentStep = CurrentStep(i);
        const size_t flushStepsCount = m_AllBP4Serializers.at(i).m_Parameters.FlushStepsCount;

        if (currentStep % flushStepsCount == 0)
        {
            Flush(i);
        }
    }
    // if (m_Verbosity == 5)
    // {
    //     std::cout << "Sirius Writer " << m_WriterRank << "   EndStep()\n";
    // }
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

// void SiriusWriter::Init()
// {
//     InitParameters();
//     InitTransports();
// }

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
        else if (key == "storage tiers")
        {
            m_StorageTiers = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_StorageTiers < 1)
                    throw std::invalid_argument(
                        "ERROR: Method storage tiers argument must be an "
                        "integer in the range [1,inf], in call to "
                        "Open or Engine constructor\n");
            }
        }
        else if (key == "wavelet name")
        {
            m_WaveletName = value;
        }        
        else if (key == "wavelet levels")
        {
            m_WaveletLevels = std::stoi(value);
            if (m_DebugMode)
            {
                if (m_WaveletLevels < 1)
                    throw std::invalid_argument(
                        "ERROR: Method wavelet levels argument must be an "
                        "integer in the range [1,inf], in call to "
                        "Open or Engine constructor\n");
            }
        }        
        
    }

    
}


} // end namespace engine
} // end namespace core
} // end namespace adios2