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

Class
    Foam::polyMeshAdder

Description
    Adds two meshes without using any polyMesh morphing.

    Gets faces to couple as faceCoupleInfo which is list of faces on both
    meshes. Returns map from last mesh addition.

SourceFiles
    polyMeshAdder.C

\*---------------------------------------------------------------------------*/

#ifndef polyMeshAdder_H
#define polyMeshAdder_H

#include "autoPtr.H"
#include "polyMesh.H"
#include "mapAddedPolyMesh.H"
#include "faceCoupleInfo.H"
#include "SortableList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of classes
class IOobject;
class polyTopoChange;

/*---------------------------------------------------------------------------*\
                        Class polyMeshAdder Declaration
\*---------------------------------------------------------------------------*/

class polyMeshAdder
{
private:

    // Private Member Functions

        //- Index of patch in allPatches. Add if nonexisting.
        static label patchIndex
        (
            const polyPatch&,
            DynamicList<word>& allPatchNames,
            DynamicList<word>& allPatchTypes
        );

        //- Index of zone in all zones
        static label zoneIndex(const word&, DynamicList<word>&);

        //- ...
        static void mergePatchNames
        (
            const polyBoundaryMesh& patches0,
            const polyBoundaryMesh& patches1,
            DynamicList<word>& allPatchNames,
            DynamicList<word>& allPatchTypes,
            labelList& from1ToAllPatches,
            labelList& fromAllTo1Patches
        );

        //- Get starts of patches
        static labelList getPatchStarts(const polyBoundaryMesh&);

        //- Get sizes of patches
        static labelList getPatchSizes(const polyBoundaryMesh&);

        //- Determine order for internalFaces to be upper-triangular.
        //  Does not change order of external faces.
        static labelList getFaceOrder
        (
            const cellList& cells,
            const label nInternalFaces,
            const labelList& owner,
            const labelList& neighbour
        );

        //- Merges primitives of two meshes.
        static void mergePrimitives
        (
            const polyMesh& mesh0,
            const polyMesh& mesh1,
            const faceCoupleInfo& coupleInfo,

            const label nAllPatches,
            const labelList& fromAllTo1Patches,
            const labelList& from1ToAllPatches,

            pointField& allPoints,
            labelList& from0ToAllPoints,
            labelList& from1ToAllPoints,

            faceList& allFaces,
            labelList& allOwner,
            labelList& allNeighbour,
            label& nInternalFaces,
            labelList& nFacesPerPatch,
            label& nCells,

            labelList& from0ToAllFaces,
            labelList& from1ToAllFaces,
            labelList& from1ToAllCells
        );

        //- Merge point zones
        static void mergePointZones
        (
            const label nAllPoints,

            const pointZoneList& pz0,
            const pointZoneList& pz1,
            const labelList& from0ToAllPoints,
            const labelList& from1ToAllPoints,

            DynamicList<word>& zoneNames,
            labelList& from1ToAll,
            List<DynamicList<label>>& pzPoints
        );

        //- Merge face zones
        static void mergeFaceZones
        (
            const labelList& allOwner,

            const polyMesh& mesh0,
            const polyMesh& mesh1,
            const labelList& from0ToAllFaces,
            const labelList& from1ToAllFaces,
            const labelList& from1ToAllCells,

            DynamicList<word>& zoneNames,
            labelList& from1ToAll,
            List<DynamicList<label>>& fzFaces,
            boolList& fzOrientations,
            List<DynamicList<bool>>& fzFlips
        );

        //- Merge cell zones
        static void mergeCellZones
        (
            const label nAllCells,

            const cellZoneList& cz0,
            const cellZoneList& cz1,
            const labelList& from1ToAllCells,

            DynamicList<word>& zoneNames,
            labelList& from1ToAll,
            List<DynamicList<label>>& czCells
        );

        //- Merge point/face/cell zone information
        static void mergeZones
        (
            const label nAllPoints,
            const labelList& allOwner,
            const label nAllCells,

            const polyMesh& mesh0,
            const polyMesh& mesh1,
            const labelList& from0ToAllPoints,
            const labelList& from0ToAllFaces,
            const labelList& from1ToAllPoints,
            const labelList& from1ToAllFaces,
            const labelList& from1ToAllCells,

            DynamicList<word>& pointZoneNames,
            List<DynamicList<label>>& pzPoints,

            DynamicList<word>& faceZoneNames,
            List<DynamicList<label>>& fzFaces,
            boolList& fzOrientations,
            List<DynamicList<bool>>& fzFlips,

            DynamicList<word>& cellZoneNames,
            List<DynamicList<label>>& czCells
        );

        //- Create new zones and add to new mesh.
        static void addZones
        (
            const DynamicList<word>& pointZoneNames,
            const List<DynamicList<label>>& pzPoints,

            const DynamicList<word>& faceZoneNames,
            const List<DynamicList<label>>& fzFaces,
            const boolList& fzOrientations,
            const List<DynamicList<bool>>& fzFlips,

            const DynamicList<word>& cellZoneNames,
            const List<DynamicList<label>>& czCells,

            polyMesh& mesh
        );


public:

    // Member Functions

        //- Inplace add mesh to polyMesh. Returns map construct.
        static autoPtr<mapAddedPolyMesh> add
        (
            polyMesh& mesh0,
            const polyMesh& mesh1,
            const faceCoupleInfo& coupleInfo,
            const bool validBoundary = true
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
