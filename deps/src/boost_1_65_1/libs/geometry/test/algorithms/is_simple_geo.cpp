// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2014-2017, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_TEST_MODULE
#define BOOST_TEST_MODULE test_is_simple_geo
#endif

#include "test_is_simple.hpp"


typedef bg::model::point<double, 2, bg::cs::geographic<bg::degree> >  point_type;
typedef bg::model::segment<point_type>                  segment_type;
typedef bg::model::linestring<point_type>               linestring_type;
typedef bg::model::multi_linestring<linestring_type>    multi_linestring_type;
// ccw open and closed polygons
typedef bg::model::polygon<point_type,false,false>      open_ccw_polygon_type;
typedef bg::model::polygon<point_type,false,true>       closed_ccw_polygon_type;
// multi-geometries
typedef bg::model::multi_point<point_type>              multi_point_type;
typedef bg::model::multi_polygon<open_ccw_polygon_type> multi_polygon_type;
// box
typedef bg::model::box<point_type>                      box_type;


BOOST_AUTO_TEST_CASE( test_is_simple_geo_linestring )
{
    typedef linestring_type G;

    bg::strategy::intersection::geographic_segments<> s;

    test_simple_s(from_wkt<G>("LINESTRING(0 0, -90 0, 90 0)"), s, true);
    test_simple_s(from_wkt<G>("LINESTRING(0 90, -90 0, 90 0)"), s, false);
    test_simple_s(from_wkt<G>("LINESTRING(0 90, -90 50, 90 0)"), s, false);
    test_simple_s(from_wkt<G>("LINESTRING(0 90, -90 -50, 90 0)"), s, true);

    test_simple_s(from_wkt<G>("LINESTRING(35 0, 110 36, 159 0, 82 30)"), s, false);
    test_simple_s(from_wkt<G>("LINESTRING(135 0, -150 36, -101 0, -178 30)"), s, false);
    test_simple_s(from_wkt<G>("LINESTRING(45 0, 120 36, 169 0, 92 30)"), s, false);
    test_simple_s(from_wkt<G>("LINESTRING(179 0, -179 1, -179 0, 179 1)"), s, false);
}

BOOST_AUTO_TEST_CASE( test_is_simple_geo_multilinestring )
{
    typedef multi_linestring_type G;

    bg::strategy::intersection::geographic_segments<> s;

    test_simple_s(from_wkt<G>("MULTILINESTRING((35 0, 110 36),(159 0, 82 30))"), s, false);
    test_simple_s(from_wkt<G>("MULTILINESTRING((135 0, -150 36),(-101 0, -178 30))"), s, false);
    test_simple_s(from_wkt<G>("MULTILINESTRING((45 0, 120 36),(169 0, 92 30))"), s, false);
    test_simple_s(from_wkt<G>("MULTILINESTRING((179 0, -179 1),(-179 0, 179 1))"), s, false);
}
