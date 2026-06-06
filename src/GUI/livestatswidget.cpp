#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <cmath>
#include "livestatswidget.h"
#include "vizconfig.h"

static const char *ROW_LABELS[] = {
	"Aircraft", "Position", "Heading (yaw)", "Pitch", "Roll", "Airspeed",
	"Vert. speed", "Roll rate", "Yaw rate", "Radar mode", "Radar scan",
	"Contact bearing", "Contact range", "Nav bearing", "Nav range", "Fuel",
	"Gear", "Weight-on-wheels", "Auto-slats", "Weapon"
};

// Radar-mode names + the engaged/label/colour semantics now come from
// VizConfig (viz.cfg). Nothing here is hard-coded — see updateTelemetry().

LiveStatsWidget::LiveStatsWidget(QWidget *parent) : QWidget(parent)
{
	_flightCombo = new QComboBox(this);
	_flightCombo->setVisible(false);
	connect(_flightCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	  this, &LiveStatsWidget::flightChanged);

	_table = new QTableWidget(RowCount, 2, this);
	_table->setHorizontalHeaderLabels(QStringList() << "Parameter" << "Value");
	_table->verticalHeader()->setVisible(false);
	_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	_table->setSelectionMode(QAbstractItemView::NoSelection);
	_table->setFocusPolicy(Qt::NoFocus);
	_table->setAlternatingRowColors(true);
	_table->horizontalHeader()->setSectionResizeMode(0,
	  QHeaderView::ResizeToContents);
	_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	for (int i = 0; i < RowCount; i++) {
		QTableWidgetItem *key = new QTableWidgetItem(QString(ROW_LABELS[i]));
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
		return QString("-");
	return QString::number(v, 'f', prec) + (unit.isEmpty()
	  ? QString() : QString(" ") + unit);
}

static QString boolStr(int v, const QString &on, const QString &off)
{
	return (v < 0) ? QString("-") : (v ? on : off);
}

void LiveStatsWidget::updateTelemetry(const QString &name,
  const Coordinates &pos, const Telemetry &t)
{
	set(RAircraft, name.isEmpty() ? QString("-") : name);
	set(RPos, pos.isValid() ? QString("%1, %2").arg(pos.lat(), 0, 'f', 5)
	  .arg(pos.lon(), 0, 'f', 5) : QString("-"));
	set(RYaw, num(t.yaw, 1, "deg"));
	set(RPitch, num(t.pitch, 1, "deg"));
	set(RRoll, num(t.roll, 1, "deg"));
	set(RAirspeed, num(t.airspeed, 0, "km/h"));
	set(RVspeed, num(t.vspeed, 1, "m/s"));
	set(RRollRate, num(t.rollRate, 1, "deg/s"));
	set(RYawRate, num(t.yawRate, 1, "deg/s"));

	const VizConfig &cfg = VizConfig::instance();
	bool radarOn = cfg.radarOn(t.radarMode);
	set(RRadarMode, cfg.radarModeName(t.radarMode),
	  radarOn ? cfg.radarActiveColor : QColor());
	set(RRadarScan, std::isnan(t.radarScan) ? QString("-")
	  : QString("+-%1 deg (sector %2)").arg(t.radarScan / 2.0, 0, 'f', 0)
	  .arg(t.radarScan, 0, 'f', 0));

	set(RContactBrg, num(t.contactBearing, 1, "deg"));
	set(RContactRng, std::isnan(t.contactRange) ? QString("-")
	  : QString::number(t.contactRange / 1000.0, 'f', 1) + " km");
	set(RNavBrg, num(t.navBearing, 1, "deg"));
	set(RNavRng, num(t.navRange, 0));
	set(RFuel, num(t.fuelPct, 1, "%"));
	set(RGear, boolStr(t.gear, cfg.gearDownLabel, cfg.gearUpLabel));
	set(RWow, boolStr(t.wow, cfg.wowGroundLabel, cfg.wowAirLabel));
	set(RSlats, boolStr(t.autoSlats, cfg.slatsOutLabel, cfg.slatsInLabel));

	bool firing = !std::isnan(t.weapon) && t.weapon >= cfg.weaponEngagedThreshold;
	set(RWeapon, firing ? cfg.weaponEngagedLabel : cfg.weaponIdleLabel,
	  firing ? cfg.weaponEngagedColor : QColor());
}

void LiveStatsWidget::clear()
{
	for (int i = 0; i < RowCount; i++)
		set(i, "-");
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
