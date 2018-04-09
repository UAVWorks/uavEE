////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2018 University of Illinois Board of Trustees
//
// This file is part of uavEE.
//
// uavEE is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// uavEE is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include <uavAP/Core/PropertyMapper/PropertyMapper.h>
#include "ground_station/MapLocation.h"

#include <cmath>

MapLocation::MapLocation(double east, double north) :
		n(north), e(east), zone(0), hemi('X'), lat(0), lon(0)
{
}

MapLocation
MapLocation::fromJson(const boost::property_tree::ptree &json)
{
	/*if (json.contains("lat") && json.contains("lon"))
	{
		return MapLocation::fromLatLong(json["lat"].toDouble(), json["lon"].toDouble());
	}*/
	PropertyMapper pm(json);
	double x, y;
	if(pm.add("lat",x,false)&&pm.add("lon",y,false))
		return MapLocation::fromLatLong(x,y);
	if(pm.add("utm_n",x,false)&&pm.add("utm_e",y,false))
		return MapLocation::fromUTM(x,y);
	APLOG_ERROR<<"MapLocation::fromJson called on json without valid coordinates";
	return MapLocation();
}

MapLocation
MapLocation::fromLatLong(double Lat, double Long)
{
	MapLocation ret;
	latLongToUTM(Lat, Long, &(ret.n), &(ret.e), &(ret.zone), &(ret.hemi));
	ret.lat = Lat;
	ret.lon = Long;
	return ret;
}

MapLocation
MapLocation::fromMapTileCoords(double x, double y, int z)
{
	double Lat;
	double Long;
	mapTileCoordsToLatLong(x, y, z, &Lat, &Long);
	return fromLatLong(Lat, Long);
}

MapLocation
MapLocation::fromUTM(double northing, double easting)
{
	MapLocation ret;
	ret.n = northing;
	ret.e = easting;
	return ret;
}

MapLocation
MapLocation::fromVector3(Vector3 vect)
{
	return MapLocation(vect.x(), vect.y());
}

double
MapLocation::northing() const
{
	return n;
}

double
MapLocation::easting() const
{
	return e;
}

double
MapLocation::x(int z) const
{
	return ((lon + 180) / 360.0) * pow(2, z);
}

double
MapLocation::y(int z) const
{
	double latRad = lat * M_PI / 180.0;
	return (1 - (log(tan(latRad) + (1 / cos(latRad))) / M_PI)) * pow(2, z - 1);
}

double
MapLocation::latitude() const
{
	return lat;
}

double
MapLocation::longitude() const
{
	return lon;
}

