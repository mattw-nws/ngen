#ifndef HY_CATCHEMENTDIVIDE_H
#define HY_CATCHEMENTDIVIDE_H

#include "GM_Object.h"

#include <memory>
#include <string>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

namespace bg = boost::geometry;

class HY_CatchementDivide : public GM_Object
{
    public:

    typedef bg::model::point<double, 2, bg::cs::cartesian> point_t;
    typedef bg::model::linestring<point_t> linestring_t;

    HY_CatchementDivide();
    virtual ~HY_CatchementDivide();

    protected:

    private:

    std::shared_ptr<linestring_t> divide_path;

    unsigned long id_number;
    std::string id;
};

#endif // HY_CATCHEMENTDIVIDE_H
