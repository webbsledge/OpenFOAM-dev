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
    Foam::vtkPVFoam

Description
    Provides a reader interface for OpenFOAM to VTK interaction.

SourceFiles
    vtkPVFoam.C
    vtkPVFoam.H
    vtkPVFoamFields.C
    vtkPVFoamMesh.C
    vtkPVFoamMeshLagrangian.C
    vtkPVFoamTemplates.C
    vtkPVFoamMeshSet.C
    vtkPVFoamMeshVolume.C
    vtkPVFoamMeshZone.C
    vtkPVFoamSurfaceField.H
    vtkPVFoamLagrangianFields.H
    vtkPVFoamPatchField.H
    vtkPVFoamPointFields.H
    vtkPVFoamPoints.H
    vtkPVFoamUpdateInfo.C
    vtkPVFoamUtils.C
    vtkPVFoamVolFields.H
    vtkPVFoamAddToSelection.H

\*---------------------------------------------------------------------------*/

#ifndef vtkPVFoam_H
#define vtkPVFoam_H

// do not include legacy strstream headers
#ifndef  VTK_EXCLUDE_STRSTREAM_HEADERS
    #define VTK_EXCLUDE_STRSTREAM_HEADERS
#endif

#include "className.H"
#include "fileName.H"
#include "stringList.H"
#include "wordList.H"
#include "primitivePatch.H"
#include "PrimitivePatchInterpolation.H"
#include "dictionary.H"
#include "PtrList.H"
#include "HashSet.H"
#include "IOField.H"
#include "volFieldsFwd.H"
#include "surfaceFieldsFwd.H"
#include "pointFieldsFwd.H"

// * * * * * * * * * * * * * Forward Declarations  * * * * * * * * * * * * * //

class vtkDataArraySelection;
class vtkDataSet;
class vtkPoints;
class vtkPVFoamReader;
class vtkRenderer;
class vtkTextActor;
class vtkMultiBlockDataSet;
class vtkPolyData;
class vtkUnstructuredGrid;
class vtkIndent;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// OpenFOAM class forward declarations
class argList;
class Time;
class processorRunTimes;
class domainDecomposition;
class fvMesh;
class IOobjectList;
class polyPatch;
class faceSet;
class pointSet;
class fvFieldReconstructor;
class pointFieldReconstructor;
class lagrangianFieldReconstructor;
class LagrangianMesh;
class LagrangianFieldReconstructor;

/*---------------------------------------------------------------------------*\
                          Class vtkPVFoam Declaration
\*---------------------------------------------------------------------------*/

class vtkPVFoam
{
    // Private classes

        //- Bookkeeping for GUI checklists and the multi-block organisation
        class arrayRange
        {
            const char *name_;
            int block_;
            int start_;
            int size_;

        public:

            arrayRange(const char *name, const int blockNo=0)
            :
                name_(name),
                block_(blockNo),
                start_(0),
                size_(0)
            {}

            //- Return the block holding these datasets
            int block() const
            {
                return block_;
            }

            //- Assign block number, return previous value
            int block(int blockNo)
            {
                int prev = block_;
                block_ = blockNo;
                return prev;
            }

            //- Return block name
            const char* name() const
            {
                return name_;
            }

            //- Return array start index
            int start() const
            {
                return start_;
            }

            //- Return array end index
            int end() const
            {
                return start_ + size_;
            }

            //- Return sublist size
            int size() const
            {
                return size_;
            }

            bool empty() const
            {
                return !size_;
            }

            //- Reset the size to zero and optionally assign a new start
            void reset(const int startAt = 0)
            {
                start_ = startAt;
                size_ = 0;
            }

            //- Increment the size
            void operator+=(const int n)
            {
                size_ += n;
            }
        };

        //- Bookkeeping for polyhedral cell decomposition
        //  hide in extra pointMap (cellSet/cellZone) for now
        class polyDecomp
        {
            labelList superCells_;
            labelList addPointCellLabels_;
            labelList pointMap_;

        public:

            polyDecomp()
            {}

            //- Label of original cell for decomposed cells
            labelList& superCells()
            {
                return superCells_;
            }

            //- Label of original cell for decomposed cells
            const labelList& superCells() const
            {
                return superCells_;
            }

            //- Cell-centre labels for additional points of decomposed cells
            labelList& addPointCellLabels()
            {
                return addPointCellLabels_;
            }

            //- Cell-centre labels for additional points of decomposed cells
            const labelList& addPointCellLabels() const
            {
                return addPointCellLabels_;
            }

            //- Point labels for subsetted meshes
            labelList& pointMap()
            {
                return pointMap_;
            }

            //- Point labels for subsetted meshes
            const labelList& pointMap() const
            {
                return pointMap_;
            }


            //- Clear
            void clear()
            {
                superCells_.clear();
                addPointCellLabels_.clear();
                pointMap_.clear();
            }
        };


