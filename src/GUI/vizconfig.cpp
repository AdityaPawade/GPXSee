#include <cmath>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>
#include "vizconfig.h"


static QColor parseColor(const QString &s, const QColor &fallback)
{
	QStringList p = s.split(',', Qt::SkipEmptyParts);
	if (p.size() < 3)
		return fallback;
	bool ok1, ok2, ok3;
	int r = p.at(0).trimmed().toInt(&ok1);
	int g = p.at(1).trimmed().toInt(&ok2);
	int b = p.at(2).trimmed().toInt(&ok3);
	if (!(ok1 && ok2 && ok3))
		return fallback;
	int a = (p.size() >= 4) ? p.at(3).trimmed().toInt() : 255;
	return QColor(r, g, b, a);
}

static bool parseBool(const QString &s, bool fallback)
{
	QString v(s.trimmed().toLower());
	if (v == "1" || v == "true" || v == "yes" || v == "on")
		return true;
	if (v == "0" || v == "false" || v == "no" || v == "off")
		return false;
	return fallback;
}

const VizConfig &VizConfig::instance()
{
	static VizConfig cfg;
	return cfg;
}

VizConfig::VizConfig()
{
	loadDefaults();

	// Shipped default next to the executable, then per-user override.
	loadFile(QDir(QCoreApplication::applicationDirPath()).filePath("viz.cfg"));
	QString userDir(QStandardPaths::writableLocation(
	  QStandardPaths::AppDataLocation));
	if (!userDir.isEmpty())
		loadFile(QDir(userDir).filePath("viz.cfg"));
}

void VizConfig::loadDefaults()
{
	// Applied when no viz.cfg is present so the build has sensible behaviour
	// out of the box; any key in viz.cfg overrides the matching default.
	fovRangeMeters   = 55560.0;   // ~30 NM
	scanDefaultDeg   = 60.0;
	radarOnMinCode   = 0;
	radarDefaultColor = QColor(120, 120, 120);
	radarActiveColor  = QColor(0, 150, 0);
	lockScanLabel     = "single-target track";

	radarModes.clear();
	radarModes.insert(0,  RadarModeDef{ "OFF",            QColor(120, 120, 120) });
	radarModes.insert(64, RadarModeDef{ "Search",         QColor(40, 120, 255) });
	radarModes.insert(68, RadarModeDef{ "Track",          QColor(255, 170, 0) });
	radarModes.insert(72, RadarModeDef{ "Lock",           QColor(230, 30, 30) });
	radarModes.insert(76, RadarModeDef{ "Event",          QColor(200, 0, 120) });

	// Continuous radar STATE (fused so a raw mode field that reads OFF while
	// locked still shows the true state). 0=OFF, 1=SEARCH, 2=LOCK.
	radarStates.clear();
	radarStates.insert(0, "OFF");
	radarStates.insert(1, "SEARCH");
	radarStates.insert(2, "LOCK");

	eventActiveThreshold = 1.0;
	eventActiveColor     = QColor(200, 0, 0);
	eventActiveLabel     = "ACTIVE";
	eventIdleLabel       = "-";

	showScanWedge = true;
	showLookRay = true;
	showTrack = true;
	showContact = true;
	lookRayColor = QColor(20, 180, 220);
	lookRayWidthPx = 2.0;
	trackColor = QColor(255, 170, 0);
	contactColor = QColor(230, 30, 30);
	trackMarkerRadiusPx = 7.0;
	contactMarkerRadiusPx = 6.0;
	targetLineWidthPx = 1.0;

	gearDownLabel = "DOWN";  gearUpLabel = "up";
	wowGroundLabel = "ON GROUND"; wowAirLabel = "airborne";
	slatsOutLabel = "OUT";   slatsInLabel = "in";
	missingValueLabel = "-";

	panelTitles.clear();
	panelTitles.insert("live_stats", "Live Stats");
	panelTitles.insert("sensor", "Sensor");
	panelTitles.insert("targets", "Targets");

	fieldLabels.clear();
	fieldLabels.insert("parameter", "Parameter");
	fieldLabels.insert("value", "Value");
	fieldLabels.insert("track_name", "Track");
	fieldLabels.insert("position", "Position");
	fieldLabels.insert("yaw", "Heading");
	fieldLabels.insert("pitch", "Pitch");
	fieldLabels.insert("roll", "Roll");
	fieldLabels.insert("airspeed", "Airspeed");
	fieldLabels.insert("vspeed", "Vert. speed");
	fieldLabels.insert("roll_rate", "Roll rate");
	fieldLabels.insert("yaw_rate", "Yaw rate");
	fieldLabels.insert("sensor_mode", "Mode");
	fieldLabels.insert("scan_width", "Scan width");
	fieldLabels.insert("scan_program", "Scan program");
	fieldLabels.insert("look_az", "Look azimuth");
	fieldLabels.insert("contact_bearing", "Contact bearing");
	fieldLabels.insert("contact_range", "Contact range");
	fieldLabels.insert("contact_alt", "Contact rel altitude");
	fieldLabels.insert("track_bearing", "Track bearing");
	fieldLabels.insert("track_range", "Track range");
	fieldLabels.insert("bearing_rel", "Bearing rel");
	fieldLabels.insert("bearing_abs", "Bearing abs");
	fieldLabels.insert("nav_bearing", "Nav bearing");
	fieldLabels.insert("nav_range", "Nav range");
	fieldLabels.insert("fuel", "Fuel");
	fieldLabels.insert("gear", "Gear");
	fieldLabels.insert("wow", "Weight-on-wheels");
	fieldLabels.insert("auto_slats", "Auto-slats");
	fieldLabels.insert("event", "Event");

	referenceYear = 2026;
	playbackSpeeds = QList<int>() << 1 << 2 << 8 << 16 << 32;
	playbackPlayLabel = "Play";
	playbackPauseLabel = "Pause";
	playbackSpeedLabel = "Speed";
	playbackSpeedSuffix = "x";
	playbackElapsedLabel = "Elapsed";
	playbackRemainingLabel = "Remaining";
	playbackClockFormat = "yyyy-MM-dd HH:mm:ss";
	playbackClockStyle = "QLabel { background: rgba(0,0,0,160); color: white;"
	  " padding: 6px 16px; border-radius: 4px; font: 700 24px; }";
	playbackTimerIntervalMs = 33;
	syncButtonLabel = "Sync...";
	syncDialogTitle = "Track sync";
	syncOffsetLabel = "Offset";
	syncDirectionLabel = "+ = later";
	syncResetLabel = "Reset";
	syncResetAllLabel = "Reset all";
	syncRangeMinutes = 60.0;
	syncStepSeconds = 1.0;

	_sourcePath = "(built-in defaults)";
}

