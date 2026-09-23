/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::vtk::vtmWriter

Description
    Provides a means of accumulating file entries for generating
    a vtkMultiBlockDataSet (.vtm) file.

    For example, to generate the following content:
    \verbatim
    <?xml version='1.0'?>
    <VTKFile type='vtkMultiBlockDataSet' ...>
      <vtkMultiBlockDataSet>
        <DataSet name='internal' file='internal.vtu' />
        <Block name='boundary'>
          <DataSet name='inlet' file='boundary/inlet.vtp' />
          <DataSet name='outlet' file='boundary/outlet.vtp' />
        </Block>
      </vtkMultiBlockDataSet>
      <FieldData>
        <DataArray type='Float32' Name='TimeValue' ...>
           12.345
        </DataArray>
      </FieldData>
    </VTKFile>
    \endverbatim

    The following code would be used:
    \code
        vtm.clear();
        vtm.setTime(12.345);

        vtm.append("internal", "internal.vtu");

        vtm.beginBlock("boundary");
        vtm.append("boundary/inlet.vtp");
        vtm.append("boundary/outlet.vtp");

        vtm.write("outputName");
    \endcode

SourceFiles
    foamVtmWriter.cxx
    foamVtmWriterI.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_vtk_vtmWriter_H
#define Foam_vtk_vtmWriter_H

#include "foamVtkOutputOptions.H"
#include "DynamicList.H"
#include <ostream>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations
class Time;
class OSstream;

namespace vtk
{

/*---------------------------------------------------------------------------*\
                       Class vtk::vtmWriter Declaration
\*---------------------------------------------------------------------------*/

class vtmWriter
{
    //- Simple structure for containing entries
    struct vtmEntry
    {
        enum entryType : char
        {
            NONE = 0,
            DATA = 'D',
            BEGIN_BLOCK = '{', END_BLOCK = '}',
        };

        //- The entry type (none, node or leaf begin/end)
        char type_;

        //- The node data type (eg, MultiBlock, PolyData, ...)
        vtk::fileTag content_;

        //- The 'name' entry (to describe block or data)
        string name_;

        //- The 'file' entry (data only)
        fileName file_;

        // Constructors

            vtmEntry(const vtmEntry&) = default;
            vtmEntry(vtmEntry&&) = default;
            vtmEntry& operator=(const vtmEntry&) = default;
            vtmEntry& operator=(vtmEntry&&) = default;

            //- Default construct
            vtmEntry()
            :
                type_(entryType::NONE),
                content_(vtk::fileTag::UNKNOWN)
            {}

            //- Construct from components
            vtmEntry
            (
                int what,
                const string& name,
                const fileName& file,
                vtk::fileTag content = vtk::fileTag::UNKNOWN
            )
            :
                type_(what), content_(content), name_(name), file_(file)
            {}


        // Factory Methods

            //- Begin a named block
            static vtmEntry block(const string& name)
            {
                return vtmEntry(entryType::BEGIN_BLOCK, name, "");
            }

            //- End a block
            static vtmEntry endblock()
            {
                return vtmEntry(entryType::END_BLOCK, "", "");
            }

            //- Specify a data entry
            static vtmEntry entry
            (
                const fileName& file,
                vtk::fileTag content = vtk::fileTag::UNKNOWN
            )
            {
                return vtmEntry(entryType::DATA, "", file, content);
            }

            //- Specify a named data entry
            static vtmEntry entry
            (
                const string& name,
                const fileName& file,
                vtk::fileTag content = vtk::fileTag::UNKNOWN
            )
            {
                return vtmEntry(entryType::DATA, name, file, content);
            }


        // Member Functions

            //- Test the type
            bool isType(entryType what) const noexcept
            {
                return (type_ == what);
            }

            //- Change the content type for a node
            bool setDataType(vtk::fileTag dataType) noexcept
            {
                if (type_ == entryType::DATA)
                {
                    content_ = dataType;
                    return true;
                }
                else
                {
                    return false;
                }
            }

            //- Reset to NONE
            void clear();

            //- True if the entry is good.
            bool good() const noexcept;

            //- Output valid entry as XML
            bool write(vtk::formatter& format) const;
    };


    // Private Member Data

        //- A vtm file entry: begin/end block, dataset
        DynamicList<vtmEntry> entries_;

        //- LIFO stack of current block names
        DynamicList<word> blocks_;

        //- TimeValue for FieldData
        scalar timeValue_;

        //- Auto-generate names from 'file' entry?
        bool autoName_;

        //- Has a TimeValue for FieldData?
        bool hasTime_;


    // Private Member Functions

        //- Remove NONE entries
        bool pruneEmpty();

        //- Remove empty blocks
        bool pruneEmptyBlocks();

        //- Collapse block if it has a single dataset and the names allow it
        bool collapseBlocks();