    // Private Data

        //- Access to the controlling vtkPVFoamReader
        vtkPVFoamReader* reader_;

        //- Configuration dictionary
        dictionary configDict_;

        //- OpenFOAM time controls
        autoPtr<processorRunTimes> procDbsPtr_;

        //- OpenFOAM meshes
        autoPtr<domainDecomposition> procMeshesPtr_;

        //- The mesh in the optional meshes sub-directory
        word meshMesh_;

        //- Path to the mesh in the optional meshes sub-directory
        word meshPath_;

        //- The mesh region
        word meshRegion_;

        //- The mesh directory
        fileName meshDir_;

        //- The time index
        int timeIndex_;

        //- Selected geometrical parts (internalMesh, patches, ...)
        boolList partStatus_;

        //- Datasets corresponding to selected geometrical pieces
        //  a negative number indicates that no vtkmesh exists for this piece
        labelList partDataset_;

        //- First instance and size of various mesh parts
        //  used to index into partStatus_ and partDataset_
        arrayRange arrayRangeVolume_;
        arrayRange arrayRangePatches_;
        arrayRange arrayRangelagrangian_;
        arrayRange arrayRangeLagrangian_;
        arrayRange arrayRangeCellZones_;
        arrayRange arrayRangeFaceZones_;
        arrayRange arrayRangePointZones_;
        arrayRange arrayRangeCellSets_;
        arrayRange arrayRangeFaceSets_;
        arrayRange arrayRangePointSets_;

        //- Decomposed cells information (mesh regions)
        List<polyDecomp> regionPolyDecomp_;

        //- Decomposed cells information (cellZones)
        List<polyDecomp> zonePolyDecomp_;

        //- Decomposed cells information (cellSets)
        List<polyDecomp> setPolyDecomp_;

        //- List of patch names for rendering to window
        List<vtkTextActor*> patchTextActorsPtrs_;

        //- Finite-volume field reconstructor
        autoPtr<fvFieldReconstructor> fvReconstructorPtr_;

        //- Point field reconstructor
        autoPtr<pointFieldReconstructor> pointReconstructorPtr_;

        //- lagrangian field reconstructors
        PtrList<lagrangianFieldReconstructor> lagrangianReconstructors_;

        //- Lagrangian meshes
        PtrList<LagrangianMesh> LagrangianMeshes_;

        //- Lagrangian field reconstructors
        PtrList<LagrangianFieldReconstructor> LagrangianReconstructors_;


    // Private Member Functions

        // Convenience method use to convert the readers from VTK 5
        // multiblock API to the current composite data infrastructure
        static void AddToBlock
        (
            vtkMultiBlockDataSet* output,
            vtkDataSet* dataset,
            const arrayRange&,
            const label datasetNo,
            const std::string& datasetName
        );

        // Convenience method use to convert the readers from VTK 5
        // multiblock API to the current composite data infrastructure
        static vtkDataSet* GetDataSetFromBlock
        (
            vtkMultiBlockDataSet* output,
            const arrayRange&,
            const label datasetNo
        );

        // Convenience method use to convert the readers from VTK 5
        // multiblock API to the current composite data infrastructure
        static label GetNumberOfDataSets
        (
            vtkMultiBlockDataSet* output,
            const arrayRange&
        );


        // Update information helper functions

            //- Update the mesh parts selected in the GUI
            void topoChangePartsStatus();

            //- Internal mesh info
            void updateInfoInternalMesh(vtkDataArraySelection*);

            //- Lagrangian info
            void updateInfolagrangian(vtkDataArraySelection*);

            //- Lagrangian info
            void updateInfoLagrangian(vtkDataArraySelection*);

            //- Patch info
            void updateInfoPatches
            (
                vtkDataArraySelection*,
                stringList&,
                const bool
            );

            //- Set info
            void updateInfoSets(vtkDataArraySelection*);

            //- Zone info
            void updateInfoZones(vtkDataArraySelection*);

            //- Get non-empty zone names for zoneType from file
            template<class ZonesType>
            wordList getZoneNames(const word& zonesName) const;

            //- Get non-empty zone names from mesh info
            template<class ZonesType>
            wordList getZoneNames(const ZonesType&) const;

            //- Add objects of Type to paraview array selection
            template<class Type>
            label addToSelection
            (
                vtkDataArraySelection*,
                const IOobjectList&,
                const string& suffix=string::null
            );

            //- Add objects of Type to paraview array selection
            template<class meshType>
            void addFieldsToSelection
            (
                vtkDataArraySelection *select,
                const IOobjectList& objects,
                const string& suffix=string::null
            );

            //- Add objects of Type to paraview array selection
            template<class meshType>
            void addInternalFieldsToSelection
            (
                vtkDataArraySelection *select,
                const IOobjectList& objects,
                const string& suffix=string::null
            );

