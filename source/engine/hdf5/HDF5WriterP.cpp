/*
 * HDF5WriterP.cpp
 *
 *  Created on: Mar 23, 2017
 *      Author: junmin
 */

#include <iostream> //needs to go away, this is just for demo purposes

#ifdef ADIOS_HAVE_MPI
#include "engine/hdf5/HDF5WriterP.h"

#include "core/Support.h"
#include "functions/adiosFunctions.h" //CSVToVector

// supported transports
//#include "transport/file/FD.h"      // uses POSIX
//#include "transport/file/FP.h"      // uses C FILE*
//#include "transport/file/FStream.h" // uses C++ fstream

namespace adios {

HDF5Writer::HDF5Writer(ADIOS &adios, const std::string name,
                       const std::string accessMode, MPI_Comm mpiComm,
                       const Method &method, const bool debugMode,
                       const unsigned int cores)
    : Engine(adios, "HDF5Writer", name, accessMode, mpiComm, method, debugMode,
             cores, " HDF5Writer constructor (or call to ADIOS Open).\n"),
      m_Buffer(accessMode, m_RankMPI, m_DebugMode) {
  DefH5T_COMPLEX_DOUBLE =
      H5Tcreate(H5T_COMPOUND, sizeof(ADIOS2_Complex_Double));
  DefH5T_COMPLEX_FLOAT = H5Tcreate(H5T_COMPOUND, sizeof(ADIOS2_Complex_Float));
  DefH5T_COMPLEX_LongDOUBLE =
      H5Tcreate(H5T_COMPOUND, sizeof(ADIOS2_Complex_LongDouble));

  // compound data type for memory
  H5Tinsert(DefH5T_COMPLEX_DOUBLE, "double real",
            HOFFSET(ADIOS2_Complex_Double, _re), H5T_NATIVE_DOUBLE);
  H5Tinsert(DefH5T_COMPLEX_DOUBLE, "double img",
            HOFFSET(ADIOS2_Complex_Double, _im), H5T_NATIVE_DOUBLE);
  H5Tinsert(DefH5T_COMPLEX_FLOAT, "float real",
            HOFFSET(ADIOS2_Complex_Float, _re), H5T_NATIVE_FLOAT);
  H5Tinsert(DefH5T_COMPLEX_FLOAT, "float img",
            HOFFSET(ADIOS2_Complex_Float, _im), H5T_NATIVE_FLOAT);
  H5Tinsert(DefH5T_COMPLEX_LongDOUBLE, "ldouble real",
            HOFFSET(ADIOS2_Complex_LongDouble, _re), H5T_NATIVE_LDOUBLE);
  H5Tinsert(DefH5T_COMPLEX_LongDOUBLE, "ldouble img",
            HOFFSET(ADIOS2_Complex_LongDouble, _im), H5T_NATIVE_LDOUBLE);

  // data type for file
  int ldsize = H5Tget_size(H5T_NATIVE_DOUBLE);
  DefH5T_filetype_COMPLEX_DOUBLE = H5Tcreate(H5T_COMPOUND, ldsize + ldsize);
  H5Tinsert(DefH5T_filetype_COMPLEX_DOUBLE, "double real", 0, H5T_IEEE_F64BE);
  H5Tinsert(DefH5T_filetype_COMPLEX_DOUBLE, "double img", ldsize,
            H5T_IEEE_F64BE);

  ldsize = H5Tget_size(H5T_NATIVE_FLOAT);
  DefH5T_filetype_COMPLEX_FLOAT = H5Tcreate(H5T_COMPOUND, ldsize + ldsize);
  H5Tinsert(DefH5T_filetype_COMPLEX_FLOAT, "float real", 0, H5T_IEEE_F32BE);
  H5Tinsert(DefH5T_filetype_COMPLEX_FLOAT, "float img", ldsize, H5T_IEEE_F32BE);

  ldsize = H5Tget_size(
      H5T_NATIVE_LDOUBLE); // note that sizeof(H5T_NATIVE_LDOUBLE) returned 4!!
  DefH5T_filetype_COMPLEX_LongDOUBLE = H5Tcreate(H5T_COMPOUND, ldsize + ldsize);
  H5Tinsert(DefH5T_filetype_COMPLEX_LongDOUBLE, "ldouble real", 0,
            H5T_NATIVE_LDOUBLE);
  H5Tinsert(DefH5T_filetype_COMPLEX_LongDOUBLE, "ldouble img", ldsize,
            H5T_NATIVE_LDOUBLE);

  Init();
}

HDF5Writer::~HDF5Writer() {}

void HDF5Writer::Init() {
  if (m_AccessMode != "w" && m_AccessMode != "write" && m_AccessMode != "a" &&
      m_AccessMode != "append") {
    throw std::invalid_argument(
        "ERROR: HDF5Writer doesn't support access mode " + m_AccessMode +
        ", in call to ADIOS Open or HDF5Writer constructor\n");
  }
  std::cout << "method: # of inputs:" << m_Method.m_Parameters.size()
            << std::endl;
  std::cout << "::Init hdf5 parallel writer. File name:" << m_Name << std::endl;

  _plist_id = H5Pcreate(H5P_FILE_ACCESS);

#ifdef HAVE_MPI
  H5Pset_fapl_mpio(_plist_id, m_MPIComm, MPI_INFO_NULL);
#endif

  /*
   * Create a new file collectively and release property list identifier.
   */
  _file_id = H5Fcreate(m_Name.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, _plist_id);
  H5Pclose(_plist_id);
}

void HDF5Writer::Write(Variable<char> &variable, const char *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_CHAR);
}

