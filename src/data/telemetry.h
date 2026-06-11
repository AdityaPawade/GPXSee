#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <cmath>

/* Per-point telemetry carried on each track/path point. Populated from optional
   GPX telemetry extensions on each <trkpt>. NAN / -1 = absent.

   RAW TRUTH ONLY (2026-06-11): every member maps 1:1 to a value recorded on the
   cartridge (optionally a raw integer x a fixed scale). There is NO fusion, NO
   inference, NO computed bearing or estimated angle anywhere in this struct or
   its consumers — absent fields stay absent (NAN / -1), shown blank.

   Scalar tags additionally carry a raw="N" attribute = the integer source value
   before scaling; it is parsed into the matching ...Raw companion below (NAN =
   absent). For auto_slats the raw is a byte stored in autoSlatsRaw.

   FIELD SET for the re-identified MiG-29K FMU schema:
     - airspeed REMOVED (0x1B4 was not a usable airspeed). groundSpeed = the
       recorded speed channel (raw 0x156), valid throughout the flight.
     - the cid10_09 "radar mode/scan/program" bytes were RE-IDENTIFIED as the
       approach/landing system -> apchMode / apchScan / apchProg.
     - the real Zhuk radar rides its own pages: radarState (the raw 0x145 byte),
       radarScanMode (the raw 0x14E byte), and the recorded antenna azimuths ->
       radarAz (absolute), radarAzRel (nose-relative), radarSearchAz
       (nose-relative). Each is present only when its page was recorded near the
       point; never derived from the others or from heading. */
struct Telemetry
{
	Telemetry()
	  : roll(NAN), pitch(NAN), yaw(NAN), vspeed(NAN),
	    groundSpeed(NAN),
	    rollRate(NAN), yawRate(NAN), fuelRaw(NAN), fuelPct(NAN), fuelKg(NAN),
	    navBearing(NAN), navRange(NAN),
	    apchMode(NAN), apchScan(NAN), apchProg(NAN),
	    radarState(NAN), radarScanMode(NAN),
	    radarAz(NAN), radarAzRel(NAN), radarSearchAz(NAN),
	    trackBearing(NAN),
	    trackRange(NAN), contactBearing(NAN), contactRange(NAN),
	    contactAltitude(NAN), beaconBearing(NAN), beaconBearingRel(NAN),
	    beaconRange(NAN), gear(-1), wow(-1), wowRaw(-1),
	    autoSlats(-1), autoSlatsRaw(-1),
	    rollRaw(NAN), pitchRaw(NAN), yawRaw(NAN),
	    groundSpeedRaw(NAN),
	    vspeedRaw(NAN), rollRateRaw(NAN), yawRateRaw(NAN), navBearingRaw(NAN),
	    radarAzRaw(NAN), radarAzRelRaw(NAN), radarSearchAzRaw(NAN),
	    trackBearingRaw(NAN), trackRangeRaw(NAN), contactBearingRaw(NAN),
	    contactRangeRaw(NAN), contactAltitudeRaw(NAN), beaconBearingRaw(NAN),
	    beaconBearingRelRaw(NAN), beaconRangeRaw(NAN) {}

	bool isValid() const
	{
		return !std::isnan(yaw) || !std::isnan(roll)
		  || !std::isnan(radarState)
		  || !std::isnan(radarSearchAz) || !std::isnan(radarAz)
		  || !std::isnan(trackRange)
		  || !std::isnan(contactRange) || !std::isnan(beaconRange)
		  || gear >= 0 || wow >= 0;
	}

	qreal roll, pitch, yaw, vspeed, groundSpeed;
	qreal rollRate, yawRate, fuelRaw, fuelPct, fuelKg, navBearing, navRange;

	/* Approach/landing-phase system (cid10_09; ex-"radar mode/scan/program").
	   Active only during descent/approach/landing/taxi, never in combat. */
	qreal apchMode, apchScan, apchProg;

	/* Zhuk radar — RAW recorded bytes/words only. radarState = the raw 0x145
	   state byte (0=OFF, 1=SEARCH, 2=LOCK, other = shown as the number).
	   radarScanMode = the raw 0x14E scan-pattern byte. radarAz is the recorded
	   ABSOLUTE antenna azimuth; radarAzRel / radarSearchAz are recorded
	   nose-relative azimuths. Each is present only when its page was recorded;
	   they are never fused with each other or with heading. */
	qreal radarState, radarScanMode;
	qreal radarAz, radarAzRel, radarSearchAz;

	qreal trackBearing, trackRange;
	qreal contactBearing, contactRange, contactAltitude;
	qreal beaconBearing, beaconBearingRel, beaconRange;
	int gear, wow, wowRaw, autoSlats, autoSlatsRaw;

	/* Raw (pre-scaling) source integers from the raw="N" attributes; NAN =
	   the attribute was absent. Shown in parentheses in the telemetry panel. */
	qreal rollRaw, pitchRaw, yawRaw, groundSpeedRaw, vspeedRaw;
	qreal rollRateRaw, yawRateRaw, navBearingRaw;
	qreal radarAzRaw, radarAzRelRaw, radarSearchAzRaw;
	qreal trackBearingRaw, trackRangeRaw, contactBearingRaw, contactRangeRaw;
	qreal contactAltitudeRaw, beaconBearingRaw, beaconBearingRelRaw,
	  beaconRangeRaw;
};

#endif // TELEMETRY_H
