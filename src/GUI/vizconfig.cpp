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

	radarModes.clear();
	radarModes.insert(0,  RadarModeDef{ "OFF",            QColor(120, 120, 120) });
	radarModes.insert(64, RadarModeDef{ "RWS (search)",   QColor(40, 120, 255) });
	radarModes.insert(68, RadarModeDef{ "TWS (track)",    QColor(255, 170, 0) });
	radarModes.insert(72, RadarModeDef{ "STT (lock)",     QColor(230, 30, 30) });
	radarModes.insert(76, RadarModeDef{ "Combat",         QColor(200, 0, 120) });

	weaponEngagedThreshold = 1.0;
	weaponEngagedColor     = QColor(200, 0, 0);
	weaponEngagedLabel     = "ENGAGED";
	weaponIdleLabel        = "-";

	targetColor          = QColor(230, 30, 30);
	targetMarkerRadiusPx = 7.0;

	gearDownLabel = "DOWN";  gearUpLabel = "up";
	wowGroundLabel = "ON GROUND"; wowAirLabel = "airborne";
	slatsOutLabel = "OUT";   slatsInLabel = "in";

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
		} else if (section == "weapon") {
			if (key == "engaged_threshold") weaponEngagedThreshold = val.toDouble();
			else if (key == "engaged_color") weaponEngagedColor = parseColor(val, weaponEngagedColor);
			else if (key == "engaged_label") weaponEngagedLabel = val;
			else if (key == "idle_label")    weaponIdleLabel = val;
		} else if (section == "contact") {
			if (key == "target_color")      targetColor = parseColor(val, targetColor);
			else if (key == "target_radius_px") targetMarkerRadiusPx = val.toDouble();
		} else if (section == "labels") {
			if (key == "gear_down")       gearDownLabel = val;
			else if (key == "gear_up")    gearUpLabel = val;
			else if (key == "wow_ground") wowGroundLabel = val;
			else if (key == "wow_air")    wowAirLabel = val;
			else if (key == "slats_out")  slatsOutLabel = val;
			else if (key == "slats_in")   slatsInLabel = val;
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
