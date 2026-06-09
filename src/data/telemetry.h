#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <cmath>

/* Per-point telemetry carried on each track/path point. Populated from optional
   GPX telemetry extensions on each <trkpt>. NAN / -1 = absent.

   Scalar tags additionally carry a raw="N" attribute = the integer source value
   before scaling; it is parsed into the matching ...Raw companion below (NAN =
   absent). For auto_slats the raw is a hex byte stored in autoSlatsRaw. */
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
	    contactAltitude(NAN), beaconBearing(NAN), beaconBearingRel(NAN),
	    beaconRange(NAN), event(NAN), gear(-1), wow(-1), wowRaw(-1),
	    autoSlats(-1), autoSlatsRaw(-1),
	    rollRaw(NAN), pitchRaw(NAN), yawRaw(NAN), airspeedRawVal(NAN),
	    vspeedRaw(NAN), rollRateRaw(NAN), yawRateRaw(NAN), navBearingRaw(NAN),
	    trackBearingRaw(NAN), trackRangeRaw(NAN), contactBearingRaw(NAN),
	    contactRangeRaw(NAN), contactAltitudeRaw(NAN), beaconBearingRaw(NAN),
	    beaconBearingRelRaw(NAN), beaconRangeRaw(NAN) {}

	bool isValid() const
	{
		return !std::isnan(yaw) || !std::isnan(roll) || !std::isnan(radarMode)
		  || !std::isnan(radarState)
		  || !std::isnan(lookAzimuth) || !std::isnan(trackRange)
		  || !std::isnan(contactRange) || !std::isnan(beaconRange)
		  || gear >= 0 || wow >= 0;
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
	qreal beaconBearing, beaconBearingRel, beaconRange;
	qreal event;
	int gear, wow, wowRaw, autoSlats, autoSlatsRaw;

	/* Raw (pre-scaling) source integers from the raw="N" attributes; NAN =
	   the attribute was absent. Shown in parentheses in the telemetry panel. */
	qreal rollRaw, pitchRaw, yawRaw, airspeedRawVal, vspeedRaw;
	qreal rollRateRaw, yawRateRaw, navBearingRaw;
	qreal trackBearingRaw, trackRangeRaw, contactBearingRaw, contactRangeRaw;
	qreal contactAltitudeRaw, beaconBearingRaw, beaconBearingRelRaw,
	  beaconRangeRaw;
};

#endif // TELEMETRY_H
