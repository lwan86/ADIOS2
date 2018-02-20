/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileLustre.h wrapper of Lustre API functions for file I/O
 *
 *  Created on: Nov 20, 2017
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_TRANSPORT_FILE_FILELUSTRE_H_
#define ADIOS2_TOOLKIT_TRANSPORT_FILE_FILELUSTRE_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/toolkit/transport/Transport.h"

namespace adios2
{
namespace transport
{

class FileLustre : public Transport
{
public:
    FileLustre(const size_t stripeSize, const size_t stripeOffset,
               const size_t stripeCount, const size_t stripePattern,
               MPI_Comm mpiComm, const bool debugMode);

    ~FileLustre();

    void Open(const std::string &name, const Mode openMode) final;

    void Write(const char *buffer, size_t size, size_t start = MaxSizeT) final;

    void Read(char *buffer, size_t size, size_t start = MaxSizeT) final;

    size_t GetSize() final;
    
    virtual void Flush() final;

    void Close() final;

private:
    const size_t m_StripeSize;
    const size_t m_StripeOffset;
    const size_t m_StripeCount;
    const size_t m_StripePattern;
    int m_FileDescriptor = -1;

    void CheckFile(const std::string hint) const;
};
}
}

#endif /* ADIOS2_TOOLKIT_TRANSPORT_FILE_FILELUSTRE_H_ */