void HDF5Writer::Write(Variable<unsigned char> &variable,
                       const unsigned char *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_UCHAR);
}

void HDF5Writer::Write(Variable<short> &variable, const short *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_SHORT);
}

void HDF5Writer::Write(Variable<unsigned short> &variable,
                       const unsigned short *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_USHORT);
}

void HDF5Writer::Write(Variable<int> &variable, const int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_INT);
}

void HDF5Writer::Write(Variable<unsigned int> &variable,
                       const unsigned int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_UINT);
}

void HDF5Writer::Write(Variable<long int> &variable, const long int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_LONG);
}

void HDF5Writer::Write(Variable<unsigned long int> &variable,
                       const unsigned long int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_ULONG);
}

void HDF5Writer::Write(Variable<long long int> &variable,
                       const long long int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_LLONG);
}

void HDF5Writer::Write(Variable<unsigned long long int> &variable,
                       const unsigned long long int *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_ULLONG);
}

void HDF5Writer::Write(Variable<float> &variable, const float *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_FLOAT);
}

void HDF5Writer::Write(Variable<double> &variable, const double *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_DOUBLE);
}

void HDF5Writer::Write(Variable<long double> &variable,
                       const long double *values) {
  UseHDFWrite(variable, values, H5T_NATIVE_LDOUBLE);
}

void HDF5Writer::Write(Variable<std::complex<float>> &variable,
                       const std::complex<float> *values) {
  UseHDFWrite(variable, values, DefH5T_filetype_COMPLEX_FLOAT);
}

void HDF5Writer::Write(Variable<std::complex<double>> &variable,
                       const std::complex<double> *values) {
  UseHDFWrite(variable, values, DefH5T_filetype_COMPLEX_DOUBLE);
}

void HDF5Writer::Write(Variable<std::complex<long double>> &variable,
                       const std::complex<long double> *values) {
  UseHDFWrite(variable, values, DefH5T_filetype_COMPLEX_LongDOUBLE);
}

// String version
void HDF5Writer::Write(const std::string variableName, const char *values) {
  UseHDFWrite(m_ADIOS.GetVariable<char>(variableName), values, H5T_NATIVE_CHAR);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned char *values) {
  UseHDFWrite(m_ADIOS.GetVariable<unsigned char>(variableName), values,
              H5T_NATIVE_UCHAR);
}

void HDF5Writer::Write(const std::string variableName, const short *values) {
  UseHDFWrite(m_ADIOS.GetVariable<short>(variableName), values,
              H5T_NATIVE_SHORT);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned short *values) {
  UseHDFWrite(m_ADIOS.GetVariable<unsigned short>(variableName), values,
              H5T_NATIVE_USHORT);
}

void HDF5Writer::Write(const std::string variableName, const int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<int>(variableName), values, H5T_NATIVE_INT);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<unsigned int>(variableName), values,
              H5T_NATIVE_UINT);
}

void HDF5Writer::Write(const std::string variableName, const long int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<long int>(variableName), values,
              H5T_NATIVE_LONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned long int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<unsigned long int>(variableName), values,
              H5T_NATIVE_ULONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const long long int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<long long int>(variableName), values,
              H5T_NATIVE_LLONG);
}

void HDF5Writer::Write(const std::string variableName,
                       const unsigned long long int *values) {
  UseHDFWrite(m_ADIOS.GetVariable<unsigned long long int>(variableName), values,
              H5T_NATIVE_ULLONG);
}

void HDF5Writer::Write(const std::string variableName, const float *values) {
  UseHDFWrite(m_ADIOS.GetVariable<float>(variableName), values,
              H5T_NATIVE_FLOAT);
}

