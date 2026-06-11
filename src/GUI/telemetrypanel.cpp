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
	_rwrSection = new CollapsibleSection(cfg.panelTitle("rwr"), body);
	_mawsSection = new CollapsibleSection(cfg.panelTitle("maws"), body);
	_beaconSection = new CollapsibleSection(cfg.panelTitle("nav_beacon"), body);

	_flightTable = createTable(QStringList() << "track_name" << "position"
	  << "ground_speed" << "vspeed");
	_attitudeTable = createTable(QStringList() << "yaw" << "pitch" << "roll"
	  << "roll_rate" << "yaw_rate");
	_engineTable = createTable(QStringList() << "fuel");
	_discretesTable = createTable(QStringList() << "gear" << "wow"
	  << "auto_slats" << "apch_mode" << "apch_scan" << "apch_prog");
	_radarTable = createTable(QStringList() << "sensor_mode" << "scan_mode"
	  << "search_az" << "lock_az" << "lock_az_rel");
	// Targets carry only nose-relative bearings; we show those raw and do NOT
	// add an absolute-bearing column (that would need heading fusion).
	_targetsTable = createTable(QStringList() << "track_bearing"
	  << "track_range" << "contact_bearing"
	  << "contact_range" << "contact_alt");
	// RWR (radar-warning receiver) discretes — raw recorded codes, panel-only
	// (no geometry to plot). Present on both jets; only the engaged one latches.
	_rwrTable = createTable(QStringList() << "rwr_status" << "rwr_phase"
	  << "rwr_code");
	// MAWS (missile-approach warning) — recorded incoming-missile bearing (abs)
	// + range; rendered on the map as a positioned threat diamond.
	_mawsTable = createTable(QStringList() << "missile_bearing" << "missile_range");
	_beaconTable = createTable(QStringList() << "beacon_bearing"
	  << "beacon_bearing_rel" << "beacon_range");

	_flightSection->setContent(_flightTable);
	_attitudeSection->setContent(_attitudeTable);
	_engineSection->setContent(_engineTable);
	_discretesSection->setContent(_discretesTable);
	_radarSection->setContent(_radarTable);
	_targetsSection->setContent(_targetsTable);
	_rwrSection->setContent(_rwrTable);
	_mawsSection->setContent(_mawsTable);
	_beaconSection->setContent(_beaconTable);

	addOverlay(_radarSection, "radar_fov", MapView::RadarFov);
	addOverlay(_radarSection, "look_ray", MapView::LookRay);
	// Targets/contacts markers: recorded bearing/range plotted at the recorded
	// heading. Values also shown raw in the Targets section.
	addOverlay(_targetsSection, "targets", MapView::Contacts);
	// MAWS overlay: positioned incoming-missile diamond (abs bearing + range).
	addOverlay(_mawsSection, "maws", MapView::Maws);
	// RWR overlay: a state annunciator ring around the jet (no bearing) — gold
	// when the receiver is idle, red double-ring when the threat latch fires.
	addOverlay(_rwrSection, "rwr_ring", MapView::Rwr);
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

	// Master "show all" toggle in the Flight header: forces every overlay
	// component the selected track exposes (radar FOV, look-ray, targets,
	// beacon) on or off at once, regardless of their individual state.
	_renderAllCheck = new QCheckBox(cfg.fieldLabel("render_all"), _flightSection);
	_renderAllCheck->setToolTip(cfg.fieldLabel("render_all"));
	_renderAllCheck->setFocusPolicy(Qt::NoFocus);
	_flightSection->addHeaderWidget(_renderAllCheck);
	connect(_renderAllCheck, &QCheckBox::toggled, this, [this](bool on) {
		for (QHash<int, QCheckBox*>::const_iterator it
		  = _overlayChecks.constBegin(); it != _overlayChecks.constEnd(); ++it) {
			if (it.value()->isVisibleTo(this))
				it.value()->setChecked(on);
		}
	});

	layout->addWidget(_flightSection);
	layout->addWidget(_attitudeSection);
	layout->addWidget(_engineSection);
	layout->addWidget(_discretesSection);
	layout->addWidget(_radarSection);
	layout->addWidget(_targetsSection);
	layout->addWidget(_rwrSection);
	layout->addWidget(_mawsSection);
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

