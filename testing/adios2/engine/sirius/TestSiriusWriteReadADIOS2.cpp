/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 *  Created on: Dec 04 2019
 *      Author: Lipeng Wan
 */

#include <cstdint>
#include <cstring>

#include <iostream>
#include <numeric> //std::iota
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class SiriusWriteReadTestADIOS2 : public ::testing::Test
{
public:
    SiriusWriteReadTestADIOS2() = default;

    SmallTestData m_TestData;
};

//******************************************************************************
// 2D 2x4 test data
//******************************************************************************

// ADIOS2 Sirius write, native ADIOS1 read
TEST_F(SiriusWriteReadTestADIOS2, ADIOS2SiriusWriteRead2D)
{
    // Each process would write a 2x4 array and all processes would
    // form a 2D 2 * (numberOfProcess*Nx) matrix where Nx is 4 here
    const std::string fname("ADIOS2SiriusWriteRead2DTest.bp");

    int mpiRank = 0, mpiSize = 1;
    // Number of rows
    const std::size_t Nx = 8;

    // Number of rows
    const std::size_t Ny = 8;

    // Number of steps
    const std::size_t NSteps = 3;

#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    // Write test data using ADIOS2

#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");
        io.SetEngine("Sirius");
        io.SetParameter("Levels", "2");
        io.AddTransport("File", {{"level id", "0"}, {"location", "/Users/lwk/Research/Projects/adios2/dev/pfs"}});
        io.AddTransport("File", {{"level id", "1"}, {"location", "/Users/lwk/Research/Projects/adios2/dev/bb"}});

        // Declare 2D variables (Ny * (NumOfProcesses * Nx))
        // The local process' part (start, count) can be defined now or later
        // before Write().
        {
            const adios2::Dims shape{Ny, static_cast<size_t>(Nx * mpiSize)};
            const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
            const adios2::Dims count{Ny, Nx};

            // auto var_iString = io.DefineVariable<std::string>("iString");
            // auto var_i8 = io.DefineVariable<int8_t>("i8", shape, start, count);
            // auto var_i16 =
            //     io.DefineVariable<int16_t>("i16", shape, start, count);
            // auto var_i32 =
            //     io.DefineVariable<int32_t>("i32", shape, start, count);
            // auto var_i64 =
            //     io.DefineVariable<int64_t>("i64", shape, start, count);
            // auto var_u8 = io.DefineVariable<uint8_t>("u8", shape, start, count);
            // auto var_u16 =
            //     io.DefineVariable<uint16_t>("u16", shape, start, count);
            // auto var_u32 =
            //     io.DefineVariable<uint32_t>("u32", shape, start, count);
            // auto var_u64 =
            //     io.DefineVariable<uint64_t>("u64", shape, start, count);
            // auto var_r32 = io.DefineVariable<float>("r32", shape, start, count);
            auto var_r64 =
                io.DefineVariable<double>("r64", shape, start, count);
        }

        adios2::Engine SiriusWriter = io.Open(fname, adios2::Mode::Write);

        for (size_t step = 0; step < NSteps; ++step)
        {
            // Generate test data for each process uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(step), mpiRank, mpiSize);

            // Retrieve the variables that previously went out of scope
            // auto var_iString = io.InquireVariable<std::string>("iString");
            // auto var_i8 = io.InquireVariable<int8_t>("i8");
            // auto var_i16 = io.InquireVariable<int16_t>("i16");
            // auto var_i32 = io.InquireVariable<int32_t>("i32");
            // auto var_i64 = io.InquireVariable<int64_t>("i64");
            // auto var_u8 = io.InquireVariable<uint8_t>("u8");
            // auto var_u16 = io.InquireVariable<uint16_t>("u16");
            // auto var_u32 = io.InquireVariable<uint32_t>("u32");
            // auto var_u64 = io.InquireVariable<uint64_t>("u64");
            // auto var_r32 = io.InquireVariable<float>("r32");
            auto var_r64 = io.InquireVariable<double>("r64");

            // Make a 2D selection to describe the local dimensions of the
            // variable we write and its offsets in the global spaces
            adios2::Box<adios2::Dims> sel(
                {0, static_cast<size_t>(mpiRank * Nx)}, {Ny, Nx});
            // var_i8.SetSelection(sel);
            // var_i16.SetSelection(sel);
            // var_i32.SetSelection(sel);
            // var_i64.SetSelection(sel);
            // var_u8.SetSelection(sel);
            // var_u16.SetSelection(sel);
            // var_u32.SetSelection(sel);
            // var_u64.SetSelection(sel);
            // var_r32.SetSelection(sel);
            var_r64.SetSelection(sel);

            // Write each one
            // fill in the variable with values from starting index to
            // starting index + count
            SiriusWriter.BeginStep();
            // SiriusWriter.Put(var_iString, currentTestData.S1);
            // SiriusWriter.Put(var_i8, currentTestData.I8.data());
            // SiriusWriter.Put(var_i16, currentTestData.I16.data());
            // SiriusWriter.Put(var_i32, currentTestData.I32.data());
            // SiriusWriter.Put(var_i64, currentTestData.I64.data());
            // SiriusWriter.Put(var_u8, currentTestData.U8.data());
            // SiriusWriter.Put(var_u16, currentTestData.U16.data());
            // SiriusWriter.Put(var_u32, currentTestData.U32.data());
            // SiriusWriter.Put(var_u64, currentTestData.U64.data());
            // SiriusWriter.Put(var_r32, currentTestData.R32.data());
            std::cout << "original data: " << std::endl;
            std::cout << "[";        
            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::cout << currentTestData.R64[i] << ", ";
            }
            std::cout << "]" << std::endl;
            
            SiriusWriter.Put(var_r64, currentTestData.R64.data());
            SiriusWriter.PerformPuts();

            SiriusWriter.EndStep();
        }

        // Close the file
        SiriusWriter.Close();
    }

    {
        adios2::IO io = adios.DeclareIO("ReadIO");

        io.SetEngine("Sirius");

        adios2::Engine SiriusReader = io.Open(fname, adios2::Mode::Read);

        // EXPECT_EQ(SiriusReader.Steps(), NSteps);
        // auto var_iString = io.InquireVariable<std::string>("iString");
        // EXPECT_TRUE(var_iString);
        // ASSERT_EQ(var_iString.Shape().size(), 0);
        // ASSERT_EQ(var_iString.Steps(), NSteps);

        // auto var_i8 = io.InquireVariable<int8_t>("i8");
        // EXPECT_TRUE(var_i8);
        // ASSERT_EQ(var_i8.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_i8.Steps(), NSteps);
        // ASSERT_EQ(var_i8.Shape()[0], Ny);
        // ASSERT_EQ(var_i8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_i16 = io.InquireVariable<int16_t>("i16");
        // EXPECT_TRUE(var_i16);
        // ASSERT_EQ(var_i16.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_i16.Steps(), NSteps);
        // ASSERT_EQ(var_i16.Shape()[0], Ny);
        // ASSERT_EQ(var_i16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_i32 = io.InquireVariable<int32_t>("i32");
        // EXPECT_TRUE(var_i32);
        // ASSERT_EQ(var_i32.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_i32.Steps(), NSteps);
        // ASSERT_EQ(var_i32.Shape()[0], Ny);
        // ASSERT_EQ(var_i32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_i64 = io.InquireVariable<int64_t>("i64");
        // EXPECT_TRUE(var_i64);
        // ASSERT_EQ(var_i64.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_i64.Steps(), NSteps);
        // ASSERT_EQ(var_i64.Shape()[0], Ny);
        // ASSERT_EQ(var_i64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_u8 = io.InquireVariable<uint8_t>("u8");
        // EXPECT_TRUE(var_u8);
        // ASSERT_EQ(var_u8.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_u8.Steps(), NSteps);
        // ASSERT_EQ(var_u8.Shape()[0], Ny);
        // ASSERT_EQ(var_u8.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_u16 = io.InquireVariable<uint16_t>("u16");
        // EXPECT_TRUE(var_u16);
        // ASSERT_EQ(var_u16.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_u16.Steps(), NSteps);
        // ASSERT_EQ(var_u16.Shape()[0], Ny);
        // ASSERT_EQ(var_u16.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_u32 = io.InquireVariable<uint32_t>("u32");
        // EXPECT_TRUE(var_u32);
        // ASSERT_EQ(var_u32.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_u32.Steps(), NSteps);
        // ASSERT_EQ(var_u32.Shape()[0], Ny);
        // ASSERT_EQ(var_u32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_u64 = io.InquireVariable<uint64_t>("u64");
        // EXPECT_TRUE(var_u64);
        // ASSERT_EQ(var_u64.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_u64.Steps(), NSteps);
        // ASSERT_EQ(var_u64.Shape()[0], Ny);
        // ASSERT_EQ(var_u64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // auto var_r32 = io.InquireVariable<float>("r32");
        // EXPECT_TRUE(var_r32);
        // ASSERT_EQ(var_r32.ShapeID(), adios2::ShapeID::GlobalArray);
        // ASSERT_EQ(var_r32.Steps(), NSteps);
        // ASSERT_EQ(var_r32.Shape()[0], Ny);
        // ASSERT_EQ(var_r32.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        auto var_r64 = io.InquireVariable<double>("r64");
        EXPECT_TRUE(var_r64);
        ASSERT_EQ(var_r64.ShapeID(), adios2::ShapeID::GlobalArray);
        ASSERT_EQ(var_r64.Steps(), NSteps);
        ASSERT_EQ(var_r64.Shape()[0], Ny);
        ASSERT_EQ(var_r64.Shape()[1], static_cast<size_t>(mpiSize * Nx));

        // std::string IString;
        // std::array<int8_t, Nx * Ny> I8;
        // std::array<int16_t, Nx * Ny> I16;
        // std::array<int32_t, Nx * Ny> I32;
        // std::array<int64_t, Nx * Ny> I64;
        // std::array<uint8_t, Nx * Ny> U8;
        // std::array<uint16_t, Nx * Ny> U16;
        // std::array<uint32_t, Nx * Ny> U32;
        // std::array<uint64_t, Nx * Ny> U64;
        // std::array<float, Nx * Ny> R32;
        std::array<double, Nx * Ny> R64;

        const adios2::Dims start{0, static_cast<size_t>(mpiRank * Nx)};
        const adios2::Dims count{Ny, Nx};

        const adios2::Box<adios2::Dims> sel(start, count);

        // var_i8.SetSelection(sel);
        // var_i16.SetSelection(sel);
        // var_i32.SetSelection(sel);
        // var_i64.SetSelection(sel);

        // var_u8.SetSelection(sel);
        // var_u16.SetSelection(sel);
        // var_u32.SetSelection(sel);
        // var_u64.SetSelection(sel);

        // var_r32.SetSelection(sel);
        var_r64.SetSelection(sel);

        for (size_t t = 0; t < NSteps; ++t)
        {
            // var_i8.SetStepSelection({t, 1});
            // var_i16.SetStepSelection({t, 1});
            // var_i32.SetStepSelection({t, 1});
            // var_i64.SetStepSelection({t, 1});

            // var_u8.SetStepSelection({t, 1});
            // var_u16.SetStepSelection({t, 1});
            // var_u32.SetStepSelection({t, 1});
            // var_u64.SetStepSelection({t, 1});

            // var_r32.SetStepSelection({t, 1});
            var_r64.SetStepSelection({t, 1});

            // SiriusReader.Get(var_iString, IString);

            // SiriusReader.Get(var_i8, I8.data());
            // SiriusReader.Get(var_i16, I16.data());
            // SiriusReader.Get(var_i32, I32.data());
            // SiriusReader.Get(var_i64, I64.data());

            // SiriusReader.Get(var_u8, U8.data());
            // SiriusReader.Get(var_u16, U16.data());
            // SiriusReader.Get(var_u32, U32.data());
            // SiriusReader.Get(var_u64, U64.data());

            // SiriusReader.Get(var_r32, R32.data());
            SiriusReader.Get(var_r64, R64.data());

            SiriusReader.PerformGets();

            // Generate test data for each rank uniquely
            SmallTestData currentTestData = generateNewSmallTestData(
                m_TestData, static_cast<int>(t), mpiRank, mpiSize);

            // EXPECT_EQ(IString, currentTestData.S1);

            for (size_t i = 0; i < Nx * Ny; ++i)
            {
                std::stringstream ss;
                ss << "t=" << t << " i=" << i << " rank=" << mpiRank;
                std::string msg = ss.str();

                // EXPECT_EQ(I8[i], currentTestData.I8[i]) << msg;
                // EXPECT_EQ(I16[i], currentTestData.I16[i]) << msg;
                // EXPECT_EQ(I32[i], currentTestData.I32[i]) << msg;
                // EXPECT_EQ(I64[i], currentTestData.I64[i]) << msg;
                // EXPECT_EQ(U8[i], currentTestData.U8[i]) << msg;
                // EXPECT_EQ(U16[i], currentTestData.U16[i]) << msg;
                // EXPECT_EQ(U32[i], currentTestData.U32[i]) << msg;
                // EXPECT_EQ(U64[i], currentTestData.U64[i]) << msg;
                // EXPECT_EQ(R32[i], currentTestData.R32[i]) << msg;
                EXPECT_EQ(R64[i], currentTestData.R64[i]) << msg;
            }
        }
        SiriusReader.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}