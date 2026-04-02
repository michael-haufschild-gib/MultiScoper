/*
    Oscil - Magnetic Snap Controller Implementation
*/

#include "ui/components/MagneticSnapController.h"

#include <algorithm>
#include <cmath>

namespace oscil
{

void MagneticSnapController::setEnabled(bool enabled) { enabled_ = enabled; }

bool MagneticSnapController::isEnabled() const { return enabled_; }

void MagneticSnapController::setMagneticPoints(const std::vector<double>& points) { magneticPoints_ = points; }

void MagneticSnapController::addMagneticPoint(double point)
{
    // Only add if not already present
    if (std::ranges::find(magneticPoints_, point) == magneticPoints_.end())
    {
        magneticPoints_.push_back(point);
    }
}

void MagneticSnapController::clearMagneticPoints() { magneticPoints_.clear(); }

const std::vector<double>& MagneticSnapController::getMagneticPoints() const { return magneticPoints_; }

double MagneticSnapController::applySnapping(double value, double minValue, double maxValue, bool& didSnap)
{
    didSnap = false;

    // If disabled or range is degenerate, return value unchanged
    if (!enabled_)
    {
        justSnapped_ = false;
        return value;
    }

    double range = maxValue - minValue;
    if (range <= 0.0)
    {
        justSnapped_ = false;
        return value;
    }

    // Calculate snap threshold as percentage of range
    double snapThreshold = range * SNAP_THRESHOLD_PERCENT;

    // Check each magnetic point
    for (double point : magneticPoints_)
    {
        if (std::abs(value - point) < snapThreshold)
        {
            // Snap to this point
            didSnap = true;
            justSnapped_ = true;
            return point;
        }
    }

    // No snap occurred
    justSnapped_ = false;
    return value;
}

bool MagneticSnapController::consumeSnapFeedback()
{
    if (justSnapped_)
    {
        justSnapped_ = false;
        return true;
    }
    return false;
}

} // namespace oscil
