// Custom: overmap proximity mechanic (not vanilla)
//
// Allows overmap_special JSON to require placement near (or far from)
// specific overmap terrain types.
//
// Example:
//   "proximity": [
//     { "terrain": ["lake_shore", "river"], "distance": [1, 5] },
//     { "terrain": ["forest"], "distance": [0, 3] }
//   ]
//
// Logic:
// - Inside "terrain" list: OR (near lake_shore OR near river)
// - Between objects: AND (near water AND near forest)
// - "distance": [min, max] in overmap tiles (Chebyshev distance)
// - [2, 2] = exactly 2 away, [0, 5] = within 5, [5, 999] = at least 5 away

#pragma once
#ifndef CATA_SRC_OVERMAP_PROXIMITY_H
#define CATA_SRC_OVERMAP_PROXIMITY_H

#include <vector>

#include "common_types.h"
#include "coordinates.h"
#include "json.h"
#include "type_id.h"

class overmap;

struct overmap_proximity_constraint {
    // Terrain ID strings (stored as strings to avoid load-time validation;
    // converted to oter_id at runtime in satisfies())
    std::vector<std::string> terrain_strs;
    // Distance range [min, max] in overmap tiles (Chebyshev distance)
    numeric_interval<int> distance{ 0, 0 };

    void deserialize( const JsonObject &jo );
    void check() const;
};

struct overmap_proximity {
    // List of constraints (AND logic: ALL must be satisfied)
    std::vector<overmap_proximity_constraint> constraints;

    bool empty() const {
        return constraints.empty();
    }
    void deserialize( const JsonArray &ja );
    void check() const;
    /**
     * Check if point p on overmap om satisfies all proximity constraints.
     * @returns true if all constraints are satisfied (or if empty).
     */
    bool satisfies( const overmap &om, const tripoint_om_omt &p ) const;
};

#endif // CATA_SRC_OVERMAP_PROXIMITY_H
