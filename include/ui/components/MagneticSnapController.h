/*
    Oscil - Magnetic Snap Controller
    Reusable controller for magnetic snapping behavior in sliders, knobs, and other controls
*/

#pragma once

#include <vector>
#include <algorithm>

namespace oscil
{

/**
 * Controller for magnetic snapping behavior
 *
 * Manages snap points and applies snapping logic to input values.
 * Designed to be reusable across sliders, knobs, and other continuous controls.
 *
 * Usage:
 *   MagneticSnapController snapController;
 *   snapController.setMagneticPoints({0.0, 50.0, 100.0});
 *
 *   bool didSnap = false;
 *   double snappedValue = snapController.applySnapping(value, 0.0, 100.0, didSnap);
 *
 *   if (didSnap && snapController.consumeSnapFeedback()) {
 *       // Trigger visual/audio feedback
 *   }
 */
class MagneticSnapController
{
public:
    MagneticSnapController() = default;
    ~MagneticSnapController() = default;

    /**
     * Enable or disable magnetic snapping
     * When disabled, applySnapping() returns the input value unchanged
     */
    void setEnabled(bool enabled);
    bool isEnabled() const;

    /**
     * Set all magnetic snap points at once
     * Points are values where the control will "snap" when nearby
     */
    void setMagneticPoints(const std::vector<double>& points);

    /**
     * Add a single magnetic point (if not already present)
     */
    void addMagneticPoint(double point);

    /**
     * Remove all magnetic points
     */
    void clearMagneticPoints();

    /**
     * Get current magnetic points (read-only access)
     */
    const std::vector<double>& getMagneticPoints() const;

    /**
     * Apply magnetic snapping to a value
     *
     * @param value The input value to potentially snap
     * @param minValue Minimum value of the control's range
     * @param maxValue Maximum value of the control's range
     * @param didSnap Output parameter - set to true if snapping occurred
     * @return The snapped value (or original if no snap point nearby)
     *
     * The snap threshold is 2% of the range (minValue to maxValue)
     */
    double applySnapping(double value, double minValue, double maxValue, bool& didSnap);

    /**
     * Check if a snap just occurred and consume the feedback flag
     *
     * This is designed for triggering one-shot feedback animations/sounds.
     * Returns true once after a snap, then false until the next snap.
     *
     * @return true if snap feedback should be triggered, false otherwise
     */
    bool consumeSnapFeedback();

private:
    bool enabled_ = true;
    std::vector<double> magneticPoints_;
    bool justSnapped_ = false;

    // Snap threshold as percentage of range
    static constexpr double SNAP_THRESHOLD_PERCENT = 0.02; // 2%
};

} // namespace oscil
