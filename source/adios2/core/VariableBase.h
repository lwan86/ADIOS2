/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariableBase.h
 *
 *  Created on: Feb 20, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_CORE_VARIABLEBASE_H_
#define ADIOS2_CORE_VARIABLEBASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <exception>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/SelectionBoundingBox.h"
#include "adios2/core/adiosFunctions.h"
#include "adios2/core/adiosTemplates.h"

namespace adios
{

using Dims = std::vector<std::size_t>;

class VariableBase
{

public:
    const std::string m_Name;   ///< variable name
    const std::string m_Type;   ///< variable type
    const bool m_ConstantShape; ///< dimensions and offsets cannot change after
                                /// declaration

    /**
     * Variable -> sizeof(T),
     * VariableCompound -> from constructor sizeof(struct)
     */
    const std::size_t m_ElementSize;

    Dims m_Shape;            ///< total dimensions across MPI
    Dims m_Start;            ///< offsets of local writer in global shape
    Dims m_Count;            ///< dimensions of the local writer in global shape
    Dims m_MemoryDimensions; ///< array of memory dimensions
    Dims m_MemoryOffsets;    ///< array of memory offsets
    bool m_IsScalar = false;
    const bool m_IsDimension = false;
    const bool m_DebugMode = false;

    VariableBase(const std::string &name, const std::string type,
                 const std::size_t elementSize, const Dims shape,
                 const Dims start, const Dims count, const bool constantShape,
                 const bool debugMode)
    : m_Name{name}, m_Type{type}, m_ConstantShape{constantShape},
      m_ElementSize{elementSize}, m_Count{count}, m_Shape{shape},
      m_Start{start}, m_DebugMode{debugMode}
    {
        if (shape.empty())
            m_IsScalar = true;
    }

    virtual ~VariableBase() {}

    std::size_t DimensionsSize() const noexcept { return m_Count.size(); }

    /**
     * Returns the payload size in bytes
     * @return TotalSize * sizeof(T)
     */
    std::size_t PayLoadSize() const noexcept
    {
        return GetTotalSize(m_Count) * m_ElementSize;
    }

    /**
     * Returns the total size
     * @return number of elements
     */
    std::size_t TotalSize() const noexcept { return GetTotalSize(m_Count); }

    /**
     * Set the local dimension and global offset of the variable
     */
    void SetSelection(const Dims start, const Dims count)
    {
        if (m_IsScalar)
        {
            throw std::invalid_argument("Variable.SetSelection() is an invalid "
                                        "call for single value variables\n");
        }
        if (m_ConstantShape)
        {
            throw std::invalid_argument(
                "Variable.SetSelection() is not allowed "
                "for arrays with a constant shape\n");
        }
        if (m_Shape.size() != count.size())
        {
            throw std::invalid_argument("Variable.SetSelection() bounding box "
                                        "dimension must equal the global "
                                        "dimension of the variable\n");
        }
        ConvertUint64VectorToSizetVector(count, m_Count);
        ConvertUint64VectorToSizetVector(start, m_Start);
    }

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     */
    void SetSelection(const SelectionBoundingBox &sel)
    {
        SetSelection(sel.m_Start, sel.m_Count);
    }

    /**
     * Set the local dimension and global offset of the variable using a
     * selection
     * Only bounding boxes are allowed
     */
    void SetMemorySelection(const SelectionBoundingBox &sel)
    {
        if (m_Shape.size() == 0)
        {
            throw std::invalid_argument(
                "Variable.SetMemorySelection() is an invalid "
                "call for single value variables\n");
        }
        if (m_Shape.size() != sel.m_Count.size())
        {
            throw std::invalid_argument(
                "Variable.SetMemorySelection() bounding box "
                "dimension must equal the global "
                "dimension of the variable\n");
        }

        ConvertUint64VectorToSizetVector(sel.m_Count, m_MemoryDimensions);
        ConvertUint64VectorToSizetVector(sel.m_Start, m_MemoryOffsets);
    }

    /** Return the number of steps available for the variable
     *  @return Number of steps
     */
    int GetNSteps() { return m_nsteps; }

private:
    /* Values filled by InquireVariable() */
    int m_nsteps =
        1; ///< number of steps available in a file (or 1 in staging),
};

} // end namespace

#endif /* ADIOS2_CORE_VARIABLEBASE_H_ */