void HDF5Writer::Write(const std::string variableName, const double *values) {
  UseHDFWrite(m_ADIOS.GetVariable<double>(variableName), values,
              H5T_NATIVE_DOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const long double *values) {
  UseHDFWrite(m_ADIOS.GetVariable<long double>(variableName), values,
              H5T_NATIVE_LDOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<float> *values) {
  UseHDFWrite(m_ADIOS.GetVariable<std::complex<float>>(variableName), values,
              DefH5T_COMPLEX_FLOAT);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<double> *values) {
  UseHDFWrite(m_ADIOS.GetVariable<std::complex<double>>(variableName), values,
              DefH5T_COMPLEX_DOUBLE);
}

void HDF5Writer::Write(const std::string variableName,
                       const std::complex<long double> *values) {
  UseHDFWrite(m_ADIOS.GetVariable<std::complex<long double>>(variableName),
              values, DefH5T_COMPLEX_LongDOUBLE);
}

void HDF5Writer::Close(const int transportIndex) {
  std::cout << " ===> CLOSING HDF5 <===== " << std::endl;
  // H5Dclose(_dset_id);
  // H5Sclose(_filespace);
  // H5Sclose(_memspace);
  // H5Pclose(_plist_id);
  H5Fclose(_file_id);
}

template <class T>
void HDF5Writer::UseHDFWrite(Variable<T> &variable, const T *values,
                             hid_t h5type) {
  // here comes your magic at Writing now variable.m_UserValues has the data
  // passed by the user
  // set variable
  variable.m_AppValues = values;
  m_WrittenVariables.insert(variable.m_Name);

  std::cout << "writting : " << variable.m_Name
            << " dim size:" << variable.m_GlobalDimensions.size() << std::endl;

  int dimSize = variable.m_GlobalDimensions.size();
  for (int i = 0; i < dimSize; i++) {
    std::cout << " dim: " << i << ", size:" << variable.m_GlobalDimensions[i]
              << " offset=" << variable.m_GlobalOffsets[i]
              << " count=" << variable.m_Dimensions[i] << std::endl;
  }
  std::vector<hsize_t> dimsf, count, offset;

  for (int i = 0; i < dimSize; i++) {
    dimsf.push_back(variable.m_GlobalDimensions[i]);
    count.push_back(variable.m_Dimensions[i]);
    offset.push_back(variable.m_GlobalOffsets[i]);
  }

  _filespace = H5Screate_simple(dimSize, dimsf.data(), NULL);

  _dset_id = H5Dcreate(_file_id, variable.m_Name.c_str(), h5type, _filespace,
                       H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  // H5Sclose(_filespace);

  _memspace = H5Screate_simple(dimSize, count.data(), NULL);

  // Select hyperslab
  _filespace = H5Dget_space(_dset_id);
  H5Sselect_hyperslab(_filespace, H5S_SELECT_SET, offset.data(), NULL,
                      count.data(), NULL);

  //  Create property list for collective dataset write.
  _plist_id = H5Pcreate(H5P_DATASET_XFER);
  H5Pset_dxpl_mpio(_plist_id, H5FD_MPIO_COLLECTIVE);

  herr_t status;

  if (h5type == DefH5T_filetype_COMPLEX_FLOAT) {
    /*
    ADIOS2_Complex_Float v[3];
    v[0]._re = 1.1, v[0]._im = 1.2;
    v[1]._re = 2.1, v[1]._im = 2.2;
    v[2]._re = 3.1, v[2]._im = 3.2;
    // status = H5Dwrite(_dset_id, DefH5T_COMPLEX_FLOAT, _memspace, _filespace,
    // _plist_id, values);
    status = H5Dwrite(_dset_id, DefH5T_COMPLEX_FLOAT, _memspace, _filespace,
                      _plist_id, v);
    */
  } else if (h5type == DefH5T_filetype_COMPLEX_DOUBLE) {
    /*
    ADIOS2_Complex_Double v[3];
    v[0]._re = 1.1, v[0]._im = 1.2;
    v[1]._re = 2.1, v[1]._im = 2.2;
    v[2]._re = 3.1, v[2]._im = 3.2;
    status = H5Dwrite(_dset_id, DefH5T_COMPLEX_DOUBLE, _memspace, _filespace,
                      _plist_id, v);
    */
  } else if (h5type == DefH5T_filetype_COMPLEX_LongDOUBLE) {
    /*
    ADIOS2_Complex_LongDouble v[3];
    v[0]._re = 1.1, v[0]._im = 1.2;
    v[1]._re = 2.1, v[1]._im = 2.2;
    v[2]._re = 3.1, v[2]._im = 3.2;
    status = H5Dwrite(_dset_id, DefH5T_COMPLEX_LongDOUBLE, _memspace,
                      _filespace, _plist_id, v);
    */
  } else {
    status =
        H5Dwrite(_dset_id, h5type, _memspace, _filespace, _plist_id, values);
  }

  if (status < 0) {
    // error
    std::cerr << " Write failed. " << std::endl;
  }

  std::cout << " ==> User is responsible for freeing the data " << std::endl;
  // free(values);

  // Close/release resources.

  // H5Dclose(_dset_id);
  // H5Sclose(_filespace);
  // H5Sclose(_memspace);
  // H5Pclose(_plist_id);
  // H5Fclose(_file_id);

  H5Dclose(_dset_id);
  H5Sclose(_filespace);
  H5Sclose(_memspace);
  H5Pclose(_plist_id);
}

} // end namespace adios

#endif //ADIOS_HAVE_MPI 
