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

\*---------------------------------------------------------------------------*/

#include "pointFieldReconstructor.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pointFieldReconstructor::pointFieldReconstructor
(
    const pointMesh& completeMesh,
    const PtrList<fvMesh>& procMeshes,
    const labelListList& pointProcAddressing
)
:
    completeMesh_(completeMesh),
    procMeshes_(procMeshes),
    pointProcAddressing_(pointProcAddressing),
    patchPointAddressing_(procMeshes.size())
{
    // Inverse-addressing of the patch point labels.
    labelList pointMap(completeMesh_.size(), -1);

    // Create the pointPatch addressing
    forAll(procMeshes_, proci)
    {
        const pointMesh& procMesh = pointMesh::New(procMeshes_[proci]);

        patchPointAddressing_[proci].setSize(procMesh.boundary().size());

        forAll(procMesh.boundary(), patchi)
        {
            if (patchi < completeMesh_.boundary().size())
            {
                labelList& procPatchAddr = patchPointAddressing_[proci][patchi];
                procPatchAddr.setSize(procMesh.boundary()[patchi].size(), -1);

                const labelList& patchPointLabels =
                    completeMesh_.boundary()[patchi].meshPoints();

                // Create the inverse-addressing of the patch point labels.
                forAll(patchPointLabels, pointi)
                {
                    pointMap[patchPointLabels[pointi]] = pointi;
                }

                const labelList& procPatchPoints =
                    procMesh.boundary()[patchi].meshPoints();

                forAll(procPatchPoints, pointi)
                {
                    procPatchAddr[pointi] =
                        pointMap
                        [
                            pointProcAddressing_[proci][procPatchPoints[pointi]]
                        ];
                }

                if (procPatchAddr.size() && min(procPatchAddr) < 0)
                {
                    FatalErrorInFunction
                        << "Incomplete patch point addressing"
                        << abort(FatalError);
                }
            }
        }
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::pointFieldReconstructor::reconstructs
(
    const IOobjectList& objects,
    const HashSet<word>& selectedFields
)
{
    bool result = false;

    #define DO_POINT_FIELDS_TYPE(Type, nullArg)                                \
        result = result                                                        \
         || reconstructs<PointField<Type>>(objects, selectedFields);
    FOR_ALL_FIELD_TYPES(DO_POINT_FIELDS_TYPE)
    #undef DO_POINT_FIELDS_TYPE

    return result;
}


// ************************************************************************* //