            //- Field info
            void updateInfoFields();

            //- Lagrangian field info
            void updateInfolagrangianFields();

            //- Lagrangian field info
            void updateInfoLagrangianFields();


        // Update helper functions

            //- Clear the reconstructor objects
            void clearReconstructors();

            //- Clear the OpenFOAM mesh if "Cache Mesh" is false and/or if we
            //  have switched from reading between the reconstructed and the
            //  decomposed case.
            void clearFoamMesh();

            //- OpenFOAM mesh
            void updateFoamMesh();

            //- Reset data counters
            void resetCounters();

            //- Reduce memory footprint after conversion
            void reduceMemory();

            //- Volume fields
            void updateVolFields(vtkMultiBlockDataSet*);

            //- Point fields
            void updatePointFields(vtkMultiBlockDataSet*);

            //- Lagrangian fields
            void updatelagrangianFields(vtkMultiBlockDataSet*);

            //- Lagrangian fields
            void updateLagrangianFields(vtkMultiBlockDataSet*);


        // Mesh conversion functions

            //- Volume mesh
            void convertMeshVolume(vtkMultiBlockDataSet*, int& blockNo);

            //- Lagrangian mesh
            void convertMeshlagrangian(vtkMultiBlockDataSet*, int& blockNo);

            //- Lagrangian mesh
            void convertMeshLagrangian(vtkMultiBlockDataSet*, int& blockNo);

            //- Patches
            void convertMeshPatches(vtkMultiBlockDataSet*, int& blockNo);

            //- Cell zones
            void convertMeshCellZones(vtkMultiBlockDataSet*, int& blockNo);

            //- Face zones
            void convertMeshFaceZones(vtkMultiBlockDataSet*, int& blockNo);

            //- Point zones
            void convertMeshPointZones(vtkMultiBlockDataSet*, int& blockNo);

            //- Cell sets
            void convertMeshCellSets(vtkMultiBlockDataSet*, int& blockNo);

            //- Face sets
            void convertMeshFaceSets(vtkMultiBlockDataSet*, int& blockNo);

            //- Point sets
            void convertMeshPointSets(vtkMultiBlockDataSet*, int& blockNo);


        // Add mesh functions

            //- Add internal mesh/cell set
            vtkUnstructuredGrid* volumeVTKMesh(const fvMesh&, polyDecomp&);

            //- Add Lagrangian mesh
            vtkPolyData* lagrangianVTKMesh
            (
                const word&,
                autoPtr<lagrangianFieldReconstructor>&
            );

            //- Add Lagrangian mesh
            vtkPolyData* LagrangianVTKMesh
            (
                const word&,
                autoPtr<LagrangianMesh>&,
                autoPtr<LagrangianFieldReconstructor>&
            );

            //- Add patch mesh
            template<class PatchType>
            vtkPolyData* patchVTKMesh(const PatchType&);

            //- Add point zone
            vtkPolyData* pointZoneVTKMesh(const fvMesh&, const labelList&);

            //- Add face set
            vtkPolyData* faceSetVTKMesh(const fvMesh&, const faceSet&);

            //- Add point mesh
            vtkPolyData* pointSetVTKMesh(const fvMesh&, const pointSet&);


        // Field conversion functions

            //- Convert fields
            void convertFields(vtkMultiBlockDataSet*);

            //- Convert Lagrangian fields
            void convertlagrangianFields(vtkMultiBlockDataSet*);

            //- Convert Lagrangian fields
            void convertLagrangianFields(vtkMultiBlockDataSet*);


        //- Add the fields in the selected time directory to the selection
        //  lists
        template<class GeoField>
        label addObjectsToSelection
        (
            vtkDataArraySelection*,
            const IOobjectList&,
            const string& suffix=string::null
        );


        // Convert OpenFOAM fields

            //- Volume fields - all types
            template<class Type>
            void convertVolFields
            (
                const PtrList<PrimitivePatchInterpolation<primitivePatch>>&,
                const IOobjectList&,
                const bool interpFields,
                vtkMultiBlockDataSet* output
            );

            //- Volume internal fields - all types
            template<class Type>
            void convertVolInternalFields
            (
                const IOobjectList&,
                vtkMultiBlockDataSet* output
            );

            //- Volume field - all selected parts
            template<class Type>
            void convertVolFieldBlock
            (
                const VolField<Type>&,
                autoPtr<PointField<Type>>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const List<polyDecomp>& decompLst
            );

            //- Volume internal field - all selected parts
            template<class Type>
            void convertVolInternalFieldBlock
            (
                const typename VolField<Type>::Internal&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const List<polyDecomp>& decompLst
            );

            //- Volume field
            template<class Type>
            void convertVolInternalField
            (
                const typename VolField<Type>::Internal&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo,
                const polyDecomp&
            );

