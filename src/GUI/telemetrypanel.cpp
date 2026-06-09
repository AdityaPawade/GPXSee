#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QHeaderView>
#include <QIcon>
#include <QPixmap>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include "collapsiblesection.h"
#include "mapview.h"
#include "telemetrypanel.h"
#include "vizconfig.h"

TelemetryPanel::TelemetryPanel(QWidget *parent) : QScrollArea(parent)
{
	const VizConfig &cfg = VizConfig::instance();
	QWidget *body = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(body);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	_flightCombo = new QComboBox(body);
	_flightCombo->setVisible(false);
	connect(_flightCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	  this, [this](int index) {
		emit flightChanged(index);
		reloadOverlayState();
		reloadTrackColor();
		applyCapabilities();
	});
	layout->addWidget(_flightCombo);

	_flightSection = new CollapsibleSection(cfg.panelTitle("flight"), body);
	_attitudeSection = new CollapsibleSection(cfg.panelTitle("attitude"), body);
	_engineSection = new CollapsibleSection(cfg.panelTitle("engine_fuel"), body);
	_discretesSection = new CollapsibleSection(cfg.panelTitle("discretes"), body);
	_radarSection = new CollapsibleSection(cfg.panelTitle("radar"), body);
	_targetsSection = new CollapsibleSection(cfg.panelTitle("targets_contacts"), body);
	_beaconSection = new CollapsibleSection(cfg.panelTitle("nav_beacon"), body);

	_flightTable = createTable(QStringList() << "track_name" << "position"
	  << "airspeed" << "vspeed");
	_attitudeTable = createTable(QStringList() << "yaw" << "pitch" << "roll"
	  << "roll_rate" << "yaw_rate");
	_engineTable = createTable(QStringList() << "fuel");
	_discretesTable = createTable(QStringList() << "gear" << "wow"
	  << "auto_slats" << "event");
	_radarTable = createTable(QStringList() << "sensor_mode" << "scan_width"
	  << "scan_program" << "look_az");
	_targetsTable = createTable(QStringList() << "track_bearing"
	  << "track_range" << "bearing_abs" << "contact_bearing"
	  << "contact_range" << "contact_alt");
	_beaconTable = createTable(QStringList() << "beacon_bearing"
	  << "beacon_bearing_rel" << "beacon_range");

	_flightSection->setContent(_flightTable);
	_attitudeSection->setContent(_attitudeTable);
	_engineSection->setContent(_engineTable);
	_discretesSection->setContent(_discretesTable);
	_radarSection->setContent(_radarTable);
	_targetsSection->setContent(_targetsTable);
	_beaconSection->setContent(_beaconTable);

	addOverlay(_radarSection, "radar_fov", MapView::RadarFov);
	addOverlay(_radarSection, "look_ray", MapView::LookRay);
	addOverlay(_targetsSection, "targets", MapView::Contacts);
	addOverlay(_beaconSection, "beacon", MapView::Beacon);

	// Per-track colour picker in the Flight section header. Opens QColorDialog
	// and recolours the selected track's polyline, graphs and overlays.
	_colorButton = new QToolButton(_flightSection);
	_colorButton->setToolTip(cfg.fieldLabel("track_color"));
	_colorButton->setAutoRaise(true);
	_colorButton->setFocusPolicy(Qt::NoFocus);
	_swatchColor = QColor();
	setSwatchColor(QColor());
	_flightSection->addHeaderWidget(_colorButton);
	connect(_colorButton, &QToolButton::clicked, this,
	  &TelemetryPanel::pickTrackColor);

	layout->addWidget(_flightSection);
	layout->addWidget(_attitudeSection);
	layout->addWidget(_engineSection);
	layout->addWidget(_discretesSection);
	layout->addWidget(_radarSection);
	layout->addWidget(_targetsSection);
	layout->addWidget(_beaconSection);
	layout->addStretch();

	setWidget(body);
	setWidgetResizable(true);
	setFrameShape(QFrame::NoFrame);
	clear();
}

