#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <cmath>

/* Per-point telemetry carried on each track/path point. Populated from optional
   GPX telemetry extensions on each <trkpt>. NAN / -1 = absent. */
struct Telemetry
{
	Telemetry()
	  : roll(NAN), pitch(NAN), yaw(NAN), airspeed(NAN), airspeedKt(NAN),
	    vspeed(NAN),
	    rollRate(NAN), yawRate(NAN), fuelRaw(NAN), fuelPct(NAN), fuelKg(NAN),
	    navBearing(NAN), navRange(NAN), radarMode(NAN), radarScan(NAN),
	    radarScanProgram(NAN), lookAzimuth(NAN),
	    radarState(NAN), radarAz(NAN), radarLockMode(NAN), radarLockWidth(NAN),
	    trackBearing(NAN),
	    trackRange(NAN), contactBearing(NAN), contactRange(NAN),
	    contactAltitude(NAN), event(NAN), gear(-1), wow(-1), wowRaw(-1),
	    autoSlats(-1) {}

	bool isValid() const
	{
		return !std::isnan(yaw) || !std::isnan(roll) || !std::isnan(radarMode)
		  || !std::isnan(radarState)
		  || !std::isnan(lookAzimuth) || !std::isnan(trackRange)
		  || !std::isnan(contactRange) || gear >= 0 || wow >= 0;
	}

	qreal roll, pitch, yaw, airspeed, airspeedKt, vspeed;
	qreal rollRate, yawRate, fuelRaw, fuelPct, fuelKg, navBearing, navRange;
	qreal radarMode, radarScan, radarScanProgram;
	qreal lookAzimuth;
	/* Continuous radar state: 0=OFF, 1=SEARCH, 2=LOCK. Some sources stop emitting
	   the search-mode field while locked on a single target, so a raw mode field
	   can read OFF while still tracking; radarState stays meaningful. radarAz is
	   the effective antenna azimuth for the active phase. */
	qreal radarState, radarAz, radarLockMode, radarLockWidth;
	qreal trackBearing, trackRange;
	qreal contactBearing, contactRange, contactAltitude;
	qreal event;
	int gear, wow, wowRaw, autoSlats;
};

#endif // TELEMETRY_H
