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

#include "sphereRandom.H"
#include "meshSearch.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace sampledSets
{
    defineTypeNameAndDebug(sphereRandom, 0);
    addToRunTimeSelectionTable(sampledSet, sphereRandom, word);
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::sampledSets::sphereRandom::calcSamples
(
    DynamicList<point>& samplingPositions,
    DynamicList<scalar>&,
    DynamicList<label>& samplingSegments,
    DynamicList<label>& samplingCells,
    DynamicList<label>& samplingFaces
) const
{
    const meshSearch& searchEngine = meshSearch::New(mesh());

    randomGenerator rndGen(261782, true);

    for (label i = 0; i < nPoints_; ++ i)
    {
        // Request all random numbers simultaneously on all processors so that
        // the generator state stays consistent

        vector dpt(vector::uniform(radius_));
        while (magSqr(dpt) > sqr(radius_))
        {
            dpt = 2*radius_*(rndGen.sample01<vector>() - vector::uniform(0.5));
        }

        const point pt = centre_ + dpt;
        const label celli = searchEngine.findCell(pt);

        if (celli != -1)
        {
            samplingPositions.append(pt);
            samplingSegments.append(i);
            samplingCells.append(celli);
            samplingFaces.append(-1);
        }
    }

    // This set is unordered. Distances have not been created.
    return false;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::sampledSets::sphereRandom::sphereRandom
(
    const word& name,
    const polyMesh& mesh,
    const dictionary& dict
)
:
    sampledSet(name, mesh, dict),
    centre_(dict.lookup("centre")),
    radius_(dict.lookup<scalar>("radius")),
    nPoints_(dict.lookup<label>("nPoints"))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::sampledSets::sphereRandom::~sphereRandom()
{}


// ************************************************************************* //
