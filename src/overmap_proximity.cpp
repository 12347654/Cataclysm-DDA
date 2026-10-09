// Custom: overmap proximity mechanic (not vanilla)
// See overmap_proximity.h for documentation.

#include "overmap_proximity.h"

#include "debug.h"
#include "overmap.h"

void overmap_proximity_constraint::deserialize( const JsonObject &jo )
{
    // "terrain": list of oter_id (OR logic)
    // Use jo.read for proper deferred ID resolution
    if( jo.has_array( "terrain" ) ) {
        JsonArray ja = jo.get_array( "terrain" );
        for( size_t i = 0; i < ja.size(); ++i ) {
            oter_id tid;
            if( ja.read( i, tid ) ) {
                terrains.push_back( tid );
            }
        }
    } else if( jo.has_string( "terrain" ) ) {
        oter_id tid;
        if( jo.read( "terrain", tid ) ) {
            terrains.push_back( tid );
        }
    }
    if( terrains.empty() ) {
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
    if( terrains.empty() ) {
        debugmsg( "overmap proximity constraint has empty terrain list" );
    }
    if( distance.min < 0 || distance.max < distance.min ) {
        debugmsg( "overmap proximity constraint has invalid distance [%d, %d]",
                  distance.min, distance.max );
    }
    for( const oter_id &tid : terrains ) {
        if( !tid.is_valid() ) {
            debugmsg( "overmap proximity constraint references invalid terrain '%s'",
                      tid.id().str().c_str() );
        }
    }
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
                const oter_id &tid = om.ter( q );
                for( const oter_id &wanted : c.terrains ) {
                    if( tid == wanted ) {
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
