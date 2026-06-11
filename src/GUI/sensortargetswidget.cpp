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
	int rows = (_mode == SensorMode) ? 4 : 5;
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
		labels << cfg.fieldLabel("sensor_mode") << cfg.fieldLabel("scan_mode")
		  << cfg.fieldLabel("search_az") << cfg.fieldLabel("lock_az");
	} else {
		labels << cfg.fieldLabel("track_bearing")
		  << cfg.fieldLabel("track_range")
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
		// Zhuk radar state (raw 0x145 byte: OFF/SEARCH/LOCK), per [radar_states].
		bool haveState = !std::isnan(t.radarState);
		int rstate = haveState ? (int)(t.radarState + 0.5) : 0;
		bool active = haveState && rstate >= 1;
		set(0, cfg.radarStateName(t.radarState),
		  active ? cfg.radarStateColor(t.radarState) : QColor());
		// Scan: pencil beam in lock, else a sector width from the scan-mode code.
		double scanDeg = cfg.scanModeWidthDeg(t.radarScanMode);
		set(1, rstate == 2 ? cfg.lockScanLabel
		  : std::isnan(t.radarScanMode) ? cfg.missingValueLabel
		  : std::isnan(scanDeg) ? QString("mode %1").arg(t.radarScanMode, 0, 'f', 0)
		  : scanDeg <= 0 ? cfg.lockScanLabel
		  : QString("+-%1 deg").arg(scanDeg / 2.0, 0, 'f', 0));
		// Search sweep azimuth (rel to nose) and lock azimuth (absolute).
		set(2, num(t.radarSearchAz, 1, "deg"), active ? cfg.lookRayColor : QColor());
		set(3, num(t.radarAz, 1, "deg"), cfg.lookRayColor);
		return;
	}

	// RAW TRUTH: targets are recorded as nose-relative bearing + range; shown as
	// recorded. No absolute bearing is computed (that needs heading fusion).
	bool hasTrack = !std::isnan(t.trackBearing) && !std::isnan(t.trackRange);
	bool hasContact = !std::isnan(t.contactBearing) && !std::isnan(t.contactRange);

	set(0, hasTrack ? num(t.trackBearing, 1, "deg")
	  : cfg.missingValueLabel, hasTrack ? cfg.trackColor : QColor());
	set(1, hasTrack ? range(t.trackRange) : cfg.missingValueLabel,
	  hasTrack ? cfg.trackColor : QColor());
	set(2, hasContact ? num(t.contactBearing, 1, "deg")
	  : cfg.missingValueLabel, hasContact ? cfg.contactColor : QColor());
	set(3, hasContact ? range(t.contactRange) : cfg.missingValueLabel,
	  hasContact ? cfg.contactColor : QColor());
	set(4, hasContact ? num(t.contactAltitude, 0, "m") : cfg.missingValueLabel,
	  hasContact ? cfg.contactColor : QColor());
}

void SensorTargetsWidget::clear()
{
	const VizConfig &cfg = VizConfig::instance();
	for (int i = 0; i < _table->rowCount(); i++)
		set(i, cfg.missingValueLabel);
}