void TelemetryPanel::setOverlayStateProvider(
  const std::function<bool(int, int)> &provider)
{
	_overlayStateProvider = provider;
	reloadOverlayState();
}

void TelemetryPanel::setCapabilityProvider(
  const std::function<bool(int, int)> &provider)
{
	_capabilityProvider = provider;
	applyCapabilities();
}

void TelemetryPanel::setTrackColorProvider(
  const std::function<QColor(int)> &provider)
{
	_trackColorProvider = provider;
	reloadTrackColor();
}

void TelemetryPanel::setSwatchColor(const QColor &color)
{
	_swatchColor = color;

	// Draw a small filled swatch as the button icon (framed when no colour yet).
	QPixmap pm(16, 16);
	pm.fill(color.isValid() ? color : QColor(Qt::transparent));
	if (color.isValid()) {
		_colorButton->setEnabled(true);
	} else
		_colorButton->setEnabled(false);
	_colorButton->setIcon(QIcon(pm));
}

void TelemetryPanel::reloadTrackColor()
{
	int idx = _flightCombo->currentIndex();
	QColor c = (_trackColorProvider && idx >= 0)
	  ? _trackColorProvider(idx) : QColor();
	setSwatchColor(c);
}

void TelemetryPanel::pickTrackColor()
{
	int idx = _flightCombo->currentIndex();
	if (idx < 0)
		return;

	QColor initial(_swatchColor.isValid() ? _swatchColor : Qt::white);
	QColor chosen(QColorDialog::getColor(initial, this,
	  VizConfig::instance().fieldLabel("track_color")));
	if (!chosen.isValid())
		return;

	setSwatchColor(chosen);
	emit trackColorChanged(idx, chosen);
}

void TelemetryPanel::applyCapabilities()
{
	int idx = _flightCombo->currentIndex();
	if (idx < 0) {
		clear();
		return;
	}
	// Flight is always relevant once a track is selected. Other sections are
	// shown iff the whole track exposes that group anywhere (persistent).
	bool all = !_capabilityProvider;
	_flightSection->setVisible(true);
	_attitudeSection->setVisible(all || _capabilityProvider(idx, CapAttitude));
	_engineSection->setVisible(all || _capabilityProvider(idx, CapEngine));
	_discretesSection->setVisible(all || _capabilityProvider(idx, CapDiscretes));
	_radarSection->setVisible(all || _capabilityProvider(idx, CapRadar));
	_targetsSection->setVisible(all || _capabilityProvider(idx, CapTargets));
	_beaconSection->setVisible(all || _capabilityProvider(idx, CapBeacon));
}

QTableWidget *TelemetryPanel::createTable(const QStringList &keys) const
{
	const VizConfig &cfg = VizConfig::instance();
	QTableWidget *table = new QTableWidget(keys.size(), 2);
	table->setHorizontalHeaderLabels(QStringList()
	  << cfg.fieldLabel("parameter") << cfg.fieldLabel("value"));
	table->verticalHeader()->setVisible(false);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionMode(QAbstractItemView::NoSelection);
	table->setFocusPolicy(Qt::NoFocus);
	table->setAlternatingRowColors(true);
	table->horizontalHeader()->setSectionResizeMode(0,
	  QHeaderView::ResizeToContents);
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

	for (int i = 0; i < keys.size(); i++) {
		QTableWidgetItem *key = new QTableWidgetItem(cfg.fieldLabel(keys.at(i)));
		QFont f(key->font()); f.setBold(true); key->setFont(f);
		table->setItem(i, 0, key);
		table->setItem(i, 1, new QTableWidgetItem(cfg.missingValueLabel));
	}
	table->resizeRowsToContents();
	table->setMinimumHeight(table->horizontalHeader()->height()
	  + table->verticalHeader()->length() + 4);
	return table;
}

