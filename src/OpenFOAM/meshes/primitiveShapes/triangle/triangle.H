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
    Foam::triangle

Description
    A triangle primitive used to calculate face areas and swept volumes.

SourceFiles
    triangleI.H

\*---------------------------------------------------------------------------*/

#ifndef triangle_H
#define triangle_H

#include "intersection.H"
#include "vector.H"
#include "tensor.H"
#include "pointHit.H"
#include "randomGenerator.H"
#include "FixedList.H"
#include "UList.H"
#include "linePointRef.H"
#include "barycentric2D.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class Istream;
class Ostream;

// Forward declaration of friend functions and operators

template<class Point, class PointRef> class triangle;

template<class Point, class PointRef>
inline Istream& operator>>
(
    Istream&,
    triangle<Point, PointRef>&
);

template<class Point, class PointRef>
inline Ostream& operator<<
(
    Ostream&,
    const triangle<Point, PointRef>&
);


/*---------------------------------------------------------------------------*\
                          Class triangle Declaration
\*---------------------------------------------------------------------------*/

template<class Point, class PointRef>
class triangle
{
    // Private Data

        PointRef a_, b_, c_;


public:

    //- Return types for classify
    enum proxType
    {
        NONE,
        POINT,  // Close to point
        EDGE    // Close to edge
    };


    // Constructors

        //- Construct from three points
        inline triangle(const Point& a, const Point& b, const Point& c);

        //- Construct from three points in the list of points
        //  The indices could be from triFace etc.
        inline triangle
        (
            const UList<Point>&,
            const FixedList<label, 3>& indices
        );

        //- Construct from Istream
        inline triangle(Istream&);


    // Member Functions

        // Access

            //- Return first vertex
            inline const Point& a() const;

            //- Return second vertex
            inline const Point& b() const;

            //- Return third vertex
            inline const Point& c() const;


        // Properties

            //- Return centre (centroid)
            inline Point centre() const;

            //- Return vector area
            inline vector area() const;

            //- Return scalar magnitude
            inline scalar mag() const;

            //- Return unit normal
            inline vector normal() const;

            //- Return the circum centre and radius
            inline Tuple2<Point, scalar> circumCircle() const;

            //- Return quality: Ratio of triangle and circum-circle
            //  area, scaled so that an equilateral triangle has a
            //  quality of 1
            inline scalar quality() const;

            //- Return swept-volume
            inline scalar sweptVol(const triangle& t) const;

            //- Return the inertia tensor, with optional reference
            //  point and density specification
            inline symmTensor inertia
            (
                PointRef refPt = vector::zero,
                scalar density = 1.0
            ) const;

            //- Return a random point on the triangle from a uniform
            //  distribution
            inline Point randomPoint(randomGenerator& rndGen) const;

            //- Calculate the point from the given barycentric coordinates.
            inline Point barycentricToPoint(const barycentric2D& bary) const;

            //- Calculate the barycentric coordinates from the given point
            inline barycentric2D pointToBarycentric(const point& pt) const;

            //- Calculate the barycentric coordinates from the given point.
            //  Returns the determinant.
            inline scalar pointToBarycentric
            (
                const point& pt,
                barycentric2D& bary
            ) const;

            //- Return point intersection with a ray.
            //  For a hit, the distance is signed. Positive number
            //  represents the point in front of triangle.
            //  In case of miss pointHit is set to nearest point
            //  on triangle and its distance to the distance between
            //  the original point and the plane intersection point
            inline pointHit ray
            (
                const point& p,
                const vector& q,
                const intersection::algorithm =
                    intersection::algorithm::fullRay,
                const intersection::direction dir =
                    intersection::direction::vector
            ) const;

            //- Fast intersection with a ray.
            //  For a hit, the pointHit.distance() is the line parameter t :
            //  intersection=p+t*q. Only defined for algorithm::visible,
            //  algorithm::fullRay or algorithm::halfRay. tol increases the
            //  virtual size of the triangle by a relative factor.
            inline pointHit intersection
            (
                const point& p,
                const vector& q,
                const intersection::algorithm alg,
                const scalar tol = 0.0
            ) const;

            //- Find the nearest point to p on the triangle and classify it:
            //  + near point (nearType=POINT, nearLabel=0, 1, 2)
            //  + near edge (nearType=EDGE, nearLabel=0, 1, 2)
            //    Note: edges are counted from starting
            //    vertex so e.g. edge 2 is from f[2] to f[0]
            pointHit nearestPointClassify
            (
                const point& p,
                label& nearType,
                label& nearLabel
            ) const;

            //- Return nearest point to p on triangle
            inline pointHit nearestPoint(const point& p) const;

            //- Classify nearest point to p in triangle plane
            //  w.r.t. triangle edges and points.  Returns inside
            //  (true)/outside (false).
            bool classify
            (
                const point& p,
                label& nearType,
                label& nearLabel
            ) const;

            //- Return nearest point to line on triangle. Returns hit if
            //  point is inside triangle. Sets edgePoint to point on edge
            //  (hit if nearest is inside line)
            inline pointHit nearestPoint
            (
                const linePointRef& edge,
                pointHit& edgePoint
            ) const;


    // IOstream Operators

        friend Istream& operator>> <Point, PointRef>
        (
            Istream&,
            triangle&
        );

        friend Ostream& operator<< <Point, PointRef>
        (
            Ostream&,
            const triangle&
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "triangleI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