        //- Add a file with specified file and data types.
        //- The name is either empty or created with autoName
        //  \return True if file is non-empty
        template
        <
            vtk::fileTag FileType,
            vtk::fileTag DataType = FileType
        >
        inline bool appendTyped(const fileName& file);

        //- Add a file with name, specified file and data types.
        //  \return True if file is non-empty
        template
        <
            vtk::fileTag FileType,
            vtk::fileTag DataType = FileType
        >
        inline bool appendTyped(const word& name, const fileName& file);


public:

    // Constructors

        //- Default construct, with autoName on
        vtmWriter();

        //- Construct with specified behaviour for autoName
        explicit vtmWriter(bool autoName);


    //- Destructor
    ~vtmWriter() = default;


    // Member Functions

        //- File extension (always "vtm")
        inline static word ext();

        //- If there are no data sets
        bool empty() const;

        //- The number of data sets
        label size() const;


    // Content Management

        //- Clear all entries and reset output
        void clear();

        //- Define "TimeValue" for FieldData (name as per Catalyst output)
        void setTime(scalar timeValue);

        //- Define "TimeValue" for FieldData (name as per Catalyst output)
        void setTime(const Time& t);


        //- Start a new block, optionally with a name
        //  \return block depth
        label beginBlock(const word& blockName = word::null);

        //- End the previous block, optionally with name checking
        //  \return block depth
        label endBlock(const word& blockName = word::null);


        //- Add a file. The name is either empty or created with autoName
        //  \return True if file is non-empty
        bool append(const fileName& file);

        //- Add a file with name
        //  \return True if file is non-empty
        bool append(const word& name, const fileName& file);

        //- Add a file with given contentType extension
        //- The name is either empty or created with autoName
        //  \return True if file is non-empty
        bool append(const fileName& file, vtk::fileTag contentType);

        //- Add a file with name, with given contentType extension
        //  \return True if file is non-empty
        bool append
        (
            const word& name,
            const fileName& file,
            vtk::fileTag contentType
        );


    // Convenience Methods

        //- Add a PolyData (.vtp) file
        //  \return True if file is non-empty
        inline bool append_poly(const fileName& file);

        //- Add a PolyData (.vtp) file with name
        //  \return True if file is non-empty
        inline bool append_poly(const word& name, const fileName& file);

        //- Add an UnstructuredGrid (.vtu) file
        //  \return True if file is non-empty
        inline bool append_ugrid(const fileName& file);

        //- Add an UnstructuredGrid (.vtu) file with name
        //  \return True if file is non-empty
        inline bool append_ugrid(const word& name, const fileName& file);

        //- Add a (.vtkhdf) file. Often need to specify the
        //- content type as well (PolyData, UnstructuredGrid)
        //  \return True if file is non-empty
        template<vtk::fileTag DataType = vtk::fileTag::VTK_HDF>
        inline bool append_hdf(const fileName& file);

        //- Add a (.vtkhdf) file with name. Often need to specify
        //- content type as well (PolyData, UnstructuredGrid)
        //  \return True if file is non-empty
        template<vtk::fileTag DataType = vtk::fileTag::VTK_HDF>
        inline bool append_hdf(const word& name, const fileName& file);


    // Content Management

        //- Sanity fixes on the data
        void repair(bool collapse=false);

        //- Add in content from another vtm and place under the given block
        //- name.
        void add(const word& blockName, const vtmWriter& other);

        //- Add in content from another vtm and place under the given block
        //- name. Adjust the added 'file' entries to include the given prefix.
        void add
        (
            const word& blockName,
            const fileName& prefix,
            const vtmWriter& other
        );


    // Writing

        //- Write the blocks and TimeValue (xml format).
        //  \return number of data sets
        label write_xml(std::ostream& os) const;

        //- Write the blocks and TimeValue (xml format).
        //  \return number of data sets
        label write_xml(OSstream& os) const;

        //- Open file for writing (creates parent directory) and write the
        //- blocks and TimeValue.
        //  The file name is with/without an extension.
        //  \return number of data sets
        label write_xml(const fileName& file) const;

        //- Forwards to write_xml().
        //- \return number of data sets
        label write(const fileName& file) const;

        //- Print debug view of block and dataset contents
        void dump(Ostream& os) const;


    // Housekeeping

        //- Same as append_poly - used until (2025-12)
        bool append_vtp(const fileName& file) { return append_poly(file); }

        //- Same as append_ugrid - used until (2025-12)
        bool append_vtu(const fileName& file) { return append_ugrid(file); }

        //- Same as append_poly - used until (2025-12)
        bool append_vtp(const word& name, const fileName& file)
        {
            return append_poly(name, file);
        }

        //- Same as append_ugrid - used until (2025-12)
        bool append_vtu(const word& name, const fileName& file)
        {
            return append_ugrid(name, file);
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace vtk
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "foamVtmWriterI.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