void TelemetryPanel::syncRenderAll()
{
	// Master toggle reflects "all on": checked iff every overlay the current
	// track exposes (its section is shown) is enabled; disabled when the track
	// has no overlay components. Signal-blocked so this never drives the
	// children (only a user click on the master does).
	bool any = false, allOn = true;
	for (QHash<int, QCheckBox*>::const_iterator it = _overlayChecks.constBegin();
	  it != _overlayChecks.constEnd(); ++it) {
		if (!it.value()->isVisibleTo(this))
			continue;
		any = true;
		if (!it.value()->isChecked())
			allOn = false;
	}
	QSignalBlocker blocker(_renderAllCheck);
	_renderAllCheck->setEnabled(any);
	_renderAllCheck->setChecked(any && allOn);
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
	_rwrSection->setVisible(all || _capabilityProvider(idx, CapRWR));
	_mawsSection->setVisible(all || _capabilityProvider(idx, CapMAWS));
	_beaconSection->setVisible(all || _capabilityProvider(idx, CapBeacon));
	syncRenderAll();
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
		syncRenderAll();
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
	bool haveFlight = pos.isValid() || !std::isnan(t.groundSpeed)
	  || !std::isnan(t.vspeed);
	// ground_speed = recorded speed channel (raw 0x156); m/s -> km/h for display.
	updateSection(_flightSection, _flightTable, QStringList()
	  << (name.isEmpty() ? cfg.missingValueLabel : name)
	  << (pos.isValid() ? QString("%1, %2").arg(pos.lat(), 0, 'f', 5)
		.arg(pos.lon(), 0, 'f', 5) : cfg.missingValueLabel)
	  << withRaw(std::isnan(t.groundSpeed) ? cfg.missingValueLabel
		: num(t.groundSpeed * 3.6, 0, "km/h"), t.groundSpeedRaw)
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

	bool haveDiscretes = t.gear >= 0 || t.wow >= 0 || t.autoSlats >= 0
	  || !std::isnan(t.apchMode);
	// Gear and WOW both come from the same wow_raw source byte; auto-slats from
	// its own byte. Discrete states show the source byte in hex. The
	// approach-system codes follow as plain recorded values.
	updateSection(_discretesSection, _discretesTable, QStringList()
	  << withRawHex(boolStr(t.gear, cfg.gearDownLabel, cfg.gearUpLabel), t.wowRaw)
	  << withRawHex(boolStr(t.wow, cfg.wowGroundLabel, cfg.wowAirLabel), t.wowRaw)
	  << withRawHex(boolStr(t.autoSlats, cfg.slatsOutLabel, cfg.slatsInLabel),
		t.autoSlatsRaw)
	  << num(t.apchMode, 0) << num(t.apchScan, 0) << num(t.apchProg, 0),
	  QList<QColor>(), haveDiscretes);

	bool haveState = !std::isnan(t.radarState);
	int rstate = haveState ? (int)(t.radarState + 0.5) : 0;
	bool radarOn = haveState && rstate >= 1;
	bool lockNoScan = rstate == 2;   // single-target lock = pencil beam, no sector
	double scanDeg = cfg.scanModeWidthDeg(t.radarScanMode);
	bool haveRadar = !std::isnan(t.radarState) || !std::isnan(t.radarScanMode)
	  || !std::isnan(t.radarSearchAz) || !std::isnan(t.radarAz)
	  || !std::isnan(t.radarAzRel);
	// State NAME with the raw scan-mode code in parentheses; scan as a sector
	// width; the two azimuths shown only when present (lock vs search phase).
	updateSection(_radarSection, _radarTable, QStringList()
	  << cfg.radarStateName(t.radarState)
	  << (lockNoScan ? cfg.lockScanLabel
		: std::isnan(t.radarScanMode) ? cfg.missingValueLabel
		: std::isnan(scanDeg) ? QString("mode %1").arg(t.radarScanMode, 0, 'f', 0)
		: scanDeg <= 0 ? cfg.lockScanLabel
		: withRaw(QString("+-%1 deg").arg(scanDeg / 2.0, 0, 'f', 0),
		  t.radarScanMode))
	  << withRaw(num(t.radarSearchAz, 1, "deg"), t.radarSearchAzRaw)
	  << withRaw(num(t.radarAz, 1, "deg"), t.radarAzRaw)
	  << withRaw(num(t.radarAzRel, 1, "deg"), t.radarAzRelRaw),
	  QList<QColor>() << (radarOn ? cfg.radarStateColor(t.radarState) : QColor())
	  << QColor() << cfg.lookRayColor << cfg.lookRayColor << QColor(),
	  haveRadar);

	bool hasTrack = !std::isnan(t.trackBearing) && !std::isnan(t.trackRange);
	bool hasContact = !std::isnan(t.contactBearing) && !std::isnan(t.contactRange);
	// RAW TRUTH: track/contact are nose-relative bearings shown as recorded.
	updateSection(_targetsSection, _targetsTable, QStringList()
	  << (hasTrack ? withRaw(num(t.trackBearing, 1, "deg"), t.trackBearingRaw)
		: cfg.missingValueLabel)
	  << (hasTrack ? withRaw(rangeKm(t.trackRange), t.trackRangeRaw)
		: cfg.missingValueLabel)
	  << (hasContact ? withRaw(num(t.contactBearing, 1, "deg"), t.contactBearingRaw)
		: cfg.missingValueLabel)
	  << (hasContact ? withRaw(rangeKm(t.contactRange), t.contactRangeRaw)
		: cfg.missingValueLabel)
	  << (hasContact ? withRaw(num(t.contactAltitude, 0, "m"),
		t.contactAltitudeRaw) : cfg.missingValueLabel),
	  QList<QColor>() << (hasTrack ? cfg.trackColor : QColor())
	  << (hasTrack ? cfg.trackColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor())
	  << (hasContact ? cfg.contactColor : QColor()),
	  hasTrack || hasContact);

	// RWR radar-warning discretes — raw recorded codes (status/phase as hex,
	// code decimal). Present on both jets; only the engaged one latches.
	bool haveRwr = !std::isnan(t.rwrStatus) || !std::isnan(t.rwrPhase)
	  || !std::isnan(t.rwrCode);
	// RWR section colour: gold when the receiver is idle, red when the raw latch
	// has the detect bit set (cfg.rwrAlert) — the panel itself signals the alert.
	QColor rwrCol = cfg.rwrAlert(t.rwrStatus) ? cfg.rwrAlertColor : cfg.rwrColor;
	updateSection(_rwrSection, _rwrTable, QStringList()
	  << (std::isnan(t.rwrStatus) ? cfg.missingValueLabel
		: QString("0x%1").arg((int)(t.rwrStatus + 0.5), 0, 16))
	  << (std::isnan(t.rwrPhase) ? cfg.missingValueLabel
		: QString("0x%1").arg((int)(t.rwrPhase + 0.5), 0, 16))
	  << (std::isnan(t.rwrCode) ? cfg.missingValueLabel
		: QString::number((int)(t.rwrCode + 0.5))),
	  QList<QColor>() << (std::isnan(t.rwrStatus) ? QColor() : rwrCol)
	  << (std::isnan(t.rwrPhase) ? QColor() : rwrCol)
	  << (std::isnan(t.rwrCode) ? QColor() : rwrCol),
	  haveRwr);

	// MAWS incoming-missile track: recorded ABSOLUTE bearing + range, shown raw.
	bool haveMaws = !std::isnan(t.threatBearing) || !std::isnan(t.threatRange);
	updateSection(_mawsSection, _mawsTable, QStringList()
	  << (std::isnan(t.threatBearing) ? cfg.missingValueLabel
		: withRaw(num(t.threatBearing, 1, "deg"), t.threatBearingRaw))
	  << (std::isnan(t.threatRange) ? cfg.missingValueLabel
		: withRaw(rangeKm(t.threatRange), t.threatRangeRaw)),
	  QList<QColor>() << (std::isnan(t.threatBearing) ? QColor() : cfg.threatColor)
	  << (std::isnan(t.threatRange) ? QColor() : cfg.threatColor),
	  haveMaws);

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
	  << _discretesSection << _radarSection << _targetsSection << _rwrSection
	  << _mawsSection << _beaconSection;
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
	syncRenderAll();
}
