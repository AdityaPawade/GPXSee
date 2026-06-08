#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <cmath>
#include "sensortargetswidget.h"
#include "vizconfig.h"

static QString num(qreal v, int prec, const QString &unit = QString())
{
	const VizConfig &cfg = VizConfig::instance();
	if (std::isnan(v))
		return cfg.missingValueLabel;
	return QString::number(v, 'f', prec) + (unit.isEmpty()
	  ? QString() : QString(" ") + unit);
}

SensorTargetsWidget::SensorTargetsWidget(Mode mode, QWidget *parent)
  : QWidget(parent), _mode(mode)
{
	const VizConfig &cfg = VizConfig::instance();
	int rows = (_mode == SensorMode) ? 4 : 6;
	_table = new QTableWidget(rows, 2, this);
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

	QStringList labels;
	if (_mode == SensorMode) {
		labels << cfg.fieldLabel("sensor_mode") << cfg.fieldLabel("scan_width")
		  << cfg.fieldLabel("scan_program") << cfg.fieldLabel("look_az");
	} else {
		labels << cfg.fieldLabel("track_bearing")
		  << cfg.fieldLabel("track_range") << cfg.fieldLabel("bearing_abs")
		  << cfg.fieldLabel("contact_bearing") << cfg.fieldLabel("contact_range")
		  << cfg.fieldLabel("contact_alt");
	}

	for (int i = 0; i < labels.size(); i++) {
		QTableWidgetItem *key = new QTableWidgetItem(labels.at(i));
		QFont f(key->font()); f.setBold(true); key->setFont(f);
		_table->setItem(i, 0, key);
		_table->setItem(i, 1, new QTableWidgetItem(cfg.missingValueLabel));
	}
	_table->resizeRowsToContents();

	QVBoxLayout *l = new QVBoxLayout();
	l->setContentsMargins(2, 2, 2, 2);
	l->addWidget(_table);
	setLayout(l);

	clear();
}

void SensorTargetsWidget::set(int row, const QString &value, const QColor &color)
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

QString SensorTargetsWidget::bearingPair(qreal rel, qreal abs) const
{
	const VizConfig &cfg = VizConfig::instance();
	if (std::isnan(rel))
		return cfg.missingValueLabel;
	return QString("%1 / %2")
	  .arg(num(rel, 1, "deg"))
	  .arg(num(abs, 1, "deg"));
}

QString SensorTargetsWidget::range(qreal meters) const
{
	const VizConfig &cfg = VizConfig::instance();
	if (std::isnan(meters))
		return cfg.missingValueLabel;
	return QString::number(meters / 1000.0, 'f', 1) + " km";
}

void SensorTargetsWidget::updateTelemetry(const QString &name,
  const Coordinates &pos, const Telemetry &t)
{
	Q_UNUSED(name);
	Q_UNUSED(pos);
	const VizConfig &cfg = VizConfig::instance();

	if (_mode == SensorMode) {
		// Prefer the continuous radar STATE (OFF/SEARCH/LOCK): a raw search-mode
		// field can read OFF while locked, so radarState is the reliable indicator.
		bool haveState = !std::isnan(t.radarState);
		bool active = haveState ? ((int)(t.radarState + 0.5) >= 1)
		  : cfg.radarOn(t.radarMode);
		set(0, haveState ? cfg.radarStateName(t.radarState)
		  : cfg.radarModeName(t.radarMode),
		  active ? cfg.radarActiveColor : QColor());
		bool lockNoScan = haveState && (int)(t.radarState + 0.5) == 2
		  && (std::isnan(t.radarScan) || t.radarScan <= 0);
		set(1, lockNoScan ? cfg.lockScanLabel
		  : std::isnan(t.radarScan) ? cfg.missingValueLabel
		  : QString("%1 deg").arg(t.radarScan, 0, 'f', 0));
		set(2, num(t.radarScanProgram, 0));
		// Effective antenna azimuth: lock azimuth in LOCK, search azimuth otherwise.
		set(3, num(std::isnan(t.radarAz) ? t.lookAzimuth : t.radarAz, 1, "deg"),
		  cfg.lookRayColor);
		return;
	}

	qreal trackAbs = (!std::isnan(t.yaw) && !std::isnan(t.trackBearing))
	  ? t.yaw + t.trackBearing : NAN;
	qreal contactAbs = (!std::isnan(t.yaw) && !std::isnan(t.contactBearing))
	  ? t.yaw + t.contactBearing : NAN;
	bool hasTrack = !std::isnan(t.trackBearing) && !std::isnan(t.trackRange);
	bool hasContact = !std::isnan(t.contactBearing) && !std::isnan(t.contactRange);

	set(0, hasTrack ? bearingPair(t.trackBearing, trackAbs)
	  : cfg.missingValueLabel, hasTrack ? cfg.trackColor : QColor());
	set(1, hasTrack ? range(t.trackRange) : cfg.missingValueLabel,
	  hasTrack ? cfg.trackColor : QColor());
	set(2, hasTrack ? num(trackAbs, 1, "deg") : cfg.missingValueLabel,
	  hasTrack ? cfg.trackColor : QColor());
	set(3, hasContact ? bearingPair(t.contactBearing, contactAbs)
	  : cfg.missingValueLabel, hasContact ? cfg.contactColor : QColor());
	set(4, hasContact ? range(t.contactRange) : cfg.missingValueLabel,
	  hasContact ? cfg.contactColor : QColor());
	set(5, hasContact ? num(t.contactAltitude, 0, "m") : cfg.missingValueLabel,
	  hasContact ? cfg.contactColor : QColor());
}

void SensorTargetsWidget::clear()
{
	const VizConfig &cfg = VizConfig::instance();
	for (int i = 0; i < _table->rowCount(); i++)
		set(i, cfg.missingValueLabel);
}
