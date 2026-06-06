#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <cmath>

/* Per-point telemetry carried on each track/path point. Populated from the
   custom GPX <extensions> tags on each <trkpt>. NAN / -1 = absent.
   Used by the Live Stats dock and the radar/target map overlays. */
struct Telemetry
{
	Telemetry()
	  : roll(NAN), pitch(NAN), yaw(NAN), airspeed(NAN), vspeed(NAN),
	    rollRate(NAN), yawRate(NAN), fuelPct(NAN), navBearing(NAN),
	    navRange(NAN), radarMode(NAN), radarScan(NAN),
	    contactBearing(NAN), contactRange(NAN), weapon(NAN),
	    gear(-1), wow(-1), autoSlats(-1) {}

	bool isValid() const
	{
		return !std::isnan(yaw) || !std::isnan(roll) || !std::isnan(radarMode)
		  || gear >= 0 || wow >= 0;
	}

	qreal roll, pitch, yaw, airspeed, vspeed;
	qreal rollRate, yawRate, fuelPct, navBearing, navRange;
	qreal radarMode, radarScan;          // radar status (mode enum, scan width deg)
	qreal contactBearing, contactRange;  // contact (relative bearing deg, range m)
	qreal weapon;                        // weapon/engagement flag (1 = active)
	int gear, wow, autoSlats;            // discretes (1/0, -1 = absent)
};

#endif // TELEMETRY_H