// Minimal, predictable INI reader (avoids QSettings' comma-as-list quirk).
//   ; and # start comments; [section] headers; key=value lines.
void VizConfig::loadFile(const QString &path)
{
	QFile f(path);
	if (!QFileInfo::exists(path) || !f.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream in(&f);
	in.setCodec("UTF-8");   // viz.cfg is UTF-8; without this Windows reads it as CP1252
	QString section;
	while (!in.atEnd()) {
		QString line(in.readLine().trimmed());
		if (line.isEmpty() || line.startsWith(';') || line.startsWith('#'))
			continue;
		if (line.startsWith('[') && line.endsWith(']')) {
			section = line.mid(1, line.size() - 2).trimmed().toLower();
			continue;
		}
		int eq = line.indexOf('=');
		if (eq < 0)
			continue;
		QString key(line.left(eq).trimmed());
		QString val(line.mid(eq + 1).trimmed());

		if (section == "radar") {
			if (key == "fov_range_m")        fovRangeMeters = val.toDouble();
			else if (key == "scan_default_deg") scanDefaultDeg = val.toDouble();
			else if (key == "on_min_code")   radarOnMinCode = val.toInt();
			else if (key == "default_color") radarDefaultColor = parseColor(val, radarDefaultColor);
			else if (key == "active_color")  radarActiveColor = parseColor(val, radarActiveColor);
			else if (key == "lock_scan_label") lockScanLabel = val;
		} else if (section == "radar_modes") {
			bool ok;
			int code = key.toInt(&ok);
			if (!ok)
				continue;
			QStringList parts(val.split('|'));
			RadarModeDef def = radarModes.value(code);
			def.name = parts.at(0).trimmed();
			if (parts.size() >= 2)
				def.color = parseColor(parts.at(1),
				  def.color.isValid() ? def.color : radarDefaultColor);
			else if (!def.color.isValid())
				def.color = radarDefaultColor;
			radarModes.insert(code, def);
		} else if (section == "radar_states") {
			bool ok;
			int code = key.toInt(&ok);
			if (ok)
				radarStates.insert(code, val.trimmed());
		} else if (section == "event") {
			if (key == "active_threshold") eventActiveThreshold = val.toDouble();
			else if (key == "active_color") eventActiveColor = parseColor(val, eventActiveColor);
			else if (key == "active_label") eventActiveLabel = val;
			else if (key == "idle_label")   eventIdleLabel = val;
		} else if (section == "overlay" || section == "contact") {
			if (key == "show_scan_wedge") showScanWedge = parseBool(val, showScanWedge);
			else if (key == "show_look_ray") showLookRay = parseBool(val, showLookRay);
			else if (key == "show_track") showTrack = parseBool(val, showTrack);
			else if (key == "show_contact") showContact = parseBool(val, showContact);
			else if (key == "look_ray_color") lookRayColor = parseColor(val, lookRayColor);
			else if (key == "look_ray_width_px") lookRayWidthPx = val.toDouble();
			else if (key == "track_color") trackColor = parseColor(val, trackColor);
			else if (key == "contact_color") contactColor = parseColor(val, contactColor);
			else if (key == "track_radius_px") trackMarkerRadiusPx = val.toDouble();
			else if (key == "contact_radius_px") contactMarkerRadiusPx = val.toDouble();
			else if (key == "target_line_width_px") targetLineWidthPx = val.toDouble();
			else if (key == "target_color") contactColor = parseColor(val, contactColor);
			else if (key == "target_radius_px") contactMarkerRadiusPx = val.toDouble();
		} else if (section == "labels") {
			if (key == "gear_down")       gearDownLabel = val;
			else if (key == "gear_up")    gearUpLabel = val;
			else if (key == "wow_ground") wowGroundLabel = val;
			else if (key == "wow_air")    wowAirLabel = val;
			else if (key == "slats_out")  slatsOutLabel = val;
			else if (key == "slats_in")   slatsInLabel = val;
			else if (key == "missing")    missingValueLabel = val;
		} else if (section == "panels") {
			panelTitles.insert(key, val);
		} else if (section == "fields") {
			fieldLabels.insert(key, val);
		} else if (section == "playback") {
			if (key == "reference_year")
				referenceYear = val.toInt();
			else if (key == "speeds") {
				QList<int> speeds;
				QStringList parts(val.split(',', Qt::SkipEmptyParts));
				for (int i = 0; i < parts.size(); i++) {
					bool ok;
					int speed = parts.at(i).trimmed().toInt(&ok);
					if (ok && speed > 0)
						speeds.append(speed);
				}
				if (!speeds.isEmpty())
					playbackSpeeds = speeds;
			} else if (key == "play_label") playbackPlayLabel = val;
			else if (key == "pause_label") playbackPauseLabel = val;
			else if (key == "speed_label") playbackSpeedLabel = val;
			else if (key == "speed_suffix") playbackSpeedSuffix = val;
			else if (key == "elapsed_label") playbackElapsedLabel = val;
			else if (key == "remaining_label") playbackRemainingLabel = val;
			else if (key == "clock_format") playbackClockFormat = val;
			else if (key == "clock_style") playbackClockStyle = val;
			else if (key == "timer_interval_ms")
				playbackTimerIntervalMs = qMax(10, val.toInt());
		} else if (section == "sync") {
			if (key == "button_label") syncButtonLabel = val;
			else if (key == "dialog_title") syncDialogTitle = val;
			else if (key == "offset_label") syncOffsetLabel = val;
			else if (key == "direction_label") syncDirectionLabel = val;
			else if (key == "reset_label") syncResetLabel = val;
			else if (key == "reset_all_label") syncResetAllLabel = val;
			else if (key == "range_minutes")
				syncRangeMinutes = qMax(1.0, val.toDouble());
			else if (key == "step_seconds")
				syncStepSeconds = qMax(0.1, val.toDouble());
		}
	}

	_sourcePath = path;
}

QString VizConfig::radarModeName(double mode) const
{
	if (std::isnan(mode))
		return QString("-");
	int v = (int)(mode + 0.5);
	if (radarModes.contains(v))
		return radarModes.value(v).name;
	return QString("0x%1").arg(v, 0, 16);
}

QColor VizConfig::radarModeColor(int code) const
{
	if (radarModes.contains(code))
		return radarModes.value(code).color;
	return radarDefaultColor;
}

bool VizConfig::radarOn(double mode) const
{
	return !std::isnan(mode) && (int)(mode + 0.5) > radarOnMinCode;
}

QString VizConfig::radarStateName(double state) const
{
	if (std::isnan(state))
		return missingValueLabel;
	int v = (int)(state + 0.5);
	if (radarStates.contains(v))
		return radarStates.value(v);
	return QString::number(v);
}

QString VizConfig::panelTitle(const QString &key) const
{
	return panelTitles.value(key, key);
}

QString VizConfig::fieldLabel(const QString &key) const
{
	return fieldLabels.value(key, key);
}