QCheckBox *TelemetryPanel::addOverlay(CollapsibleSection *section,
  const QString &key, int type)
{
	QCheckBox *box = new QCheckBox(VizConfig::instance().fieldLabel(key), section);
	_overlayChecks.insert(type, box);
	section->addHeaderWidget(box);
	connect(box, &QCheckBox::toggled, this, [this, type](bool checked) {
		emit overlayToggled(_flightCombo->currentIndex(), type, checked);
	});
	return box;
}

void TelemetryPanel::set(QTableWidget *table, int row, const QString &value,
  const QColor &color) const
{
	QTableWidgetItem *it = table->item(row, 1);
	if (!it)
		return;
	it->setText(value);
	if (color.isValid()) {
		it->setForeground(color);
		QFont f(it->font()); f.setBold(true); it->setFont(f);
	} else {
		it->setForeground(table->palette().text().color());
		QFont f(it->font()); f.setBold(false); it->setFont(f);
	}
}

QString TelemetryPanel::num(qreal v, int prec, const QString &unit) const
{
	if (std::isnan(v))
		return VizConfig::instance().missingValueLabel;
	return QString::number(v, 'f', prec) + (unit.isEmpty()
	  ? QString() : QString(" ") + unit);
}

QString TelemetryPanel::rangeKm(qreal meters) const
{
	if (std::isnan(meters))
		return VizConfig::instance().missingValueLabel;
	return QString::number(meters / 1000.0, 'f', 1) + " km";
}

QString TelemetryPanel::bearingPair(qreal rel, qreal abs) const
{
	if (std::isnan(rel))
		return VizConfig::instance().missingValueLabel;
	return QString("%1 / %2").arg(num(rel, 1, "deg")).arg(num(abs, 1, "deg"));
}

QString TelemetryPanel::boolStr(int v, const QString &on,
  const QString &off) const
{
	return (v < 0) ? VizConfig::instance().missingValueLabel : (v ? on : off);
}

QString TelemetryPanel::withRaw(const QString &value, qreal raw) const
{
	if (std::isnan(raw) || value == VizConfig::instance().missingValueLabel)
		return value;
	// Raw source integers are whole numbers; show without a fractional part.
	return QString("%1 (%2)").arg(value,
	  QString::number((qlonglong)llround(raw)));
}

QString TelemetryPanel::withRawHex(const QString &value, int raw) const
{
	if (raw < 0 || value == VizConfig::instance().missingValueLabel)
		return value;
	return QString("%1 (0x%2)").arg(value,
	  QString::number(raw, 16).toUpper());
}

void TelemetryPanel::updateSection(CollapsibleSection *section,
  QTableWidget *table, const QStringList &values, const QList<QColor> &colors,
  bool visible)
{
	// Visibility is decided once per track by applyCapabilities() (whole-track
	// scan); the per-cursor update only refreshes values, never hides a section.
	Q_UNUSED(section);
	Q_UNUSED(visible);
	for (int i = 0; i < values.size(); i++)
		set(table, i, values.at(i), i < colors.size() ? colors.at(i) : QColor());
	table->resizeRowsToContents();
	table->setMinimumHeight(table->horizontalHeader()->height()
	  + table->verticalHeader()->length() + 4);
}

