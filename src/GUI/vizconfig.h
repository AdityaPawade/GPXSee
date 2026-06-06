#ifndef VIZCONFIG_H
#define VIZCONFIG_H

// ---------------------------------------------------------------------------
// VizConfig — runtime configuration for the telemetry-overlay layer.
//
// GPX tracks may carry custom per-point telemetry in <trkpt><extensions>
// (attitude, rates, fuel, radar mode/scan, contacts, discrete states). How
// those values are INTERPRETED for display (radar-mode value -> name, the FOV
// wedge length, the scan-sector default, the "engaged" threshold, the
// discrete-state labels and the overlay colours) is read from an external INI
// file `viz.cfg` so that NOTHING is hard-coded in C++. Built-in defaults apply
// when no file is present; the file only needs to list the keys it overrides.
//
// Search order (later overrides earlier):
//   1) built-in defaults (loadDefaults())
//   2) <application dir>/viz.cfg            (shipped next to the executable)
//   3) <AppDataLocation>/GPXSee/viz.cfg     (per-user override, hand-editable)
// ---------------------------------------------------------------------------

#include <QString>
#include <QColor>
#include <QMap>

struct RadarModeDef {
	QString name;
	QColor  color;
};

class VizConfig
{
public:
	static const VizConfig &instance();

	// [radar]
	double fovRangeMeters;       // visual radar wedge length (m)
	double scanDefaultDeg;       // sector width used when scan is unknown (deg)
	int    radarOnMinCode;       // radar considered "on" when mode > this
	QColor radarDefaultColor;    // wedge colour for unknown modes
	QColor radarActiveColor;     // Live-Stats text colour when radar is on

	// [radar_modes]  value -> { name, wedge colour }
	QMap<int, RadarModeDef> radarModes;

	// [weapon]
	double  weaponEngagedThreshold;
	QColor  weaponEngagedColor;
	QString weaponEngagedLabel;
	QString weaponIdleLabel;

	// [contact]
	QColor targetColor;
	double targetMarkerRadiusPx;

	// [labels] discrete-state interpretations
	QString gearDownLabel, gearUpLabel;
	QString wowGroundLabel, wowAirLabel;
	QString slatsOutLabel, slatsInLabel;

	// helpers
	QString radarModeName(double mode) const;   // value -> display name
	QColor  radarModeColor(int code) const;     // value -> wedge colour
	bool    radarOn(double mode) const;
	QString sourcePath() const { return _sourcePath; }

private:
	VizConfig();
	void loadDefaults();
	void loadFile(const QString &path);

	QString _sourcePath;
};

#endif // VIZCONFIG_H
