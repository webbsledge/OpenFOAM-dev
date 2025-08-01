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

#include "NoInjection.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class CloudType>
Foam::NoInjection<CloudType>::NoInjection
(
    const dictionary&,
    CloudType& owner,
    const word&
)
:
    InjectionModel<CloudType>(owner)
{}


template<class CloudType>
Foam::NoInjection<CloudType>::NoInjection(const NoInjection<CloudType>& im)
:
    InjectionModel<CloudType>(im.owner_)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class CloudType>
Foam::NoInjection<CloudType>::~NoInjection()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class CloudType>
Foam::scalar Foam::NoInjection<CloudType>::timeEnd() const
{
    return 0;
}


template<class CloudType>
Foam::scalar Foam::NoInjection<CloudType>::nParcelsToInject
(
    const scalar,
    const scalar
)
{
    return 0;
}


template<class CloudType>
Foam::scalar Foam::NoInjection<CloudType>::massToInject
(
    const scalar,
    const scalar
)
{
    return 0;
}


template<class CloudType>
void Foam::NoInjection<CloudType>::setPositionAndCell
(
    const meshSearch&,
    const label,
    const label,
    const scalar,
    barycentric&,
    label&,
    label&,
    label&,
    label&
)
{}


template<class CloudType>
void Foam::NoInjection<CloudType>::setProperties
(
    const label,
    const label,
    const scalar,
    typename CloudType::parcelType::trackingData& td,
    typename CloudType::parcelType& parcel
)
{
    // set particle velocity
    parcel.U() = Zero;

    // set particle diameter
    parcel.d() = 0;
}


template<class CloudType>
bool Foam::NoInjection<CloudType>::fullyDescribed() const
{
    return false;
}


// ************************************************************************* //