void TelemetryPanel::updateTelemetry(const QString &name,
  const Coordinates &pos, const Telemetry &t)
{
	const VizConfig &cfg = VizConfig::instance();
	// Every decoded value in the PANEL is shown with its raw source integer in
	// parentheses (withRaw / withRawHex); absent raw -> value shown unchanged.
	// Map overlays are deliberately left untouched (no parentheses on the map).
	bool haveFlight = pos.isValid() || !std::isnan(t.airspeed)
	  || !std::isnan(t.airspeedKt) || !std::isnan(t.vspeed);
	updateSection(_flightSection, _flightTable, QStringList()
	  << (name.isEmpty() ? cfg.missingValueLabel : name)
	  << (pos.isValid() ? QString("%1, %2").arg(pos.lat(), 0, 'f', 5)
		.arg(pos.lon(), 0, 'f', 5) : cfg.missingValueLabel)
	  << withRaw(std::isnan(t.airspeedKt) ? num(t.airspeed, 0, "km/h")
		: num(t.airspeedKt, 0, "kt"), t.airspeedRawVal)
	  << withRaw(num(t.vspeed, 1, "m/s"), t.vspeedRaw),
	  QList<QColor>(), haveFlight);

	bool haveAttitude = !std::isnan(t.yaw) || !std::isnan(t.pitch)
	  || !std::isnan(t.roll) || !std::isnan(t.rollRate)
	  || !std::isnan(t.yawRate);
	updateSection(_attitudeSection, _attitudeTable, QStringList()
	  << withRaw(num(t.yaw, 1, "deg"), t.yawRaw)
	  << withRaw(num(t.pitch, 1, "deg"), t.pitchRaw)
	  << withRaw(num(t.roll, 1, "deg"), t.rollRaw)
	  << withRaw(num(t.rollRate, 1, "deg/s"), t.rollRateRaw)
	  << withRaw(num(t.yawRate, 1, "deg/s"), t.yawRateRaw),
	  QList<QColor>(), haveAttitude);

	bool haveEngine = !std::isnan(t.fuelKg) || !std::isnan(t.fuelPct);
	updateSection(_engineSection, _engineTable, QStringList()
	  << (std::isnan(t.fuelKg) ? num(t.fuelPct, 1, "%")
		: withRaw(num(t.fuelKg, 0, "kg"), t.fuelRaw)),
	  QList<QColor>(), haveEngine);

	bool eventActive = !std::isnan(t.event) && t.event >= cfg.eventActiveThreshold;
	bool haveDiscretes = t.gear >= 0 || t.wow >= 0 || t.autoSlats >= 0
	  || !std::isnan(t.event);
	// Gear and WOW both derive from the same wow_raw source byte; auto-slats from
	// its own raw byte. Discrete states are shown with their source byte in hex.
	updateSection(_discretesSection, _discretesTable, QStringList()
	  << withRawHex(boolStr(t.gear, cfg.gearDownLabel, cfg.gearUpLabel), t.wowRaw)
	  << withRawHex(boolStr(t.wow, cfg.wowGroundLabel, cfg.wowAirLabel), t.wowRaw)
	  << withRawHex(boolStr(t.autoSlats, cfg.slatsOutLabel, cfg.slatsInLabel),
		t.autoSlatsRaw)
	  << (eventActive ? cfg.eventActiveLabel : cfg.eventIdleLabel),
	  QList<QColor>() << QColor() << QColor() << QColor()
	  << (eventActive ? cfg.eventActiveColor : QColor()), haveDiscretes);

	bool haveState = !std::isnan(t.radarState);
	bool radarOn = haveState ? ((int)(t.radarState + 0.5) >= 1)
	  : cfg.radarOn(t.radarMode);
	bool lockNoScan = haveState && (int)(t.radarState + 0.5) == 2
	  && (std::isnan(t.radarScan) || t.radarScan <= 0);
	bool haveRadar = !std::isnan(t.radarState) || !std::isnan(t.radarMode)
	  || !std::isnan(t.radarScan) || !std::isnan(t.radarScanProgram)
	  || !std::isnan(t.lookAzimuth) || !std::isnan(t.radarAz);
	// Mode shows the state/mode NAME with the raw mode byte (e.g. "SEARCH (68)").
	updateSection(_radarSection, _radarTable, QStringList()
	  << withRaw(haveState ? cfg.radarStateName(t.radarState)
		: cfg.radarModeName(t.radarMode), t.radarMode)
	  << (lockNoScan ? cfg.lockScanLabel
		: std::isnan(t.radarScan) ? cfg.missingValueLabel
		: QString("%1 deg").arg(t.radarScan, 0, 'f', 0))
	  << num(t.radarScanProgram, 0)
	  << num(std::isnan(t.radarAz) ? t.lookAzimuth : t.radarAz, 1, "deg"),
	  QList<QColor>() << (radarOn ? cfg.radarActiveColor : QColor())
	  << QColor() << QColor() << cfg.lookRayColor, haveRadar);

	qreal trackAbs = (!std::isnan(t.yaw) && !std::isnan(t.trackBearing))
	  ? t.yaw + t.trackBearing : NAN;
	qreal contactAbs = (!std::isnan(t.yaw) && !std::isnan(t.contactBearing))
	  ? t.yaw + t.contactBearing : NAN;
	bool hasTrack = !std::isnan(t.trackBearing) && !std::isnan(t.trackRange);
	bool hasContact = !std::isnan(t.contactBearing) && !std::isnan(t.contactRange);
	updateSection(_targetsSection, _targetsTable, QStringList()
	  << (hasTrack ? withRaw(bearingPair(t.trackBearing, trackAbs),
		t.trackBearingRaw) : cfg.missingValueLabel)
	  << (hasTrack ? withRaw(rangeKm(t.trackRange), t.trackRangeRaw)
		: cfg.missingValueLabel)
	  << (hasTrack ? num(trackAbs, 1, "deg") : cfg.missingValueLabel)
	  << (hasContact ? withRaw(bearingPair(t.contactBearing, contactAbs),
		t.contactBearingRaw) : cfg.missingValueLabel)
	  << (hasContact ? withRaw(rangeKm(t.contactRange), t.contactRangeRaw)
		: cfg.missingValueLabel)
	  << (hasContact ? withRaw(num(t.contactAltitude, 0, "m"),
		t.contactAltitudeRaw) : cfg.missingValueLabel),
	  QList<QColor>() << (hasTrack ? cfg.trackColor : QColor())
	  << (hasTrack ? cfg.trackColor : QColor())
	  << (hasTrack ? cfg.trackColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor()), hasTrack || hasContact);

	bool haveBeacon = !std::isnan(t.beaconBearing) || !std::isnan(t.beaconBearingRel)
	  || !std::isnan(t.beaconRange);
	updateSection(_beaconSection, _beaconTable, QStringList()
	  << withRaw(num(t.beaconBearing, 1, "deg"), t.beaconBearingRaw)
	  << withRaw(num(t.beaconBearingRel, 1, "deg"), t.beaconBearingRelRaw)
	  << withRaw(std::isnan(t.beaconRange) ? cfg.missingValueLabel
		: QString::number(t.beaconRange, 'f', 1) + " km", t.beaconRangeRaw),
	  QList<QColor>() << cfg.beaconColor << cfg.beaconColor << cfg.beaconColor,
	  haveBeacon);
}

