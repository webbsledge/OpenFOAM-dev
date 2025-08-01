/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2025 OpenFOAM Foundation
     \\/     M anipulation  |
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

Description
    Misc helper methods and utilities

\*---------------------------------------------------------------------------*/

#include "vtkPVblockMesh.H"
#include "vtkPVblockMeshReader.h"

// VTK includes
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    //! \cond fileScope
    //  Extract up to the first non-word characters
    inline word getFirstWord(const char* str)
    {
        if (str)
        {
            label n = 0;
            while (str[n] && word::valid(str[n]))
            {
                ++n;
            }
            return word(str, n, true);
        }
        else
        {
            return word::null;
        }

    }
    //! \endcond

} // End namespace Foam



// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::vtkPVblockMesh::AddToBlock
(
    vtkMultiBlockDataSet* output,
    vtkDataSet* dataset,
    const arrayRange& range,
    const label datasetNo,
    const std::string& datasetName
)
{
    const int blockNo = range.block();

    vtkDataObject* blockDO = output->GetBlock(blockNo);
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(blockDO);

    if (!block)
    {
        if (blockDO)
        {
            FatalErrorInFunction
                << "Block already has a vtkDataSet assigned to it"
                << endl;
        }

        block = vtkMultiBlockDataSet::New();
        output->SetBlock(blockNo, block);
        block->Delete();
    }

    block->SetBlock(datasetNo, dataset);

    // name the block when assigning the first dataset
    if (datasetNo == 0)
    {
        output->GetMetaData(blockNo)->Set
        (
            vtkCompositeDataSet::NAME(),
            range.name()
        );
    }

    if (datasetName.size())
    {
        block->GetMetaData(datasetNo)->Set
        (
            vtkCompositeDataSet::NAME(),
            datasetName.c_str()
        );
    }
}


vtkDataSet* Foam::vtkPVblockMesh::GetDataSetFromBlock
(
    vtkMultiBlockDataSet* output,
    const arrayRange& range,
    const label datasetNo
)
{
    const int blockNo = range.block();

    vtkDataObject* blockDO = output->GetBlock(blockNo);
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(blockDO);

    if (block)
    {
        return vtkDataSet::SafeDownCast(block->GetBlock(datasetNo));
    }

    return 0;
}


Foam::label Foam::vtkPVblockMesh::GetNumberOfDataSets
(
    vtkMultiBlockDataSet* output,
    const arrayRange& range
)
{
    const int blockNo = range.block();

    vtkDataObject* blockDO = output->GetBlock(blockNo);
    vtkMultiBlockDataSet* block = vtkMultiBlockDataSet::SafeDownCast(blockDO);

    if (block)
    {
        return block->GetNumberOfBlocks();
    }

    return 0;
}


Foam::wordHashSet Foam::vtkPVblockMesh::getSelected
(
    vtkDataArraySelection* select
)
{
    int nElem = select->GetNumberOfArrays();
    wordHashSet selections(2*nElem);

    for (int elemI=0; elemI < nElem; ++elemI)
    {
        if (select->GetArraySetting(elemI))
        {
            selections.insert(getFirstWord(select->GetArrayName(elemI)));
        }
    }

    return selections;
}


Foam::wordHashSet Foam::vtkPVblockMesh::getSelected
(
    vtkDataArraySelection* select,
    const arrayRange& range
)
{
    int nElem = select->GetNumberOfArrays();
    wordHashSet selections(2*nElem);

    for (int elemI = range.start(); elemI < range.end(); ++elemI)
    {
        if (select->GetArraySetting(elemI))
        {
            selections.insert(getFirstWord(select->GetArrayName(elemI)));
        }
    }

    return selections;
}


Foam::stringList Foam::vtkPVblockMesh::getSelectedArrayEntries
(
    vtkDataArraySelection* select,
    const bool debug
)
{
    stringList selections(select->GetNumberOfArrays());

    label nElem = 0;

    forAll(selections, elemI)
    {
        if (select->GetArraySetting(elemI))
        {
            selections[nElem++] = select->GetArrayName(elemI);
        }
    }

    selections.setSize(nElem);

    // Debugging ...
    const label nAvailableElem = select->GetNumberOfArrays();
    DebugInFunction
        << "available(";
    for (int elemI = 0; elemI < nAvailableElem; ++elemI)
    {
        DebugInfo<< " \"" << select->GetArrayName(elemI) << "\"";
    }
    DebugInfo<< " )\nselected(";
    forAll(selections, elemI)
    {
        DebugInfo<< " " << selections[elemI];
    }
    DebugInfo<< " )" << endl;

    return selections;
}


Foam::stringList Foam::vtkPVblockMesh::getSelectedArrayEntries
(
    vtkDataArraySelection* select,
    const arrayRange& range,
    const bool debug
)
{
    stringList selections(range.size());

    label nElem = 0;

    for (int elemI = range.start(); elemI < range.end(); ++elemI)
    {
        if (select->GetArraySetting(elemI))
        {
            selections[nElem++] = select->GetArrayName(elemI);
        }
    }

    selections.setSize(nElem);

    // Debugging ...
    DebugInFunction
        << "available(";
    for (int elemI = range.start(); elemI < range.end(); ++elemI)
    {
        DebugInfo<< " \"" << select->GetArrayName(elemI) << "\"";
    }
    DebugInfo<< " )\nselected(";
    forAll(selections, elemI)
    {
        DebugInfo<< " " << selections[elemI];
    }
    DebugInfo<< " )" << endl;

    return selections;
}


void Foam::vtkPVblockMesh::setSelectedArrayEntries
(
    vtkDataArraySelection* select,
    const stringList& selections
)
{
    const int nElem = select->GetNumberOfArrays();

    select->DisableAllArrays();

    // Loop through entries, setting values from selectedEntries
    for (int elemI=0; elemI < nElem; ++elemI)
    {
        string arrayName(select->GetArrayName(elemI));

        forAll(selections, elemI)
        {
            if (selections[elemI] == arrayName)
            {
                select->EnableArray(arrayName.c_str());
                break;
            }
        }
    }
}


void Foam::vtkPVblockMesh::updateBoolListStatus
(
    boolList& status,
    vtkDataArraySelection* selection
)
{
    const label nElem = selection->GetNumberOfArrays();

    if (status.size() != nElem)
    {
        status.setSize(nElem);
        status = false;
    }

    forAll(status, elemI)
    {
        status[elemI] = selection->GetArraySetting(elemI);
    }
}


// ************************************************************************* //
