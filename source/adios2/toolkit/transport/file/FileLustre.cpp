/*
 * FileLustre.cpp
 *
 *  Created on: Nov 20, 2017
 *      Author: lwk
 */
//#ifndef LUSTRE_NEW
//#include <lustre/liblustreapi.h>
//#include <lustre/lustre_user.h>
//#else
extern "C"{
#include <lustre/lustre_user.h>
#include <lustre/lustreapi.h>
}
//#endif

#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close

#include "FileLustre.h"

//extern "C" int llapi_file_create(const char*, unsigned long long, int, int, int);

namespace adios2
{
namespace transport
{

FileLustre::FileLustre(const size_t stripeSize, const size_t stripeOffset,
                       const size_t stripeCount, const size_t stripePattern,
                       MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "Lustre", mpiComm, debugMode), m_StripeSize(stripeSize),
  m_StripeOffset(stripeOffset), m_StripeCount(stripeCount),
  m_StripePattern(stripePattern)
{
}

FileLustre::~FileLustre()
{
    if (m_IsOpen)
    {
        close(m_FileDescriptor);
    }
}


void FileLustre::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    CheckName();
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {

    case (Mode::Write):
    {
        ProfilerStart("open");
        MkDir(m_Name);
        int status;
        const char* nameC = m_Name.c_str();
        unsigned long long stripe_size = static_cast<unsigned long long>(m_StripeSize);
        int stripe_offset = static_cast<int>( m_StripeOffset);
        int stripe_count = static_cast<int>( m_StripeCount);
        int stripe_pattern = static_cast<int>( m_StripePattern);

        status = llapi_file_create(nameC, stripe_size, stripe_offset, stripe_count, stripe_pattern);

        if (status)
        {
            throw std::runtime_error(
                "ERROR: couldn't create file using Lustre API!");
        }
        m_FileDescriptor =
            open(m_Name.c_str(),
                 O_WRONLY | O_CREAT | O_TRUNC | O_LOV_DELAY_CREATE, 0777);
        ProfilerStop("open");
        break;
    }
    case (Mode::Append):
    {
        ProfilerStart("open");
        m_FileDescriptor = open(m_Name.c_str(), O_RDWR);
        ProfilerStop("open");
        break;
    }
    case (Mode::Read):
    {
        ProfilerStart("open");
        m_FileDescriptor = open(m_Name.c_str(), O_RDONLY);
        ProfilerStop("open");
        break;
    }
    default:
    {
        CheckFile("unknown open mode for file " + m_Name +
                  ", in call to Lustre-POSIX open");
    }

    } //end switch

    CheckFile(
        "couldn't open file " + m_Name +
        ", check permissions or path existence, in call to Lustre-POSIX open");

    m_IsOpen = true;
}

void FileLustre::Write(const char *buffer, size_t size, size_t start)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        ProfilerStart("write");
        const auto writtenSize = write(m_FileDescriptor, buffer, size);
        ProfilerStop("write");

        if (writtenSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name +
                                         ", in call to Lustre-POSIX Write\n");
        }

        if (static_cast<size_t>(writtenSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: written size + " + std::to_string(writtenSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to Lustre-POSIX Write\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to Lustre-POSIX lseek\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Write(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Write(&buffer[position], remainder);
    }
    else
    {
        lf_Write(buffer, size);
    }
}

void FileLustre::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {

        ProfilerStart("read");
        const auto readSize = read(m_FileDescriptor, buffer, size);
        ProfilerStop("read");

        if (readSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't read from file " +
                                         m_Name +
                                         ", in call to Lustre-POSIX read\n");
        }

        if (static_cast<size_t>(readSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: read size + " + std::to_string(readSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to Lustre-POSIX read\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to Lustre-POSIX lseek errno " +
                std::to_string(errno) + "\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Read(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Read(&buffer[position], remainder);
    }
    else
    {
        lf_Read(buffer, size);
    }
}

void FileLustre::Close()
{
    ProfilerStart("close");
    const int status = close(m_FileDescriptor);
    ProfilerStop("close");

    if (status == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to Lustre-POSIX close\n");
    }

    m_IsOpen = false;
}

size_t FileLustre::GetSize()
{
    struct stat fileStat;
    if (fstat(m_FileDescriptor, &fileStat) == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't get size of file " +
                                     m_Name + "\n");
    }
    return static_cast<size_t>(fileStat.st_size);
}

void FileLustre::Flush( )
{
}

void FileLustre::CheckFile(const std::string hint) const
{
    if (m_FileDescriptor == -1)
    {
        throw std::ios_base::failure("ERROR: " + hint + "\n");
    }
}

} // end namespace transport
} // end namespace adios2
