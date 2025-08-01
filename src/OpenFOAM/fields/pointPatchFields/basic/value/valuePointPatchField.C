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

#include "valuePointPatchField.H"
#include "fieldMapper.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class Type>
void Foam::valuePointPatchField<Type>::checkFieldSize() const
{
    if (this->size() != this->patch().size())
    {
        FatalErrorInFunction
            << "field does not correspond to patch. " << endl
            << "Field size: " << size() << " patch size: "
            << this->patch().size()
            << abort(FatalError);
    }
}


// * * * * * * * * * * * * * * * * Constructors * * * * * * * * * * * * * * * //

template<class Type>
Foam::valuePointPatchField<Type>::valuePointPatchField
(
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF
)
:
    pointPatchField<Type>(p, iF),
    Field<Type>(p.size())
{}


template<class Type>
Foam::valuePointPatchField<Type>::valuePointPatchField
(
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF,
    const dictionary& dict,
    const bool valueRequired
)
:
    pointPatchField<Type>(p, iF, dict),
    Field<Type>(p.size())
{
    if (dict.found("value"))
    {
        Field<Type>::operator=
        (
            Field<Type>("value", iF.dimensions(), dict, p.size())
        );
    }
    else if (!valueRequired)
    {
        Field<Type>::operator=(Zero);
    }
    else
    {
        FatalIOErrorInFunction(dict)
            << "Essential entry 'value' missing"
            << exit(FatalIOError);
    }
}


template<class Type>
Foam::valuePointPatchField<Type>::valuePointPatchField
(
    const valuePointPatchField<Type>& ptf,
    const pointPatch& p,
    const DimensionedField<Type, pointMesh>& iF,
    const fieldMapper& mapper
)
:
    pointPatchField<Type>(ptf, p, iF, mapper),
    Field<Type>(mapper(ptf))
{}


template<class Type>
Foam::valuePointPatchField<Type>::valuePointPatchField
(
    const valuePointPatchField<Type>& ptf,
    const DimensionedField<Type, pointMesh>& iF
)
:
    pointPatchField<Type>(ptf, iF),
    Field<Type>(ptf)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::valuePointPatchField<Type>::map
(
    const pointPatchField<Type>& ptf,
    const fieldMapper& mapper
)
{
    const valuePointPatchField<Type>& vptf =
        refCast<const valuePointPatchField<Type>>(ptf);

    mapper(*this, vptf);
}


template<class Type>
void Foam::valuePointPatchField<Type>::reset
(
    const pointPatchField<Type>& ptf
)
{
    Field<Type>::reset(refCast<const valuePointPatchField<Type>>(ptf));
}


template<class Type>
void Foam::valuePointPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    // Get internal field to insert values into
    Field<Type>& iF = const_cast<Field<Type>&>(this->primitiveField());

    this->setInternalField(iF, *this);

    pointPatchField<Type>::updateCoeffs();
}


template<class Type>
void Foam::valuePointPatchField<Type>::evaluate(const Pstream::commsTypes)
{
    // Get internal field to insert values into
    Field<Type>& iF = const_cast<Field<Type>&>(this->primitiveField());

    this->setInternalField(iF, *this);

    pointPatchField<Type>::evaluate();
}


template<class Type>
void Foam::valuePointPatchField<Type>::write(Ostream& os) const
{
    pointPatchField<Type>::write(os);
    writeEntry(os, "value", static_cast<const Field<Type>&>(*this));
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class Type>
void Foam::valuePointPatchField<Type>::operator=
(
    const valuePointPatchField<Type>& ptf
)
{
    Field<Type>::operator=(ptf);
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator=
(
    const pointPatchField<Type>& ptf
)
{
    Field<Type>::operator=(this->patchInternalField());
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator=
(
    pointPatchField<Type>&& ptf
)
{
    Field<Type>::operator=(this->patchInternalField());
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator=
(
    const UList<Type>& ul
)
{
    Field<Type>::operator=(ul);
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator=
(
    const Type& t
)
{
    Field<Type>::operator=(t);
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator==
(
    const valuePointPatchField<Type>& ptf
)
{
    Field<Type>::operator=(ptf);
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator==
(
    const pointPatchField<Type>& ptf
)
{
    Field<Type>::operator=(this->patchInternalField());
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator==
(
    const Field<Type>& tf
)
{
    Field<Type>::operator=(tf);
}


template<class Type>
void Foam::valuePointPatchField<Type>::operator==
(
    const Type& t
)
{
    Field<Type>::operator=(t);
}


// ************************************************************************* //
