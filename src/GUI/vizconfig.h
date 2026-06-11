#ifndef VIZCONFIG_H
#define VIZCONFIG_H

// ---------------------------------------------------------------------------
// VizConfig - runtime configuration for the telemetry-overlay layer.
//
// GPX tracks may carry custom per-point telemetry in <trkpt><extensions>
// (attitude, rates, fuel, sensor scan state, tracks, contacts, discrete
// states). Presentation labels and overlay styles are read from `viz.cfg`.
//
// Search order (later overrides earlier):
//   1) built-in defaults (loadDefaults())
//   2) <application dir>/viz.cfg            (shipped next to the executable)
//   3) <AppDataLocation>/GPXSee/viz.cfg     (per-user override, hand-editable)
// ---------------------------------------------------------------------------

#include <QString>
#include <QList>
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
	double fovRangeMeters;       // visual sensor wedge length (m)
	double scanDefaultDeg;       // sector width used when scan is unknown (deg)
	int    radarOnMinCode;       // sensor considered active when mode > this
	QColor radarDefaultColor;    // wedge colour for unknown modes
	QColor radarActiveColor;     // status text colour when sensor is active

	// [radar_modes]  value -> { name, wedge colour }
	QMap<int, RadarModeDef> radarModes;
	QMap<int, QString> radarStates;   // radar state code -> label (0/1/2/3)
	QMap<int, QColor>  radarStateColors;  // radar state code -> status colour
	// [radar_scan_modes] raw scan-pattern code -> full sector width (deg).
	// A mapped value of 0 means "pencil beam" (no wedge, ray only). Unmapped
	// codes fall back to scanDefaultDeg. Drives the on-map FOV wedge width.
	QMap<int, double>  radarScanModes;

	// [event]
	double  eventActiveThreshold;
	QColor  eventActiveColor;
	QString eventActiveLabel;
	QString eventIdleLabel;

	// [overlay]
	bool showScanWedge, showLookRay, showTrack, showContact;
	QColor lookRayColor;
	double lookRayWidthPx;
	QColor trackColor, contactColor;
	QColor beaconColor;
	double trackMarkerRadiusPx, contactMarkerRadiusPx;
	double targetLineWidthPx;

	// [labels] discrete-state interpretations
	QString gearDownLabel, gearUpLabel;
	QString wowGroundLabel, wowAirLabel;
	QString slatsOutLabel, slatsInLabel;
	QString missingValueLabel;
	QMap<QString, QString> panelTitles;
	QMap<QString, QString> fieldLabels;

	// [playback]
	int referenceYear;
	QList<int> playbackSpeeds;
	QString playbackPlayLabel, playbackPauseLabel, playbackSpeedLabel;
	QString playbackSpeedSuffix;
	QString playbackElapsedLabel, playbackRemainingLabel;
	QString playbackClockFormat, playbackClockStyle;
	int playbackTimerIntervalMs;

	// [sync]
	QString syncButtonLabel, syncDialogTitle, syncOffsetLabel;
	QString syncDirectionLabel, syncResetLabel, syncResetAllLabel;
	double syncRangeMinutes, syncStepSeconds;

	// helpers
	QString radarModeName(double mode) const;   // value -> display name
	QColor  radarModeColor(int code) const;     // value -> wedge colour
	bool    radarOn(double mode) const;
	QString radarStateName(double state) const; // raw byte -> 0/1/2=OFF/SEARCH/LOCK, else the number
	QColor  radarStateColor(double state) const; // status colour for a state code
	// Full sector width (deg) for a raw scan-mode code: >0 = sector wedge,
	// 0 = pencil beam (ray only, no wedge), NAN = unknown (caller may default).
	double  scanModeWidthDeg(double mode) const;
	QString lockScanLabel;   // shown for scan-mode during a lock (no sector scan)
	QString panelTitle(const QString &key) const;
	QString fieldLabel(const QString &key) const;
	QString sourcePath() const { return _sourcePath; }

private:
	VizConfig();
	void loadDefaults();
	void loadFile(const QString &path);

	QString _sourcePath;
};

#endif // VIZCONFIG_H