            //- Patch field
            template<class Type>
            void convertPatchField
            (
                const word& name,
                const Field<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo
            );

            //- Face set/zone field from volField
            template<class Type>
            void convertSurfaceField
            (
                const VolField<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo,
                const fvMesh&,
                const labelList& faceLabels
            );

            //- Face set/zone field from surfaceField
            template<class Type>
            void convertSurfaceField
            (
                const SurfaceField<Type>& tf,
                vtkMultiBlockDataSet* output,
                const arrayRange& range,
                const label datasetNo,
                const fvMesh& mesh,
                const labelList& faceLabels
            );

            //- Surface fields - all types
            template<class Type>
            void convertSurfaceFields
            (
                const IOobjectList& objects,
                vtkMultiBlockDataSet* output
            );

            //- Lagrangian fields - all types
            template<class Type>
            void convertlagrangianFields
            (
                const IOobjectList&,
                vtkMultiBlockDataSet* output,
                const label datasetNo
            );

            //- Lagrangian fields - all types
            template<class Type, template<class> class GeoField>
            void convertLagrangianFields
            (
                const IOobjectList&,
                vtkMultiBlockDataSet* output,
                const label datasetNo
            );

            //- Lagrangian field
            template<class Type>
            void convertlagrangianField
            (
                const IOField<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo
            );

            //- Lagrangian field
            template<class Type, template<class> class GeoField>
            void convertLagrangianField
            (
                const GeoField<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo
            );

            //- Point fields - all types
            template<class Type>
            void convertPointFields
            (
                const IOobjectList&,
                vtkMultiBlockDataSet* output
            );

            //- Point field - all selected parts
            template<class Type>
            void convertPointFieldBlock
            (
                const PointField<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const List<polyDecomp>&
            );

            //- Point fields
            template<class Type>
            void convertPointField
            (
                const PointField<Type>&,
                const VolField<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo,
                const polyDecomp&
            );

            //- Patch point field
            template<class Type>
            void convertPatchPointField
            (
                const word& name,
                const Field<Type>&,
                vtkMultiBlockDataSet* output,
                const arrayRange&,
                const label datasetNo
            );


       // GUI selection helper functions

            //- Only keep what is listed in hashSet
            static void pruneObjectList
            (
                IOobjectList&,
                const wordHashSet&
            );

            //- Get the list of selected objects
            IOobjectList getObjects
            (
                const wordHashSet& selected,
                const fvMesh& mesh,
                const fileName& instance,
                const fileName& local = ""
            );

            //- Retrieve the current selections
            static wordHashSet getSelected(vtkDataArraySelection*);

            //- Retrieve a sub-list of the current selections
            static wordHashSet getSelected
            (
                vtkDataArraySelection*,
                const arrayRange&
            );

            //- Retrieve the current selections
            static stringList getSelectedArrayEntries
            (
                vtkDataArraySelection*,
                const bool debug = vtkPVFoam::debug
            );

            //- Retrieve a sub-list of the current selections
            static stringList getSelectedArrayEntries
            (
                vtkDataArraySelection*,
                const arrayRange&,
                const bool debug = vtkPVFoam::debug
            );

            //- Set selection(s)
            static void setSelectedArrayEntries
            (
                vtkDataArraySelection*,
                const stringList&
            );

            //- Get the first word from the mesh parts selection
            word getPartName(const int);


public:

    //- Static data members

        ClassName("vtkPVFoam");


    // Constructors

        //- Construct from components
        vtkPVFoam
        (
            const char* const FileName,
            vtkPVFoamReader* reader
        );

        //- Disallow default bitwise copy construction
        vtkPVFoam(const vtkPVFoam&) = delete;


    //- Destructor
    ~vtkPVFoam();


    // Member Functions

        //- Update information for selection dialogs
        void updateInfo();

        //- Update
        void Update
        (
            vtkMultiBlockDataSet* output,
            vtkMultiBlockDataSet* lagrangianOutput,
            vtkMultiBlockDataSet* LagrangianOutput
        );

        //- Clean any storage
        void CleanUp();

        //- Allocate and return a list of selected times
        //  returns the count via the parameter
        double* findTimes(const bool first, int& nTimeSteps);

        //- Add/remove patch names to/from the view
        void renderPatchNames(vtkRenderer*, const bool show);

        //- Set the runTime to the first plausible request time,
        //  returns the timeIndex
        //  sets to "constant" on error
        int setTime(int count, const double requestTimes[]);

        //- The current time index
        int timeIndex() const
        {
            return timeIndex_;
        }


     // Access

        //- Debug information
        void PrintSelf(ostream&, vtkIndent) const;


    // Member Operators

        //- Disallow default bitwise assignment
        void operator=(const vtkPVFoam&) = delete;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "vtkPVFoamTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
