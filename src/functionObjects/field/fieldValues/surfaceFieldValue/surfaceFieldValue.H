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
    Foam::functionObjects::fieldValues::surfaceFieldValue

Description
    Surface, patch or faceZone selection class

    Given a list of user-specified fields and a selection of mesh (or general
    surface) faces, a number of operations can be performed, such as sums,
    averages and integrals.

    For example, to calculate the volumetric or mass flux across a patch,
    apply the 'orientedSum' operator to the flux field (typically \c phi)

    Examples of function object specification:
    \verbatim
    movingWallPatch
    {
        type            surfaceFieldValue;
        libs            ("libfieldFunctionObjects.so");

        log             true;
        writeControl    writeTime;
        writeFields     true;

        patch           movingWall;

        operation       areaAverage;

        fields
        (
            p
            phi
            U
        );
    }

    surfaceFieldValue1
    {
        type            surfaceFieldValue;
        libs            ("libfieldFunctionObjects.so");

        log             true;
        writeControl    writeTime;
        writeFields     true;

        surfaceFormat   none;

        faceZone        f0;

        operation       sum;

        weightField     alpha1;

        fields
        (
            p
            phi
            U
        );
    }
    \endverbatim

Usage
    \table
        Property     | Description             | Required    | Default value
        type         | type name: surfaceFieldValue   | yes         |
        log          | write data to standard output | no    | no
        writeFields  | Write the field values  | yes     |
        writeArea    | Write the area of the surfaceFieldValue | no |
        surfaceFormat | output value format    | no          |
        operation    | operation to perform    | yes         |
        weightField  | name of field to apply weighting | no |
        weightFields | Names of fields to apply weighting | no |
        fields       | list of fields to operate on | yes    |
    \endtable

    Where the supported selections are:
    \plaintable
        faceZone     | requires a 'faceZone' entry to specify the faceZone \\
                       or dictionary to specify the zoneGenerator
        patch        | requires a 'patch' entry to specify the patch
        patches      | requires a 'patches' entry to specify the patches
        sampledSurface | requires a 'sampledSurface' sub-dictionary
    \endplaintable

    The \c operation is one of:
    \plaintable
       none                | No operation
       sum                 | Sum
       sumMag              | Sum of component magnitudes
       orientedSum         | Sum with face orientations
       average             | Ensemble average
       areaAverage         | Area weighted average
       areaIntegrate       | Area integral
       min                 | Minimum
       max                 | Maximum
       minMag              | Minimum magnitude
       maxMag              | Maximum magnitude
       CoV                 | Coefficient of variation: (standard deviation)/mean
       UI                  | Uniformity index: ???
       areaNormalAverage   | Area weighted average in face normal direction
       areaNormalIntegrate | Area weighted integral in face normal direction
    \endplaintable

    Note:
      - Faces on empty patches get ignored.
      - The `oriented' operations will flip the sign of the field so that all
        the normals point in a consistent direction. This is only of relevance
        when summing mesh-oriented fields, such as the flux, on faceZones.
      - If the field is a volField then a \c faceZone can only consist of
        boundary faces, because only these faces have a value associated with
        them. No cell-to-face interpolation is performed.
      - If the field is a surfaceField then the selection cannot be a \c
        sampledSurface
      - If a sampledSurface has interpolation set to false then the surface
        face values will be taken directly from the cell that contains the
        surface face centre
      - If a \c sampledSurface has interpolation set to true then the field
        will be interpolated to the vertices, then averaged onto the surface
        faces

See also
    Foam::fieldValues
    Foam::functionObject

SourceFiles
    surfaceFieldValue.C
    surfaceFieldValueTemplates.C

\*---------------------------------------------------------------------------*/

#ifndef surfaceFieldValue_functionObject_H
#define surfaceFieldValue_functionObject_H

#include "fieldValue.H"
#include "NamedEnum.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class sampledSurface;
class surfaceWriter;

namespace functionObjects
{
namespace fieldValues
{

/*---------------------------------------------------------------------------*\
                      Class surfaceFieldValue Declaration
\*---------------------------------------------------------------------------*/

class surfaceFieldValue
:
    public fieldValue
{

public:

    // Public data types

        //- Selection type enumeration
        enum class selectionTypes
        {
            faceZone,
            patch,
            patches,
            sampledSurface
        };

        //- Selection type names
        static const NamedEnum<selectionTypes, 4> selectionTypeNames;


        //- Operation type enumeration
        enum class operationType
        {
            none,
            sum,
            sumMag,
            orientedSum,
            average,
            areaAverage,
            areaIntegrate,
            min,
            max,
            minMag,
            maxMag,
            CoV,
            UI,
            areaNormalAverage,
            areaNormalIntegrate
        };

        //- Operation type names
        static const NamedEnum<operationType, 15> operationTypeNames_;


private:

    // Private Member Functions

        //- Set faces to evaluate from a faceZone
        void setFaceZoneFaces(const faceZone& zone);

        //- Set faces to evaluate based on a list of patch indices
        void setPatchesFaces(const labelList&);

        //- Set faces to evaluate based on a list of patch names
        void setPatchesFaces(const wordReList&);

        //- Set faces to evaluate based on a patch name
        void setPatchFaces(const wordRe&);

        //- Set faces according to sampledSurface
        void setSampledSurfaceFaces(const dictionary&);

        //- Combine mesh faces and points from multiple processors
        void combineMeshGeometry
        (
            faceList& faces,
            pointField& points
        ) const;

        //- Combine surface faces and points from multiple processors
        void combineSurfaceGeometry
        (
            faceList& faces,
            pointField& points
        ) const;

        //- Calculate and return total area of the surfaceFieldValue: sum(magSf)
        scalar totalArea() const;


protected:

    // Protected data

        //- Input dictionary
        //  Cached for mesh changes
        dictionary dict_;

        //- Surface writer
        autoPtr<surfaceWriter> surfaceWriterPtr_;

        //- Selection type
        selectionTypes selectionType_;

        //- Name of face selection (patch, faceZone, etc.)
        string selectionName_;

        //- Operation to apply to values
        operationType operation_;

        //- Weight field names - optional
        wordList weightFieldNames_;

        //- Total area of the surfaceFieldValue
        scalar totalArea_;

        //- Optionally write the area of the surfaceFieldValue
        bool writeArea_;

        //- Global number of faces
        label nFaces_;


        // If operating on mesh faces (faceZone, patch)

            //- Local list of face IDs
            labelList faceId_;

            //- Local list of patch ID per face
            labelList facePatchId_;

            //- List of +1/-1 representing face flip map
            //  (1 use as is, -1 negate)
            labelList faceSign_;


        // If operating on sampledSurface

            //- Underlying sampledSurface
            autoPtr<sampledSurface> surfacePtr_;


    // Protected Member Functions

        //- Initialise, e.g. face addressing
        void initialise(const dictionary& dict);

        //- Return true if the field name is valid
        template<class Type>
        bool validField(const word& fieldName) const;

        //- Return field values by looking up field name
        template<class Type>
        tmp<Field<Type>> getFieldValues(const word& fieldName) const;

        //- Apply the operation to the values, and return true if successful.
        //  Does nothing unless overloaded below.
        template<class Type, class ResultType>
        bool processValues
        (
            const Field<Type>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            ResultType& result
        ) const;

        //- Apply Type -> Type operation to the values. Calls
        //  processValuesTypeType.
        template<class Type>
        bool processValues
        (
            const Field<Type>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            Type& result
        ) const;

        //- Apply Type -> scalar operation to the values. Tries to apply
        //  Type -> scalar specific operations, otherwise does nothing.
        template<class Type>
        bool processValues
        (
            const Field<Type>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            scalar& result
        ) const;

        //- Apply scalar -> scalar operation to the values. Tries to apply
        //  scalar -> scalar specific operations, otherwise calls
        //  processValuesTypeType.
        bool processValues
        (
            const Field<scalar>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            scalar& result
        ) const;

        //- Apply vector -> scalar operation to the values. Tries to apply
        //  vector -> scalar specific operations, otherwise does nothing.
        bool processValues
        (
            const Field<vector>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            scalar& result
        ) const;

        //- Apply a Type -> Type operation to the values
        template<class Type>
        bool processValuesTypeType
        (
            const Field<Type>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf,
            Type& result
        ) const;

        //- Output file header information
        virtual void writeFileHeader(const label i);


public:

    //- Run-time type information
    TypeName("surfaceFieldValue");


    // Constructors

        //- Construct from name, Time and dictionary
        surfaceFieldValue
        (
            const word& name,
            const Time& runTime,
            const dictionary& dict
        );

        //- Construct from name, objectRegistry and dictionary
        surfaceFieldValue
        (
            const word& name,
            const objectRegistry& obr,
            const dictionary& dict
        );


    //- Destructor
    virtual ~surfaceFieldValue();


    // Public Member Functions

        //- Templated helper function to output field values
        template<class Type>
        void writeValues
        (
            const word& fieldName,
            const Field<Type>& values,
            const scalarField& signs,
            const scalarField& weights,
            const vectorField& Sf
        );

        //- Filter a surface field according to faceIds
        template<class Type>
        tmp<Field<Type>> filterField
        (
            const SurfaceField<Type>& field
        ) const;

        //- Filter a volume field according to faceIds
        template<class Type>
        tmp<Field<Type>> filterField
        (
            const VolField<Type>& field
        ) const;

        //- Read from dictionary
        virtual bool read(const dictionary&);

        //- Calculate and write
        virtual bool write();

        //- Update for mesh point-motion
        virtual void movePoints(const polyMesh&);

        //- Update topology using the given map
        virtual void topoChange(const polyTopoChangeMap&);

        //- Update from another mesh using the given map
        virtual void mapMesh(const polyMeshMap&);

        //- Redistribute or update using the given distribution map
        virtual void distribute(const polyDistributionMap&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fieldValues
} // End namespace functionObjects
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "surfaceFieldValueTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
