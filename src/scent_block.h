#pragma once
#ifndef SCENT_BLOCK_H
#define SCENT_BLOCK_H

#include <algorithm>
#include <array>

#include "point.h"
#include "scent_map.h"

struct scent_block {
    template<typename T>
    using data_block = std::array < std::array < T, SEEY + 2 >, SEEX + 2 >;

    enum data_mode {
        NONE = 0,
        SET = 1,
        MAX = 2
    };

    struct datum {
        data_mode mode;
        int intensity;
    };

    data_block<bool> assignable;
    data_block<datum> assignment;

    tripoint origin;
    scent_map &scents;
    int modification_count;

    scent_block( int subx, int suby, int subz, scent_map &scents )
        : origin( subx * SEEX - 1, suby * SEEY - 1, subz ), scents( scents ), modification_count( 0 ) {
        for( int x = 0; x < SEEX + 2; ++x ) {
            for( int y = 0; y < SEEY + 2; ++y ) {
                assignable[x][y] = scents.inbounds( origin + tripoint( x, y, 0 ) );
                assignment[x][y] = { NONE, 0 };
            }
        }
    }

    void commit_modifications() {
        if( modification_count == 0 ) {
            return;
        }
        for( int x = 0; x < SEEX + 2; ++x ) {
            for( int y = 0; y < SEEY + 2; ++y ) {
                if( assignable[x][y] ) {
                    switch( assignment[x][y].mode ) {
                        case NONE:
                            break;
                        case SET: {
                            scents.set_unsafe( origin + tripoint( x, y, 0 ), assignment[x][y].intensity );
                            break;
                        }
                        case MAX: {
                            tripoint p = origin + tripoint( x, y, 0 );
                            scents.set_unsafe( p, std::max( assignment[x][y].intensity, scents.get_unsafe( p ) ) );
                            break;
                        }
                    }
                }
            }
        }
    }

    point index( const tripoint &p ) const {
        return point( p.x - origin.x, p.y - origin.y );
    }

    // We should be working entirely within the range, so don't range check here
    void apply_gas( const tripoint &p ) {
        const point ndx = index( p );
        assignment[ndx.x][ndx.y].mode = SET;
        assignment[ndx.x][ndx.y].intensity = 0;
        ++modification_count;
    }
    void apply_slime( const tripoint &p, int intensity ) {
        const point ndx = index( p );
        datum &dat = assignment[ndx.x][ndx.y];
        switch( dat.mode ) {
            case NONE: {
                // we don't know what the current intensity is, so we must do a max operation
                dat.mode = MAX;
                dat.intensity = intensity;
                break;
            }
            case SET: {
                // new intensity is going to be dat.intensity, so we just need to make it larger
                // but cannot change
                dat.intensity = std::max( dat.intensity, intensity );
                break;
            }
            case MAX: {
                // Already max for some reason, shouldn't occur. If it does we want to grow if possible
                dat.intensity = std::max( dat.intensity, intensity );
                break;
            }
        }
        ++modification_count;
    }
};

#endif
