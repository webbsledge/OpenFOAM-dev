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

Application
    extrudeMesh

Description
    Extrude mesh from existing patch or from patch read from file.

    Type of extrusion prescribed by run-time selectable model.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "polyTopoChange.H"
#include "edgeCollapser.H"
#include "perfectInterface.H"
#include "addPatchCellLayer.H"
#include "fvMesh.H"
#include "MeshedSurfaces.H"
#include "globalIndex.H"
#include "cellSet.H"
#include "systemDict.H"

#include "extrudedMesh.H"
#include "extrudeModel.H"

#include "wedge.H"
#include "wedgePolyPatch.H"
#include "planeExtrusion.H"
#include "emptyPolyPatch.H"
#include "processorPolyPatch.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

enum class extrudeSurfaceType
{
    mesh,
    patch,
    surface
};

static const NamedEnum<extrudeSurfaceType, 3> extrudeSurfaceTypeNames
{
    "mesh",
    "patch",
    "surface"
};


label findIndex(const polyBoundaryMesh& patches, const word& name)
{
    const label patchID = patches.findIndex(name);

    if (patchID == -1)
    {
        FatalErrorInFunction
            << "Cannot find patch " << name
            << " in the source mesh.\n"
            << "Valid patch names are " << patches.names()
            << exit(FatalError);
    }
    return patchID;
}


labelList findPatchZones(polyMesh& mesh, const wordList& zoneNames)
{
    labelList patchZones(zoneNames.size());

    // Add optional cellZone generation for the extrusion layer
    forAll(zoneNames, i)
    {
        const label cellZonei = mesh.cellZones().findIndex(zoneNames[i]);
        if (cellZonei >= 0)
        {
            patchZones[i] = cellZonei;
        }
        else
        {
            label nZones = mesh.cellZones().size();
            mesh.cellZones().setSize(nZones + 1);
            mesh.cellZones().set
            (
                nZones,
                new cellZone
                (
                    zoneNames[i],
                    labelList(),
                    mesh.cellZones()
                )
            );
            patchZones[i] = nZones;
        }
    }

    return patchZones;
}


labelList findPatchFaces
(
    polyMesh& mesh,
    const polyBoundaryMesh& patches,
    const wordList& patchNames,
    const labelList& patchZones,
    labelList& faceCellZones
)
{
    // Count of the number of faces in the named patches
    label n = 0;

    forAll(patchNames, i)
    {
        n += patches[findIndex(patches, patchNames[i])].size();
    }

    labelList faceLabels(n);
    faceCellZones.setSize(n);
    n = 0;

    forAll(patchNames, i)
    {
        const polyPatch& pp = patches[findIndex(patches, patchNames[i])];

        forAll(pp, j)
        {
            faceLabels[n] = pp.start()+j;
            faceCellZones[n] = patchZones[i];
            n++;
        }
    }

    return faceLabels;
}


labelList findPatchFaces
(
    const polyBoundaryMesh& patches,
    const word& name
)
{
    const polyPatch& pp = patches[findIndex(patches, name)];
    labelList faceLabels(pp.size());

    forAll(pp, i)
    {
        faceLabels[i] = pp.start() + i;
    }

    return faceLabels;
}


void updateFaceLabels(const polyTopoChangeMap& map, labelList& faceLabels)
{
    const labelList& reverseMap = map.reverseFaceMap();

    labelList newFaceLabels(faceLabels.size());
    label newI = 0;

    forAll(faceLabels, i)
    {
        label oldFacei = faceLabels[i];

        if (reverseMap[oldFacei] >= 0)
        {
            newFaceLabels[newI++] = reverseMap[oldFacei];
        }
    }
    newFaceLabels.setSize(newI);
    faceLabels.transfer(newFaceLabels);
}


void updateCellSet(const polyTopoChangeMap& map, labelHashSet& cellLabels)
{
    const labelList& reverseMap = map.reverseCellMap();

    labelHashSet newCellLabels(2*cellLabels.size());

    forAll(cellLabels, i)
    {
        label oldCelli = cellLabels[i];

        if (reverseMap[oldCelli] >= 0)
        {
            newCellLabels.insert(reverseMap[oldCelli]);
        }
    }
    cellLabels.transfer(newCellLabels);
}


