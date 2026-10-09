// Custom: overmap proximity mechanic (not vanilla)
// See overmap_proximity.h for documentation.

#include "overmap_proximity.h"

#include "debug.h"
#include "overmap.h"

void overmap_proximity_constraint::deserialize( const JsonObject &jo )
{
    // "terrain": list of terrain ID strings (OR logic)
    // Stored as strings to avoid load-time ID validation;
    // converted to oter_id at runtime in satisfies().
    if( jo.has_array( "terrain" ) ) {
        JsonArray ja = jo.get_array( "terrain" );
        for( size_t i = 0; i < ja.size(); ++i ) {
            terrain_strs.push_back( ja.get_string( i ) );
        }
    } else if( jo.has_string( "terrain" ) ) {
        // Allow single string for convenience
        terrain_strs.push_back( jo.get_string( "terrain" ) );
    }
    if( terrain_strs.empty() ) {
        jo.throw_error( "\"proximity\" constraint requires \"terrain\" (string or array)" );
    }

    // "distance": [min, max] interval (mandatory)
    if( !jo.has_array( "distance" ) ) {
        jo.throw_error( "\"proximity\" constraint requires \"distance\" ([min, max])" );
    }
    JsonArray ja = jo.get_array( "distance" );
    if( ja.size() != 2 ) {
        jo.throw_error( "\"distance\" must be [min, max]" );
    }
    distance.min = ja.get_int( 0 );
    distance.max = ja.get_int( 1 );
}

void overmap_proximity_constraint::check() const
{
    if( terrain_strs.empty() ) {
        debugmsg( "overmap proximity constraint has empty terrain list" );
    }
    if( distance.min < 0 || distance.max < distance.min ) {
        debugmsg( "overmap proximity constraint has invalid distance [%d, %d]",
                  distance.min, distance.max );
    }
    // Note: terrain ID validity is checked at runtime in satisfies(),
    // not here, to avoid load-order issues.
}

void overmap_proximity::deserialize( const JsonArray &ja )
{
    for( const JsonObject jo : ja ) {
        overmap_proximity_constraint c;
        c.deserialize( jo );
        constraints.push_back( std::move( c ) );
    }
}

void overmap_proximity::check() const
{
    for( const overmap_proximity_constraint &c : constraints ) {
        c.check();
    }
}

bool overmap_proximity::satisfies( const overmap &om, const tripoint_om_omt &p ) const
{
    for( const overmap_proximity_constraint &c : constraints ) {
        bool found = false;
        // Cap max scan radius for performance (4M tiles at 999 is too slow)
        const int max_d = std::min( c.distance.max, 100 );
        // Scan square around p with Chebyshev distance
        for( int dx = -max_d; dx <= max_d && !found; ++dx ) {
            for( int dy = -max_d; dy <= max_d && !found; ++dy ) {
                // Chebyshev distance
                const int dist = std::max( std::abs( dx ), std::abs( dy ) );
                if( dist < c.distance.min || dist > c.distance.max ) {
                    continue;
                }
                const tripoint_om_omt q = p + tripoint( dx, dy, 0 );
                // Compare ID strings directly to avoid oter_id(string) validation issues
                const std::string tid_str = om.ter( q ).id().str();
                for( const std::string &wanted : c.terrain_strs ) {
                    if( tid_str == wanted ) {
                        found = true;
                        break;
                    }
                }
            }
        }
        if( !found ) {
            return false; // AND logic: all constraints must pass
        }
    }
    return true;
}
// trigger
