// Location.h
#pragma once

struct GeoLocation
{
    double latitudeDeg;
    double longitudeDeg;
    double altitudeM;
    bool   valid;
};

// Blocks until a location is obtained or times out (~10s).
GeoLocation getCurrentLocation();

