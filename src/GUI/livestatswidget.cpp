#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <cmath>
#include "livestatswidget.h"
#include "vizconfig.h"

static const char *ROW_KEYS[] = {
	"track_name", "position", "yaw", "pitch", "roll", "airspeed",
	"vspeed", "roll_rate", "yaw_rate", "sensor_mode", "scan_width",
	"contact_bearing", "contact_range", "nav_bearing", "nav_range", "fuel",
	"gear", "wow", "auto_slats", "event"
};

LiveStatsWidget::LiveStatsWidget(QWidget *parent) : QWidget(parent)
{
	const VizConfig &cfg = VizConfig::instance();
	_flightCombo = new QComboBox(this);
	_flightCombo->setVisible(false);
	connect(_flightCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	  this, &LiveStatsWidget::flightChanged);

	_table = new QTableWidget(RowCount, 2, this);
	_table->setHorizontalHeaderLabels(QStringList()
	  << cfg.fieldLabel("parameter") << cfg.fieldLabel("value"));
	_table->verticalHeader()->setVisible(false);
	_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	_table->setSelectionMode(QAbstractItemView::NoSelection);
	_table->setFocusPolicy(Qt::NoFocus);
	_table->setAlternatingRowColors(true);
	_table->horizontalHeader()->setSectionResizeMode(0,
	  QHeaderView::ResizeToContents);
	_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	for (int i = 0; i < RowCount; i++) {
		QTableWidgetItem *key = new QTableWidgetItem(
		  cfg.fieldLabel(QString(ROW_KEYS[i])));
		QFont f(key->font()); f.setBold(true); key->setFont(f);
		_table->setItem(i, 0, key);
		_table->setItem(i, 1, new QTableWidgetItem("-"));
	}
	_table->resizeRowsToContents();

	QVBoxLayout *l = new QVBoxLayout();
	l->setContentsMargins(2, 2, 2, 2);
	l->addWidget(_flightCombo);
	l->addWidget(_table);
	setLayout(l);

	clear();
}

void LiveStatsWidget::set(int row, const QString &value, const QColor &color)
{
	QTableWidgetItem *it = _table->item(row, 1);
	if (!it)
		return;
	it->setText(value);
	if (color.isValid()) {
		it->setForeground(color);
		QFont f(it->font()); f.setBold(true); it->setFont(f);
	} else {
		it->setForeground(_table->palette().text().color());
		QFont f(it->font()); f.setBold(false); it->setFont(f);
	}
}

static QString num(qreal v, int prec, const QString &unit = QString())
{
	if (std::isnan(v))
		return VizConfig::instance().missingValueLabel;
	return QString::number(v, 'f', prec) + (unit.isEmpty()
	  ? QString() : QString(" ") + unit);
}

static QString boolStr(int v, const QString &on, const QString &off)
{
	return (v < 0) ? VizConfig::instance().missingValueLabel : (v ? on : off);
}

void LiveStatsWidget::updateTelemetry(const QString &name,
  const Coordinates &pos, const Telemetry &t)
{
	const VizConfig &cfg = VizConfig::instance();
	set(RAircraft, name.isEmpty() ? cfg.missingValueLabel : name);
	set(RPos, pos.isValid() ? QString("%1, %2").arg(pos.lat(), 0, 'f', 5)
	  .arg(pos.lon(), 0, 'f', 5) : cfg.missingValueLabel);
	set(RYaw, num(t.yaw, 1, "deg"));
	set(RPitch, num(t.pitch, 1, "deg"));
	set(RRoll, num(t.roll, 1, "deg"));
	set(RAirspeed, std::isnan(t.airspeedKt) ? num(t.airspeed, 0, "km/h")
	  : num(t.airspeedKt, 0, "kt"));
	set(RVspeed, num(t.vspeed, 1, "m/s"));
	set(RRollRate, num(t.rollRate, 1, "deg/s"));
	set(RYawRate, num(t.yawRate, 1, "deg/s"));

	// Prefer the continuous radar STATE (OFF/SEARCH/LOCK) when present: a raw
	// search-mode field can read OFF while locked, so radarState is reliable.
	bool haveState = !std::isnan(t.radarState);
	bool radarOn = haveState ? ((int)(t.radarState + 0.5) >= 1)
	  : cfg.radarOn(t.radarMode);
	set(RRadarMode, haveState ? cfg.radarStateName(t.radarState)
	  : cfg.radarModeName(t.radarMode),
	  radarOn ? cfg.radarActiveColor : QColor());
	// During a lock there is no sector scan (single-target track), so show the
	// track-gate label instead of a misleading "+-0 deg".
	bool lockNoScan = haveState && (int)(t.radarState + 0.5) == 2
	  && (std::isnan(t.radarScan) || t.radarScan <= 0);
	set(RRadarScan, lockNoScan ? cfg.lockScanLabel
	  : std::isnan(t.radarScan) ? cfg.missingValueLabel
	  : QString("+-%1 deg (sector %2)").arg(t.radarScan / 2.0, 0, 'f', 0)
	  .arg(t.radarScan, 0, 'f', 0));

	set(RContactBrg, num(t.contactBearing, 1, "deg"));
	set(RContactRng, std::isnan(t.contactRange) ? cfg.missingValueLabel
	  : QString::number(t.contactRange / 1000.0, 'f', 1) + " km");
	set(RNavBrg, num(t.navBearing, 1, "deg"));
	set(RNavRng, num(t.navRange, 0));
	set(RFuel, std::isnan(t.fuelKg) ? num(t.fuelPct, 1, "%")
	  : num(t.fuelKg, 0, "kg"));
	set(RGear, boolStr(t.gear, cfg.gearDownLabel, cfg.gearUpLabel));
	set(RWow, boolStr(t.wow, cfg.wowGroundLabel, cfg.wowAirLabel));
	set(RSlats, boolStr(t.autoSlats, cfg.slatsOutLabel, cfg.slatsInLabel));

	bool active = !std::isnan(t.event) && t.event >= cfg.eventActiveThreshold;
	set(REvent, active ? cfg.eventActiveLabel : cfg.eventIdleLabel,
	  active ? cfg.eventActiveColor : QColor());
}

void LiveStatsWidget::clear()
{
	for (int i = 0; i < RowCount; i++)
		set(i, VizConfig::instance().missingValueLabel);
}

void LiveStatsWidget::setFlights(const QStringList &names)
{
	int cur = _flightCombo->currentIndex();
	_flightCombo->blockSignals(true);
	_flightCombo->clear();
	for (int i = 0; i < names.size(); i++) {
		QString n(names.at(i).isEmpty() ? QString("Track %1").arg(i + 1)
		  : names.at(i));
		_flightCombo->addItem(QString("%1: %2").arg(i + 1).arg(n));
	}
	if (cur >= 0 && cur < names.size())
		_flightCombo->setCurrentIndex(cur);
	_flightCombo->blockSignals(false);
	_flightCombo->setVisible(names.size() > 1);
}