void
MapLocation::latLongToUTM(double Lat, double Long, double *northing, double *easting, int *zone,
		char *hemi)
{
	/*% -------------------------------------------------------------------------
	 % [x,y,utmzone] = wgs2utm(Lat,Lon,Zone)
	 %
	 % Description:
	 %    Convert WGS84 coordinates (Latitude, Longitude) into UTM coordinates
	 %    (northing, easting) according to (optional) input UTM zone and
	 %    hemisphere.
	 %
	 % Input:
	 %    Lat: WGS84 Latitude scalar, vector or array in decimal degrees.
	 %    Lon: WGS84 Longitude scalar, vector or array in decimal degrees.
	 %    utmzone (optional): UTM longitudinal zone. Scalar or same size as Lat
	 %       and Lon.
	 %    utmhemi (optional): UTM hemisphere as a single character, 'N' or 'S',
	 %       or array of 'N' or 'S' characters of same size as Lat and Lon.
	 %
	 % Output:
	 %    x: UTM easting in meters.
	 %    y: UTM northing in meters.
	 %    utmzone: UTM longitudinal zone.
	 %    utmhemi: UTM hemisphere as array of 'N' or 'S' characters.
	 %
	 % Author notes:
	 %    I downloaded and tried deg2utm.m from Rafael Palacios but found
	 %    differences of up to 1m with my reference converters in southern
	 %    hemisphere so I wrote my own code based on "Map Projections - A
	 %    Working Manual" by J.P. Snyder (1987). Quick quality control performed
	 %    only by comparing with LINZ converter
	 %    (www.linz.govt.nz/apps/coordinateconversions/) and Chuck Taylor's
	 %    (http://home.hiwaay.net/~taylorc/toolbox/geography/geoutm.html) on a
	 %    few test points, so use results with caution. Equations not suitable
	 %    for a latitude of +/- 90deg.
	 %
	 %    UPDATE: Following requests, this new version allows forcing UTM zone
	 %    in input.
	 %
	 % Examples:
	 %
	 %    % set random latitude and longitude arrays
	 %    Lat= 90.*(2.*rand(3)-1)
	 %    Lon= 180.*(2.*rand(3)-1)
	 %
	 %    % let the function find appropriate UTM zone and hemisphere from data
	 %    [x1,y1,utmzone1,utmhemi1] = wgs2utm(Lat,Lon)
	 %
	 %    % forcing unique UTM zone and hemisphere for all data entries
	 %    % note: resulting easting and northing are way off the usual values
	 %    [x2,y2,utmzone2,utmhemi2] = wgs2utm(Lat,Lon,60,'S')
	 %
	 %    % forcing different UTM zone and hemisphere for each data entry
	 %    % note: resulting easting and northing are way off the usual values
	 %    utmzone = floor(59.*rand(3))+1
	 %    utmhemi = char(78 + 5.*round(rand(3)))
	 %    [x3,y3,utmzone3,utmhemi3] = wgs2utm(Lat,Lon,utmzone,utmhemi)
	 %
	 % Author:
	 %   Alexandre Schimel
	 %   MetOcean Solutions Ltd
	 %   New Plymouth, New Zealand
	 %
	 % Version 2:
	 %   February 2011
	 %-------------------------------------------------------------------------
	 */
	double latRad = Lat * M_PI / 180;
	double lonRad = Long * M_PI / 180;

	double a = 6378137;            //semi-major axis
	double b = 6356752.314245;   //semi-minor axis
	double e = sqrt(1 - pow((b / a), 2));   //eccentricity

	double Lon0 = floor(Long / 6) * 6 + 3;
	double lon0 = Lon0 * M_PI / 180; //reference longitude in radians

	double k0 = 0.9996;               // scale on central meridian

	double FE = 500000;             // false easting
	double FN = (Lat < 0) ? 10000000 : 0;             // false northing

	// Equations parameters
	double eps = (e * e) / (1 - (e * e));  // e prime square
	// N: radius of curvature of the earth perpendicular to meridian plane
	// Also, distance from point to polar axis
	double N = a / sqrt(1 - e * e * pow(sin(latRad), 2));
	double T = pow(tan(latRad), 2);
	double C = eps * pow(cos(latRad), 2);
	double A = (lonRad - lon0) * cos(latRad);
	// M: true distance along the central meridian from the equator to lat
	double M = a
			* ((1 - pow(e, 2) / 4 - 3 * pow(e, 4) / 64 - 5 * pow(e, 6) / 256) * latRad
					- (3 * pow(e, 2) / 8 + 3 * pow(e, 4) / 32 + 45 * pow(e, 6) / 1024)
							* sin(2 * latRad)
					+ (15 * pow(e, 4) / 256 + 45 * pow(e, 6) / 1024) * sin(4 * latRad)
					- (35 * pow(e, 6) / 3072) * sin(6 * latRad));

	// easting
	*easting = FE
			+ k0 * N
					* (A + (1 - T + C) * pow(A, 3) / 6
							+ (5 - 18 * T + pow(T, 2) + 72 * C - 58 * eps) * pow(A, 5) / 120);

	// northing
	// M(lat0) = 0 so not used in following formula
	*northing = FN + k0 * M
			+ k0 * N * tan(latRad)
					* (pow(A, 2) / 2 + (5 - T + 9 * C + 4 * pow(C, 2)) * pow(A, 4) / 24
							+ (61 - 58 * T + pow(T, 2) + 600 * C - 330 * eps) * pow(A, 6) / 720);

	// UTM zone
	*zone = floor(Lon0 / 6) + 31;
	*hemi = (Lat < 0) ? 'S' : 'N';
}

void
MapLocation::mapTileCoordsToLatLong(double x, double y, int z, double *Lat, double *Long)
{
	int n = pow(2, z);
	*Long = (x / n) * 360.0 - 180.0;
	*Lat = atan(sinh(M_PI - (y / n) * 2 * M_PI)) * 180 / M_PI;
}