template<class PatchType>
void changeFrontBackPatches
(
    polyMesh& mesh,
    const word& frontPatchName,
    const word& backPatchName
)
{
    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    label frontPatchi = findIndex(patches, frontPatchName);
    label backPatchi = findIndex(patches, backPatchName);

    DynamicList<polyPatch*> newPatches(patches.size());

    forAll(patches, patchi)
    {
        const polyPatch& pp(patches[patchi]);

        if (patchi == frontPatchi || patchi == backPatchi)
        {
            newPatches.append
            (
                new PatchType
                (
                    pp.name(),
                    pp.size(),
                    pp.start(),
                    pp.index(),
                    mesh.boundaryMesh(),
                    PatchType::typeName
                )
            );
        }
        else
        {
            newPatches.append(pp.clone(mesh.boundaryMesh()).ptr());
        }
    }

    // Edit patches
    mesh.removeBoundary();
    mesh.addPatches(newPatches, true);
}


int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "addMeshOption.H"
    #include "addDictOption.H"

    #include "setRootCase.H"
    #include "createTimeExtruded.H"

    const dictionary dict(systemDict("extrudeMeshDict", args, runTimeExtruded));

    // Point generator
    autoPtr<extrudeModel> model(extrudeModel::New(dict));

    // Whether to flip normals
    const Switch flipNormals(dict.lookup("flipNormals"));

    // What to extrude
    const extrudeSurfaceType mode = extrudeSurfaceTypeNames.read
    (
        dict.lookup("constructFrom")
    );

    // Any merging of small edges
    const scalar mergeTol(dict.lookupOrDefault<scalar>("mergeTol", 1e-4));

    Info<< "Extruding from " << extrudeSurfaceTypeNames[mode]
        << " using model " << model().type() << endl;
    if (flipNormals)
    {
        Info<< "Flipping normals before extruding" << endl;
    }
    if (mergeTol > 0)
    {
        Info<< "Collapsing edges < " << mergeTol << " of bounding box" << endl;
    }
    else
    {
        Info<< "Not collapsing any edges after extrusion" << endl;
    }
    Info<< endl;


    // Generated mesh (one of either)
    autoPtr<fvMesh> meshFromMesh;
    autoPtr<polyMesh> meshFromSurface;

    // Faces on front and back for stitching (in case of mergeFaces)
    word frontPatchName;
    labelList frontPatchFaces;
    word backPatchName;
    labelList backPatchFaces;

    // Optional added cells (get written to cellSet)
    labelHashSet addedCellsSet;

    if (mode == extrudeSurfaceType::patch || mode == extrudeSurfaceType::mesh)
    {
        if (flipNormals && mode == extrudeSurfaceType::mesh)
        {
            FatalErrorInFunction
                << "Flipping normals not supported for extrusions from mesh."
                << exit(FatalError);
        }

        fileName sourceCasePath(dict.lookup("sourceCase"));
        sourceCasePath.expand();
        fileName sourceRootDir = sourceCasePath.path();
        fileName sourceCaseDir = sourceCasePath.name();
        if (Pstream::parRun())
        {
            sourceCaseDir =
                sourceCaseDir
               /"processor" + Foam::name(Pstream::myProcNo());
        }
        wordList sourcePatches(dict.lookup("sourcePatches"));

        if (sourcePatches.size() == 1)
        {
            frontPatchName = sourcePatches[0];
        }

        wordList zoneNames(dict.lookupOrDefault("zoneNames", wordList::null()));
        if (zoneNames.size() == 1)
        {
            if (zoneNames[0] == "patchNames")
            {
                zoneNames = sourcePatches;
            }
            else
            {
                zoneNames = wordList(sourcePatches.size(), zoneNames[0]);
            }
        }
        else if (zoneNames.size() && zoneNames.size() != sourcePatches.size())
        {
            FatalErrorInFunction
                << "Number of zoneNames "
                   "does not equal the number of sourcePatches"
                << exit(FatalError);
        }

        Info<< "Extruding patches " << sourcePatches
            << " on mesh " << sourceCasePath << nl
            << endl;

        Time runTime
        (
            Time::controlDictName,
            sourceRootDir,
            sourceCaseDir
        );

        #include "createSpecifiedMeshNoChangers.H"

        const polyBoundaryMesh& patches = mesh.boundaryMesh();

        const labelList patchZones
        (
            zoneNames.size()
          ? findPatchZones(mesh, zoneNames)
          : labelList(sourcePatches.size(), -1)
        );

        labelList faceCellZones;
        const labelList patchFaces
        (
            findPatchFaces
            (
                mesh,
                patches,
                sourcePatches,
                patchZones,
                faceCellZones
            )
        );

        // Extrusion engine. Either adding to existing mesh or
        // creating separate mesh.
        addPatchCellLayer layerExtrude(mesh, mode == extrudeSurfaceType::mesh);

        if (mode == extrudeSurfaceType::patch && flipNormals)
        {
            // Cheat. Flip patch faces in mesh. This invalidates the
            // mesh (open cells) but does produce the correct extrusion.
            polyTopoChange meshMod(mesh);
            forAll(patchFaces, i)
            {
                label patchFacei = patchFaces[i];

                label patchi = patches.whichPatch(patchFacei);
                label own = mesh.faceOwner()[patchFacei];
                label nei = -1;
                if (patchi == -1)
                {
                    nei = mesh.faceNeighbour()[patchFacei];
                }

                meshMod.modifyFace
                (
                    mesh.faces()[patchFacei].reverseFace(), // modified face
                    patchFacei,                     // label of face
                    own,                            // owner
                    nei,                            // neighbour
                    true,                           // face flip
                    patchi                          // patch for face
                );
            }

            // Change the mesh. without keeping old points.
            autoPtr<polyTopoChangeMap> map = meshMod.changeMesh(mesh);

            // Update fields
            mesh.topoChange(map);
        }


        indirectPrimitivePatch extrudePatch
        (
            IndirectList<face>
            (
                mesh.faces(),
                patchFaces
            ),
            mesh.points()
        );

        // Determine extrudePatch normal
        pointField extrudePatchPointNormals
        (
            PatchTools::pointNormals(mesh, extrudePatch)
        );


        // Precalculate mesh edges for pp.edges.
        const labelList meshEdges
        (
            extrudePatch.meshEdges
            (
                mesh.edges(),
                mesh.pointEdges()
            )
        );

        // Global face indices engine
        const globalIndex globalFaces(mesh.nFaces());

        // Determine extrudePatch.edgeFaces in global numbering (so across
        // coupled patches)
        labelListList edgeGlobalFaces
        (
            addPatchCellLayer::globalEdgeFaces
            (
                mesh,
                globalFaces,
                extrudePatch
            )
        );


        // Determine what patches boundary edges need to get extruded into.
        // This might actually cause edge-connected processors to become
        // face-connected so might need to introduce new processor boundaries.
        // Calculates:
        //  - per pp.edge the patch to extrude into
        //  - any additional processor boundaries (patchToNbrProc = map from
        //    new patchID to neighbour processor)
        //  - number of new patches (nPatches)

        labelList sidePatchID;
        label nPatches;
        Map<label> nbrProcToPatch;
        Map<label> patchToNbrProc;
        addPatchCellLayer::calcSidePatch
        (
            mesh,
            globalFaces,
            edgeGlobalFaces,
            extrudePatch,

            sidePatchID,
            nPatches,
            nbrProcToPatch,
            patchToNbrProc
        );


        // Add any patches

        label nAdded = nPatches - mesh.boundaryMesh().size();
        reduce(nAdded, sumOp<label>());

        Info<< "Adding overall " << nAdded << " processor patches." << endl;

        if (nAdded > 0)
        {
            DynamicList<polyPatch*> newPatches(nPatches);
            forAll(mesh.boundaryMesh(), patchi)
            {
                newPatches.append
                (
                    mesh.boundaryMesh()[patchi].clone
                    (
                        mesh.boundaryMesh()
                    ).ptr()
                );
            }
            for
            (
                label patchi = mesh.boundaryMesh().size();
                patchi < nPatches;
                patchi++
            )
            {
                label nbrProci = patchToNbrProc[patchi];

                Pout<< "Adding patch " << patchi
                    << " between " << Pstream::myProcNo()
                    << " and " << nbrProci
                    << endl;

                newPatches.append
                (
                    new processorPolyPatch
                    (
                        0,                  // size
                        mesh.nFaces(),      // start
                        patchi,             // index
                        mesh.boundaryMesh(),// polyBoundaryMesh
                        Pstream::myProcNo(),// myProcNo
                        nbrProci            // neighbProcNo
                    )
                );
            }

            // Add patches. Do no parallel updates.
            mesh.removeFvBoundary();
            mesh.addFvPatches(newPatches, true);
        }



        // Only used for addPatchCellLayer into new mesh
        labelList exposedPatchID;
        if (mode == extrudeSurfaceType::patch)
        {
            dict.lookup("exposedPatchName") >> backPatchName;
            exposedPatchID.setSize
            (
                extrudePatch.size(),
                findIndex(patches, backPatchName)
            );
        }

        // Determine points and extrusion
        pointField layer0Points(extrudePatch.nPoints());
        pointField displacement(extrudePatch.nPoints());
        forAll(displacement, pointi)
        {
            const vector& patchNormal = extrudePatchPointNormals[pointi];

            // layer0 point
            layer0Points[pointi] = model()
            (
                extrudePatch.localPoints()[pointi],
                patchNormal,
                0
            );
            // layerN point
            point extrudePt = model()
            (
                extrudePatch.localPoints()[pointi],
                patchNormal,
                model().nLayers()
            );
            displacement[pointi] = extrudePt - layer0Points[pointi];
        }


        // Check if wedge (has layer0 different from original patch points)
        // If so move the mesh to starting position.
        if (gMax(mag(layer0Points-extrudePatch.localPoints())) > small)
        {
            Info<< "Moving mesh to layer0 points since differ from original"
                << " points - this can happen for wedge extrusions." << nl
                << endl;

            pointField newPoints(mesh.points());
            forAll(extrudePatch.meshPoints(), i)
            {
                newPoints[extrudePatch.meshPoints()[i]] = layer0Points[i];
            }
            mesh.setPoints(newPoints);
        }


        // Layers per face
        labelList nFaceLayers(extrudePatch.size(), model().nLayers());

        // Layers per point
        labelList nPointLayers(extrudePatch.nPoints(), model().nLayers());

        // Displacement for first layer
        vectorField firstLayerDisp(displacement*model().sumThickness(1));

        // Expansion ratio not used.
        scalarField ratio(extrudePatch.nPoints(), 1.0);

        // Topo change container. Either copy an existing mesh or start
        // with empty storage (number of patches only needed for checking)
        autoPtr<polyTopoChange> meshMod
        (
            (
                mode == extrudeSurfaceType::mesh
              ? new polyTopoChange(mesh)
              : new polyTopoChange(patches.size())
            )
        );

        layerExtrude.setRefinement
        (
            mesh,
            globalFaces,
            edgeGlobalFaces,

            ratio,              // expansion ratio
            extrudePatch,       // patch faces to extrude
            sidePatchID,        // if boundary edge: patch to use
            exposedPatchID,     // if new mesh: patches for exposed faces
            nFaceLayers,
            nPointLayers,
            firstLayerDisp,
            faceCellZones,
            meshMod()
        );

        // Reset points according to extrusion model
        forAll(layerExtrude.addedPoints(), pointi)
        {
            const labelList& pPoints = layerExtrude.addedPoints()[pointi];
            forAll(pPoints, pPointi)
            {
                label meshPointi = pPoints[pPointi];

                point modelPt
                (
                    model()
                    (
                        extrudePatch.localPoints()[pointi],
                        extrudePatchPointNormals[pointi],
                        pPointi+1       // layer
                    )
                );

                const_cast<DynamicList<point>&>
                (
                    meshMod().points()
                )[meshPointi] = modelPt;
            }
        }

        // Store faces on front and exposed patch (if mode=patch there are
        // only added faces so cannot used map to old faces)
        const labelListList& layerFaces = layerExtrude.layerFaces();
        backPatchFaces.setSize(layerFaces.size());
        frontPatchFaces.setSize(layerFaces.size());
        forAll(backPatchFaces, patchFacei)
        {
            backPatchFaces[patchFacei]  = layerFaces[patchFacei].first();
            frontPatchFaces[patchFacei] = layerFaces[patchFacei].last();
        }


        // Create actual mesh from polyTopoChange container
        autoPtr<polyTopoChangeMap> map = meshMod().makeMesh
        (
            meshFromMesh,
            IOobject
            (
                regionName,
                runTimeExtruded.constant(),
                runTimeExtruded,
                IOobject::NO_READ,
                IOobject::AUTO_WRITE,
                false
            ),
            mesh
        );

        layerExtrude.updateZones(meshFromMesh());

        meshFromMesh().topoChange(map);

        layerExtrude.topoChange
        (
            map(),
            identityMap(extrudePatch.size()),
            identityMap(extrudePatch.nPoints())
        );

        // Calculate face labels for front and back.
        frontPatchFaces = renumber
        (
            map().reverseFaceMap(),
            frontPatchFaces
        );
        backPatchFaces = renumber
        (
            map().reverseFaceMap(),
            backPatchFaces
        );

        // Store added cells
        if (mode == extrudeSurfaceType::mesh)
        {
            const labelListList addedCells
            (
                layerExtrude.addedCells
                (
                    meshFromMesh,
                    layerExtrude.layerFaces()
                )
            );
            forAll(addedCells, facei)
            {
                const labelList& aCells = addedCells[facei];
                forAll(aCells, i)
                {
                    addedCellsSet.insert(aCells[i]);
                }
            }
        }
    }
    else
    {
        // Read from surface
        fileName surfName(dict.lookup("surface"));
        surfName.expand();

        Info<< "Extruding surfaceMesh read from file " << surfName << nl
            << endl;

        MeshedSurface<face> fMesh(surfName);

        if (flipNormals)
        {
            Info<< "Flipping faces." << nl << endl;
            faceList& faces = const_cast<faceList&>(fMesh.faces());
            forAll(faces, i)
            {
                faces[i] = fMesh[i].reverseFace();
            }
        }

        Info<< "Extruding surface with :" << nl
                << "    points     : " << fMesh.points().size() << nl
                << "    faces      : " << fMesh.size() << nl
                << "    normals[0] : " << fMesh.faceNormals()[0]
                << nl
                << endl;

        meshFromSurface.reset
        (
            new extrudedMesh
            (
                IOobject
                (
                    extrudedMesh::defaultRegion,
                    runTimeExtruded.constant(),
                    runTimeExtruded
                ),
                fMesh,
                model()
            )
        );


        // Get the faces on front and back
        frontPatchName = "originalPatch";
        frontPatchFaces = findPatchFaces
        (
            meshFromSurface().boundaryMesh(),
            frontPatchName
        );
        backPatchName = "otherSide";
        backPatchFaces = findPatchFaces
        (
            meshFromSurface().boundaryMesh(),
            backPatchName
        );
    }


    polyMesh& mesh =
    (
        meshFromMesh.valid()
      ? meshFromMesh()
      : meshFromSurface()
    );


    const boundBox& bb = mesh.bounds();
    const vector span = bb.span();
    const scalar mergeDim = mergeTol * bb.minDim();

    Info<< "Mesh bounding box : " << bb << nl
        << "        with span : " << span << nl
        << "Merge distance    : " << mergeDim << nl
        << endl;


    // Collapse edges
    // ~~~~~~~~~~~~~~

    if (mergeDim > 0)
    {
        Pout<< "Collapsing edges < " << mergeDim << " ..." << nl << endl;

        // Edge collapsing engine
        edgeCollapser collapser(mesh);

        const edgeList& edges = mesh.edges();
        const pointField& points = mesh.points();

        PackedBoolList collapseEdge(mesh.nEdges());
        Map<point> collapsePointToLocation(mesh.nPoints());

        forAll(edges, edgeI)
        {
            const edge& e = edges[edgeI];

            scalar d = e.mag(points);

            if (d < mergeDim)
            {
                Pout<< "Merging edge " << e << " since length " << d
                    << " << " << mergeDim << nl;

                collapseEdge[edgeI] = true;
                collapsePointToLocation.set(e[1], points[e[0]]);
            }
        }

        List<pointEdgeCollapse> allPointInfo;
        const globalIndex globalPoints(mesh.nPoints());
        labelList pointPriority(mesh.nPoints(), 0);

        collapser.consistentCollapse
        (
            globalPoints,
            pointPriority,
            collapsePointToLocation,
            collapseEdge,
            allPointInfo
        );

        // Topo change container
        polyTopoChange meshMod(mesh);

        // Put all modifications into meshMod
        bool anyChange = collapser.setRefinement(allPointInfo, meshMod);
        reduce(anyChange, orOp<bool>());

        if (anyChange)
        {
            // Construct new mesh from polyTopoChange.
            autoPtr<polyTopoChangeMap> map = meshMod.changeMesh(mesh);

            // Update fields
            mesh.topoChange(map);

            // Update stored data
            updateFaceLabels(map(), frontPatchFaces);
            updateFaceLabels(map(), backPatchFaces);
            updateCellSet(map(), addedCellsSet);
        }
    }


    // Change the front and back patch types as required
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    word frontBackType(word::null);

    if (isType<extrudeModels::wedge>(model()))
    {
        changeFrontBackPatches<wedgePolyPatch>
        (
            mesh,
            frontPatchName,
            backPatchName
        );
    }
    else if (isType<extrudeModels::plane>(model()))
    {
        changeFrontBackPatches<emptyPolyPatch>
        (
            mesh,
            frontPatchName,
            backPatchName
        );
    }


    // Merging front and back patch faces
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Switch mergeFaces(dict.lookup("mergeFaces"));
    if (mergeFaces)
    {
        if (mode == extrudeSurfaceType::mesh)
        {
            FatalErrorInFunction
                << "Cannot stitch front and back of extrusion since"
                << " in 'mesh' mode (extrusion appended to mesh)."
                << exit(FatalError);
        }

        Info<< "Assuming full 360 degree axisymmetric case;"
            << " stitching faces on patches "
            << frontPatchName << " and "
            << backPatchName << " together ..." << nl << endl;

        if (frontPatchFaces.size() != backPatchFaces.size())
        {
            FatalErrorInFunction
                << "Differing number of faces on front ("
                << frontPatchFaces.size() << ") and back ("
                << backPatchFaces.size() << ")"
                << exit(FatalError);
        }

        // Add the perfect interface mesh modifier
        perfectInterface perfectStitcher
        (
            "couple",
            mesh,
            word::null,         // dummy patch name
            word::null          // dummy patch name
        );

        // Topo change container
        polyTopoChange meshMod(mesh);

        perfectStitcher.setRefinement
        (
            indirectPrimitivePatch
            (
                IndirectList<face>
                (
                    mesh.faces(),
                    frontPatchFaces
                ),
                mesh.points()
            ),
            indirectPrimitivePatch
            (
                IndirectList<face>
                (
                    mesh.faces(),
                    backPatchFaces
                ),
                mesh.points()
            ),
            meshMod
        );

        // Construct new mesh from polyTopoChange.
        autoPtr<polyTopoChangeMap> map = meshMod.changeMesh(mesh);

        // Update fields
        mesh.topoChange(map);

        // Update local data
        updateCellSet(map(), addedCellsSet);
    }

    mesh.setInstance(runTimeExtruded.constant());
    Info<< "Writing mesh to " << mesh.relativeObjectPath() << nl << endl;

    if (!mesh.write())
    {
        FatalErrorInFunction
            << exit(FatalError);
    }

    // Need writing cellSet
    label nAdded = returnReduce(addedCellsSet.size(), sumOp<label>());
    if (nAdded > 0)
    {
        cellSet addedCells(mesh, "addedCells", addedCellsSet);
        Info<< "Writing added cells to cellSet " << addedCells.name()
            << nl << endl;
        if (!addedCells.write())
        {
            FatalErrorInFunction
                << addedCells.name()
                << exit(FatalError);
        }
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
