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
    Foam::sampledSurfaces::thresholdCellFaces

Description
    A sampledSurface defined by the cell faces corresponding to a threshold
    value.

    Example:
    \verbatim
    {
        type        thresholdCellFaces;
        field       p;
        lowerLimit  5;
        upperLimit  10;
        interpolate yes;
    }
    \endverbatim

Usage
    \table
        Property     | Description             | Required    | Default value
        field        | the field to threshold  | yes         |
        lowerLimit   | the limit below which cells are omitted \\
                     | if no upperLimit | -vGreat
        upperLimit   | the limit above which cells are omitted \\
                     | if no lowerLimit | +vGreat
        interpolate  | interpolate values to the surface points | no | no
    \endtable

SourceFiles
    sampledThresholdCellFaces.C

\*---------------------------------------------------------------------------*/

#ifndef sampledThresholdCellFaces_H
#define sampledThresholdCellFaces_H

#include "sampledSurface.H"
#include "MeshedSurface.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace sampledSurfaces
{

/*---------------------------------------------------------------------------*\
                     Class thresholdCellFaces Declaration
\*---------------------------------------------------------------------------*/

class thresholdCellFaces
:
    public sampledSurface,
    public MeshedSurface<face>
{
    // Private Data

        //- Field to get threshold
        const word fieldName_;

        //- Threshold value
        const scalar lowerThreshold_;

        //- Threshold value
        const scalar upperThreshold_;

        //- Triangulated faces or keep faces as is
        bool triangulate_;

        // Recreated for every time-step

        //- Time at last call, also track it surface needs an update
        mutable label prevTimeIndex_;

        //- For every face the original cell in mesh
        mutable labelList meshCells_;


    // Private Member Functions

        //- Create surface (if time has changed)
        //  Do nothing (and return false) if no update was needed
        bool updateGeometry() const;

        //- Sample field on faces
        template<class Type>
        tmp<Field<Type>> sampleField
        (
            const VolField<Type>& vField
        ) const;


        template<class Type>
        tmp<Field<Type>>
        interpolateField(const interpolation<Type>&) const;


public:

    //- Runtime type information
    TypeName("thresholdCellFaces");


    // Constructors

        //- Construct from dictionary
        thresholdCellFaces
        (
            const word& name,
            const polyMesh&,
            const dictionary&
        );


    //- Destructor
    virtual ~thresholdCellFaces();


    // Member Functions

        //- Return the list of fields required
        virtual wordList fields() const;

        //- Does the surface need an update?
        virtual bool needsUpdate() const;

        //- Mark the surface as needing an update.
        //  May also free up unneeded data.
        //  Return false if surface was already marked as expired.
        virtual bool expire();

        //- Update the surface as required.
        //  Do nothing (and return false) if no update was needed
        virtual bool update();

        //- Points of surface
        virtual const pointField& points() const
        {
            return MeshedSurface<face>::points();
        }

        //- Faces of surface
        virtual const faceList& faces() const
        {
            return MeshedSurface<face>::faces();
        }

        //- Sample field on surface
        virtual tmp<scalarField> sample(const volScalarField&) const;

        //- Sample field on surface
        virtual tmp<vectorField> sample( const volVectorField&) const;

        //- Sample field on surface
        virtual tmp<sphericalTensorField> sample
        (
            const volSphericalTensorField&
        ) const;

        //- Sample field on surface
        virtual tmp<symmTensorField> sample(const volSymmTensorField&) const;

        //- Sample field on surface
        virtual tmp<tensorField> sample
        (
            const volTensorField&
        ) const;

        //- Interpolate field on surface
        virtual tmp<scalarField> interpolate
        (
            const interpolation<scalar>&
        ) const;

        //- Interpolate field on surface
        virtual tmp<vectorField> interpolate
        (
            const interpolation<vector>&
        ) const;

        //- Interpolate field on surface
        virtual tmp<sphericalTensorField> interpolate
        (
            const interpolation<sphericalTensor>&
        ) const;

        //- Interpolate field on surface
        virtual tmp<symmTensorField> interpolate
        (
            const interpolation<symmTensor>&
        ) const;

        //- Interpolate field on surface
        virtual tmp<tensorField> interpolate
        (
            const interpolation<tensor>&
        ) const;

        //- Write
        virtual void print(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace sampledSurfaces
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "sampledThresholdCellFacesTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
