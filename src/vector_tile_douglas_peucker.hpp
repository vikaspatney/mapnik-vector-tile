#ifndef __MAPNIK_VECTOR_TILE_DOUGLAS_PEUCKER_H__
#define __MAPNIK_VECTOR_TILE_DOUGLAS_PEUCKER_H__

// mapnik
#include <mapnik/geometry.hpp>

// std
#include <vector>

namespace mapnik 
{

namespace vector_tile_impl
{

namespace detail
{

template<typename T>
struct douglas_peucker_point
{
    mapnik::geometry::point<T> const& p;
    bool included;

    inline douglas_peucker_point(mapnik::geometry::point<T> const& ap)
        : p(ap)
        , included(false)
    {}

    inline douglas_peucker_point<T> operator=(douglas_peucker_point<T> const& )
    {
        return douglas_peucker_point<T>(*this);
    }
};

template <typename value_type, typename calc_type>
inline void consider(typename std::vector<douglas_peucker_point<value_type> >::iterator begin,
                     typename std::vector<douglas_peucker_point<value_type> >::iterator end,
                     calc_type const& max_dist)
{
    typedef typename std::vector<douglas_peucker_point<value_type> >::iterator iterator_type;
    
    std::size_t size = end - begin;

    // size must be at least 3
    // because we want to consider a candidate point in between
    if (size <= 2)
    {
        return;
    }

    iterator_type last = end - 1;

    // Find most far point, compare to the current segment
    calc_type md(-1.0); // any value < 0
    iterator_type candidate;
    {
        /*
            Algorithm [p: (px,py), p1: (x1,y1), p2: (x2,y2)]
            VECTOR v(x2 - x1, y2 - y1)
            VECTOR w(px - x1, py - y1)
            c1 = w . v
            c2 = v . v
            b = c1 / c2
            RETURN POINT(x1 + b * vx, y1 + b * vy)
        */
        calc_type const v_x = last->p.x - begin->p.x;
        calc_type const v_y = last->p.y - begin->p.y;
        calc_type const c2 = v_x * v_x + v_y * v_y;
        for(iterator_type it = begin + 1; it != last; ++it)
        {
            calc_type const w_x = it->p.x - begin->p.x;
            calc_type const w_y = it->p.y - begin->p.y;
            calc_type const c1 = w_x * v_x + w_y * v_y;
            calc_type dist;
            if (c1 <= 0) // calc_type() should be 0 of the proper calc type format
            {
                calc_type const dx = it->p.x - begin->p.x;
                calc_type const dy = it->p.y - begin->p.y;
                dist = dx * dx + dy * dy;
            }
            else if (c2 <= c1)
            {
                calc_type const dx = it->p.x - last->p.x;
                calc_type const dy = it->p.y - last->p.y;
                dist = dx * dx + dy * dy;
            }
            else 
            {
                // See above, c1 > 0 AND c2 > c1 so: c2 != 0
                calc_type const b = c1 / c2;
                calc_type const p_x = begin->p.x + b * v_x;
                calc_type const p_y = begin->p.y + b * v_y;
                calc_type const dx = it->p.x - p_x;
                calc_type const dy = it->p.y - p_y;
                dist = dx * dx + dy * dy;
            }
            if (md < dist)
            {
                md = dist;
                candidate = it;
            }
        }
    }

    // If a point is found, set the include flag
    // and handle segments in between recursively
    if (max_dist < md)
    {
        candidate->included = true;
        consider<value_type>(begin, candidate + 1, max_dist);
        consider<value_type>(candidate, end, max_dist);
    }
}

} // end ns detail

template <typename value_type, typename calc_type, typename Range, typename OutputIterator>
inline void douglas_peucker(Range const& range,
                            OutputIterator out,
                            calc_type max_distance)
{
    // Copy coordinates, a vector of references to all points
    std::vector<detail::douglas_peucker_point<value_type> > ref_candidates(std::begin(range),
                    std::end(range));

    // Include first and last point of line,
    // they are always part of the line
    ref_candidates.front().included = true;
    ref_candidates.back().included = true;

    // We will compare to squared of distance so we don't have to do a sqrt
    calc_type const max_sqrd = max_distance * max_distance;

    // Get points, recursively, including them if they are further away
    // than the specified distance
    detail::consider<value_type, calc_type>(std::begin(ref_candidates), std::end(ref_candidates), max_sqrd);

    // Copy included elements to the output
    for(typename std::vector<detail::douglas_peucker_point<value_type> >::const_iterator it
                    = std::begin(ref_candidates);
        it != std::end(ref_candidates);
        ++it)
    {
        if (it->included)
        {
            // copy-coordinates does not work because OutputIterator
            // does not model Point (??)
            //geometry::convert(it->p, *out);
            *out = it->p;
            out++;
        }
    }
}

} // end ns vector_tile_impl

} // end ns mapnik

#endif // __MAPNIK_VECTOR_TILE_SIMPLIFY_H__
