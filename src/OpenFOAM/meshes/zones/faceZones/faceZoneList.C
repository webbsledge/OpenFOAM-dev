/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2024-2025 OpenFOAM Foundation
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

#include "faceZoneList.H"
#include "polyMesh.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(faceZoneList, 0);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::label Foam::faceZoneList::allSize() const
{
    return mesh().nFaces();
}


void Foam::faceZoneList::insert(const List<Map<bool>>& zonesOrientedIndices)
{
    PtrList<faceZone>& zones = *this;

    if (zonesOrientedIndices.size() != zones.size())
    {
        FatalErrorInFunction
            << "zonesOrientedIndices.size() " << zonesOrientedIndices.size()
            << " != number of zones " << zones.size()
            << exit(FatalError);
    }

    forAll(zonesOrientedIndices, zonei)
    {
        if (zonesOrientedIndices[zonei].size())
        {
            zones[zonei].insert(zonesOrientedIndices[zonei]);
        }
    }
}


void Foam::faceZoneList::insert(const List<labelHashSet>& zonesIndices)
{
    PtrList<faceZone>& zones = *this;

    if (zonesIndices.size() != zones.size())
    {
        FatalErrorInFunction
            << "zonesIndices.size() " << zonesIndices.size()
            << " != number of zones " << zones.size()
            << exit(FatalError);
    }

    forAll(zonesIndices, zonei)
    {
        if (zonesIndices[zonei].size())
        {
            zones[zonei].insert(zonesIndices[zonei]);
        }
    }
}


// ************************************************************************* //