void TelemetryPanel::clear()
{
	QList<CollapsibleSection*> sections;
	sections << _flightSection << _attitudeSection << _engineSection
	  << _discretesSection << _radarSection << _targetsSection << _beaconSection;
	for (int i = 0; i < sections.size(); i++)
		sections.at(i)->setVisible(false);
}

void TelemetryPanel::setFlights(const QStringList &names)
{
	int cur = _flightCombo->currentIndex();
	QSignalBlocker blocker(_flightCombo);
	_flightCombo->clear();
	for (int i = 0; i < names.size(); i++) {
		QString n(names.at(i).isEmpty() ? QString("Track %1").arg(i + 1)
		  : names.at(i));
		_flightCombo->addItem(QString("%1: %2").arg(i + 1).arg(n));
	}
	if (cur >= 0 && cur < names.size())
		_flightCombo->setCurrentIndex(cur);
	else if (!names.isEmpty())
		_flightCombo->setCurrentIndex(0);
	_flightCombo->setVisible(names.size() > 1);
	reloadOverlayState();
	reloadTrackColor();
	applyCapabilities();
}

void TelemetryPanel::reloadOverlayState()
{
	for (QHash<int, QCheckBox*>::const_iterator it = _overlayChecks.constBegin();
	  it != _overlayChecks.constEnd(); ++it) {
		QSignalBlocker blocker(it.value());
		bool enabled = _overlayStateProvider
		  ? _overlayStateProvider(_flightCombo->currentIndex(), it.key()) : false;
		it.value()->setChecked(enabled);
	}
}
