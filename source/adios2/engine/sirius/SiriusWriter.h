/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SiriusWriter.h
 * Sirius engine from which any engine can be built.
 *
 *  Created on: Dec 04, 2019
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_SIRIUSWRITER_H_
#define ADIOS2_ENGINE_SIRIUSWRITER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/format/bp/bp4/BP4Serializer.h"
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class SiriusWriter : public Engine
{

public:
    /**
     * Constructor for Writer
     * @param name unique name given to the engine
     * @param accessMode
     * @param comm
     * @param method
     * @param debugMode
     */
    SiriusWriter(IO &adios, const std::string &name, const Mode mode,
                   helper::Comm comm);

    ~SiriusWriter() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep(const int transportIndex);
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex) final;

private:

    /**  controlling BP buffering for each tier */
    //std::unordered_map<std::string, format::BP4Serializer> m_TransportID2BP4Serializer;
    std::unordered_map<size_t, format::BP4Serializer> m_AllBP4Serializers;

    std::unordered_map<size_t, std::unordered_map<std::string, std::string>> m_AllLevels;

    adios2::format::BufferSTL m_LevelIndex;

    /** Manage BP data files Transports from IO AddTransport */
    transportman::TransportMan m_FileDataManager;

    /** Manages the optional collective metadata files */
    transportman::TransportMan m_FileMetadataManager;

    /* transport manager for managing the metadata index file */
    transportman::TransportMan m_FileMetadataIndexManager;

    transportman::TransportMan m_FileDataLocationManager;

    /** Allocates memory and starts a PG group */
    void InitBPBuffer();


    int m_Verbosity = 0;
    // int m_WriterRank;       // my rank in the writers' comm
    // int m_CurrentStep = -1; // steps start from 0

    int m_StorageTiers = 1;

    std::string m_WaveletName;

    int m_WaveletLevels = 1;

    // EndStep must call PerformPuts if necessary
    bool m_NeedPerformPuts = false;

    //void Init() final;
    void InitParameters() final;
    //void InitTransports() final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it closes a transport in m_Transport[transportIndex].
     * In debug mode the latter is bounds-checked.
     */
    void DoClose(const int transportIndex) final;

    void WriteData(const bool isFinal, const int transportIndex);

    void AggregateWriteData(const bool isFinal, const int transportIndex);

    void DoFlush(const bool isFinal, const int transportIndex);

    void UpdateActiveFlag(const bool active, const int transportIndex);

    void WriteCollectiveMetadataFile(const bool isFinal, const int transportIndex);

    void PopulateMetadataIndexFileContent(
    format::BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp);

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *data);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_SIRIUSWRITER_H_ */