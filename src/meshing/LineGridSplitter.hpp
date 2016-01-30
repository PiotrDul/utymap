#ifndef TERRAIN_LINEGRIDSPLITTER_HPP_DEFINED
#define TERRAIN_LINEGRIDSPLITTER_HPP_DEFINED

#include "clipper/clipper.hpp"
#include "meshing/MeshTypes.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace utymap { namespace meshing {

// Splits line to segments according to axis aligned grid.
template<typename T>
class LineGridSplitter
{
    typedef utymap::meshing::Point<T> TPoint;
    typedef std::vector<TPoint> Points;

    typedef ClipperLib::cInt Int;
    typedef ClipperLib::IntPoint IPoint;
    typedef std::vector<IPoint> IPoints;

    struct sort_x
    {
        inline bool operator() (const IPoint& a, const IPoint& b) { return a.X < b.X; }
    };

    struct sort_reverse_x
    {
        inline bool operator() (const IPoint& a, const IPoint& b) { return a.X > b.X; }
    };

    struct sort_y
    {
        inline bool operator() (const IPoint& a, const IPoint& b) { return a.Y < b.Y; }
    };

    struct sort_reverse_y
    {
        inline bool operator() (const IPoint& a, const IPoint& b) { return a.Y > b.Y; }
    };

public:
    LineGridSplitter() :
        step_(1),
        scale_(1)
    {
    }

    void setParams(uint64_t scale, double step)
    {
        scale_ = scale;
        step_ = step * scale;
    }

    // Splits line to segments.
    void split(IPoint start, IPoint end, Points& result)
    {
        std::vector<IPoint> points;
        points.reserve(2);
        points.push_back(start);

        double slope = (end.Y - start.Y) / ((double)end.X - start.X);
        if (std::isinf(slope) || std::abs(slope) < std::numeric_limits<double>::epsilon())
            zeroSlope(start, end, points);
        else
            normalCase(start, end, slope, points);

        points.push_back(end);

        filterResults(points, result);
    }

private:

    inline Int ceil(Int value)
    {
        return std::ceil(value / step_) * step_;
    }

    void zeroSlope(IPoint start, IPoint end, IPoints& points)
    {
        if ((start.X - end.X) == 0)
        {
            bool isBottomTop = start.Y < end.Y;
            if (!isBottomTop)
            {
                IPoint tmp = start;
                start = end;
                end = tmp;
            }

            Int yStart = ceil(start.Y);
            Int yEnd = end.Y;
            for (double y = yStart; y < yEnd; y += step_)
                points.push_back(IPoint(start.X, y));

            if (isBottomTop)
                std::sort(points.begin(), points.end(), sort_y());
            else
                std::sort(points.begin(), points.end(), sort_reverse_y());
        }
        else
        {
            bool isLeftRight = start.X < end.X;
            if (!isLeftRight)
            {
                IPoint tmp = start;
                start = end;
                end = tmp;
            }

            Int xStart = ceil(start.X);
            Int xEnd = end.X;
            for (double x = xStart; x < xEnd; x += step_)
                points.push_back(IPoint(x, start.Y));

            if (isLeftRight)
                std::sort(points.begin(), points.end(), sort_x());
            else
                std::sort(points.begin(), points.end(), sort_reverse_x());
        }
    }

    void normalCase(IPoint start, IPoint end, double slope, IPoints& points)
    {
        double inverseSlope = 1 / slope;
        double b = start.Y - slope * start.X;

        bool isLeftRight = start.X < end.X;
        if (!isLeftRight)
        {
            IPoint tmp = start;
            start = end;
            end = tmp;
        }

        Int xStart = ceil(start.X);
        Int xEnd = end.X;
        for (double x = xStart; x < xEnd; x += step_)
            points.push_back(IPoint(x, slope * x + b));

        bool isBottomTop = start.Y < end.Y;
        if (!isBottomTop)
        {
            IPoint tmp = start;
            start = end;
            end = tmp;
        }

        Int yStart = ceil(start.Y);
        Int yEnd = end.Y;
        for (double y = yStart; y < yEnd; y += step_)
            points.push_back(IPoint((y - b) * inverseSlope, y));

        if (isLeftRight)
            std::sort(points.begin(), points.end(), sort_x());
        else
            std::sort(points.begin(), points.end(), sort_reverse_x());
    }

    void filterResults(const IPoints& points, Points& result)
    {
        for (int i = 0; i < points.size(); ++i)
        {
            IPoint candidate = points[i];
            if (result.size() > 0)
            {
                TPoint last = result[result.size() - 1];
                Int lastX = last.x * scale_;
                Int lastY = last.y * scale_;
                IPoint point(last.x * scale_, last.y * scale_);
                if (candidate == point) 
                    continue;
            }

            result.push_back(TPoint(candidate.X / scale_, candidate.Y / scale_));
        }

        // NOTE do not allow first vertex to be equal the last one
        if (result[0] == result[result.size() - 1])
            result.pop_back();
    }

    double scale_;
    double step_;
};

}}

#endif // TERRAIN_LINEGRIDSPLITTER_HPP_DEFINED
