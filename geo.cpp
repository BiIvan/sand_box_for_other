//2026-04-26 10:35

#define _USE_MATH_DEFINES

#include <cmath>

#include "geo.h"

namespace geo {

  double ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = M_PI / 180.;
    static const int kEarthRadius = 6371000;
    return acos(sin(from.lat * dr) * sin(to.lat * dr) 
      + cos(from.lat * dr) * cos(to.lat * dr) 
      * cos(abs(from.lng - to.lng) * dr)) * kEarthRadius;
  }

}  // namespace geo