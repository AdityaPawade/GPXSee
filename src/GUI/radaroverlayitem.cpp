#include <QPainter>
#include <cmath>
#include "map/map.h"
#include "vizconfig.h"
#include "radaroverlayitem.h"

#define DEG2RAD(x) ((x) * M_PI / 180.0)
#define RAD2DEG(x) ((x) * 180.0 / M_PI)
#define EARTH_R   6371000.0   // WGS84 mean radius (physical constant)
#define Z_VALUE   3.0         // Qt scene draw order (above tracks/markers)

RadarOverlayItem::RadarOverlayItem(QGraphicsItem *parent)
  : QGraphicsItem(parent), _map(0), _active(false), _hasLookRay(false),
    _hasTrack(false), _hasContact(false), _hasThreat(false),
    _hasRwr(false), _rwrAlert(false),
    _components(All), _mode(0)
{
	setZValue(Z_VALUE);
	setVisible(false);
}

Coordinates RadarOverlayItem::destination(const Coordinates &o,
  double bearingDeg, double distM) const
{
	double br = DEG2RAD(bearingDeg), d = distM / EARTH_R;
	double lat1 = DEG2RAD(o.lat()), lon1 = DEG2RAD(o.lon());
	double lat2 = std::asin(std::sin(lat1) * std::cos(d)
	  + std::cos(lat1) * std::sin(d) * std::cos(br));
	double lon2 = lon1 + std::atan2(std::sin(br) * std::sin(d) * std::cos(lat1),
	  std::cos(d) - std::sin(lat1) * std::sin(lat2));
	return Coordinates(RAD2DEG(lon2), RAD2DEG(lat2));
}

void RadarOverlayItem::clear()
{
	if (_active || isVisible()) {
		prepareGeometryChange();
		_active = false;
		_hasLookRay = false;
		_hasTrack = false;
		_hasContact = false;
		_hasThreat = false;
		_hasRwr = false;
		_bound = QRectF();
		setVisible(false);
		update();
	}
}

void RadarOverlayItem::setData(const Coordinates &pos, const Telemetry &t)
{
	if (!_map || !pos.isValid()) {
		clear();
		return;
	}

	prepareGeometryChange();

	QPointF apex(_map->ll2xy(pos));
	setPos(apex);

	const VizConfig &cfg = VizConfig::instance();
	_mode = std::isnan(t.radarState) ? 0 : (int)(t.radarState + 0.5);
	_cone.clear();
	_hasLookRay = false;
	_hasTrack = false;
	_hasContact = false;
	_hasThreat = false;
	_hasRwr = false;
	_rwrAlert = false;
	_lookRay = QPointF();
	_track = QPointF();
	_contact = QPointF();
	_threat = QPointF();

	// RAW TRUTH: the overlay renders only RECORDED values, and never fuses
	// channels.
	//   * radarAz is the recorded ABSOLUTE antenna azimuth -> used directly.
	//   * if only the search sector is active (no absolute azimuth recorded),
	//     the sector is anchored to the recorded heading (yaw) = the antenna
	//     boresight. The nose-relative search azimuth is NEVER combined with
	//     heading to synthesise an absolute pointing direction.
	//   * the sector WIDTH is a user-defined display choice from viz.cfg
	//     [radar_scan_modes], keyed by the recorded scan-mode byte — not a
	//     value measured or derived from the telemetry.
	int state = std::isnan(t.radarState) ? -1 : (int)(t.radarState + 0.5);
	bool radarOn = state >= 1;
	double azimuth = NAN;
	if (!std::isnan(t.radarAz))
		azimuth = t.radarAz;            // recorded absolute antenna azimuth
	else if (radarOn && !std::isnan(t.yaw))
		azimuth = t.yaw;                // boresight = recorded heading
	bool haveAzimuth = !std::isnan(azimuth);

	double scan = cfg.scanModeWidthDeg(t.radarScanMode);
	if (std::isnan(scan))
		scan = cfg.scanDefaultDeg;
	if ((_components & Fov) && cfg.showScanWedge && radarOn && scan > 0
	  && haveAzimuth) {
		double half = scan / 2.0;
		_cone << QPointF(0, 0);
		const int steps = 12;
		for (int i = 0; i <= steps; i++) {
			double a = azimuth - half + (scan * i / steps);
			QPointF p(_map->ll2xy(destination(pos, a, cfg.fovRangeMeters)));
			_cone << (p - apex);
		}
		_cone << QPointF(0, 0);
	}

	// The radar FOV is a sensor-attribute overlay: it keeps its own dedicated
	// colour (per radar state) and does NOT follow the per-track colour.
	_color = cfg.radarStateColor(t.radarState);
	if (!_color.isValid())
		_color = cfg.radarDefaultColor;

	// Look ray = the recorded ABSOLUTE antenna azimuth (radarAz) only, drawn as
	// a direction line (no range implied, single channel, no fusion). Absent
	// when no absolute azimuth was recorded near this point.
	if ((_components & LookRay) && cfg.showLookRay && !std::isnan(t.radarAz)) {
		_lookRay = _map->ll2xy(destination(pos, t.radarAz,
		  cfg.fovRangeMeters * 0.75)) - apex;
		_hasLookRay = true;
	}

	// Radar-track + datalink-contact markers. The bearing/range/altitude are
	// RECORDED values (shown raw in the Targets panel); to place the marker on a
	// north-up map the recorded nose-relative bearing is referenced to the
	// recorded heading (yaw). This is a geometric plot of recorded data, not an
	// invented value.
	if ((_components & Contacts) && cfg.showTrack && !std::isnan(t.yaw)
	  && !std::isnan(t.trackBearing)
	  && !std::isnan(t.trackRange) && t.trackRange > 0) {
		Coordinates c(destination(pos, t.yaw + t.trackBearing, t.trackRange));
		_track = _map->ll2xy(c) - apex;
		_hasTrack = true;
	}
	if ((_components & Contacts) && cfg.showContact && !std::isnan(t.yaw)
	  && !std::isnan(t.contactBearing)
	  && !std::isnan(t.contactRange) && t.contactRange > 0) {
		Coordinates c(destination(pos, t.yaw + t.contactBearing, t.contactRange));
		_contact = _map->ll2xy(c) - apex;
		_hasContact = true;
	}

	// MAWS incoming-missile ray = the recorded ABSOLUTE threat bearing (c2b48),
	// drawn from the jet. It is already an absolute compass bearing AS RECORDED,
	// used directly with NO heading fusion. When a range was recorded the diamond
	// sits at the REAL distance (a positioned missile); otherwise it is a
	// fixed-length direction ray. Has its own "MAWS" overlay toggle.
	if ((_components & MawsRay) && !std::isnan(t.threatBearing)) {
		double dist = (!std::isnan(t.threatRange) && t.threatRange > 0)
		  ? t.threatRange : cfg.fovRangeMeters;
		_threat = _map->ll2xy(destination(pos, t.threatBearing, dist)) - apex;
		_hasThreat = true;
	}

	// RWR warning ring at the ownship — a STATE annunciator (no bearing): present
	// whenever an RWR status byte was recorded; it goes to its "alert" style when
	// the raw latch has a detect bit set (cfg.rwrAlert, a config lookup).
	if ((_components & RwrRing) && !std::isnan(t.rwrStatus)) {
		_hasRwr = true;
		_rwrAlert = cfg.rwrAlert(t.rwrStatus);
	}

	// Always span from the apex (0,0): the look-ray / markers all start there, so
	// the bounding rect must include it. Without this, in LOCK (no FOV wedge) the
	// rect would cover only the far endpoint and Qt would cull the whole item
	// when zoomed in near the apex — the ray would vanish.
	QRectF b = _cone.boundingRect();
	b |= QRectF(-2, -2, 4, 4);
	if (_hasLookRay)
		b |= QRectF(_lookRay - QPointF(2, 2), QSizeF(4, 4));
	if (_hasTrack)
		b |= QRectF(_track - QPointF(16, 16), QSizeF(32, 32));
	if (_hasContact)
		b |= QRectF(_contact - QPointF(16, 16), QSizeF(32, 32));
	if (_hasThreat)
		b |= QRectF(_threat - QPointF(8, 8), QSizeF(16, 16));
	if (_hasRwr)
		b |= QRectF(-26, -26, 52, 52);
	_bound = b.adjusted(-4, -4, 4, 4);

	_active = _cone.size() > 2 || _hasLookRay || _hasTrack || _hasContact
	  || _hasThreat || _hasRwr;
	setVisible(_active);
	update();
}

void RadarOverlayItem::paint(QPainter *painter,
  const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (!_active)
		return;

	painter->setRenderHint(QPainter::Antialiasing, true);

	if (_cone.size() > 2) {
		QColor fill(_color); fill.setAlpha(45);
		painter->setPen(QPen(_color, 1.5));
		painter->setBrush(fill);
		painter->drawPolygon(_cone);
	}

	const VizConfig &cfg = VizConfig::instance();
	QColor lookColor(cfg.lookRayColor);   // attribute colour, not the track colour
	if (_hasLookRay) {
		painter->setPen(QPen(lookColor, cfg.lookRayWidthPx, Qt::DashLine));
		painter->drawLine(QPointF(0, 0), _lookRay);
	}

	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(0, 0, 0));
	painter->drawEllipse(QPointF(0, 0), 3, 3);

	// Radar-track marker: a square (with a dotted lead-in line from the jet).
	if (_hasTrack) {
		painter->setPen(QPen(cfg.trackColor, cfg.targetLineWidthPx, Qt::DotLine));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), _track);

		painter->setPen(QPen(cfg.trackColor, 2.0));
		double r = cfg.trackMarkerRadiusPx;
		painter->drawRect(QRectF(_track - QPointF(r, r), QSizeF(r * 2, r * 2)));
		painter->drawLine(_track + QPointF(-r - 3, 0), _track + QPointF(r + 3, 0));
		painter->drawLine(_track + QPointF(0, -r - 3), _track + QPointF(0, r + 3));
	}

	// Datalink-contact marker: a circle (with a dotted lead-in line from the jet).
	if (_hasContact) {
		painter->setPen(QPen(cfg.contactColor, cfg.targetLineWidthPx, Qt::DotLine));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), _contact);

		painter->setPen(QPen(cfg.contactColor, 2.0));
		double r = cfg.contactMarkerRadiusPx;
		painter->drawEllipse(_contact, r, r);
		painter->drawLine(_contact + QPointF(-r - 3, 0), _contact + QPointF(r + 3, 0));
		painter->drawLine(_contact + QPointF(0, -r - 3), _contact + QPointF(0, r + 3));
	}

	// EW threat ray: a long dashed line from the jet along the recorded ABSOLUTE
	// threat bearing, with an open diamond at the far end. Direction only (no
	// range recorded), distinct threat colour so it reads as "incoming bearing".
	if (_hasThreat) {
		painter->setPen(QPen(cfg.threatColor, cfg.lookRayWidthPx, Qt::DashDotLine));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), _threat);
		double d = 5.0;
		QPolygonF diamond;
		diamond << _threat + QPointF(0, -d) << _threat + QPointF(d, 0)
		        << _threat + QPointF(0, d) << _threat + QPointF(-d, 0);
		painter->setPen(QPen(cfg.threatColor, 2.0));
		painter->drawPolygon(diamond);
	}

	// RWR annunciator ring around the jet (a STATE indicator, no bearing). Idle =
	// a thin dotted gold ring (receiver active); on threat-detect = a bold red
	// double-ring with 8 radial ticks — a cockpit RWR-scope look.
	if (_hasRwr) {
		QColor rc = _rwrAlert ? cfg.rwrAlertColor : cfg.rwrColor;
		painter->setBrush(Qt::NoBrush);
		if (_rwrAlert) {
			painter->setPen(QPen(rc, 2.5));
			painter->drawEllipse(QPointF(0, 0), 15, 15);
			painter->setPen(QPen(rc, 1.5));
			painter->drawEllipse(QPointF(0, 0), 21, 21);
			for (int k = 0; k < 8; k++) {
				double a = DEG2RAD(k * 45.0);
				painter->drawLine(QPointF(15 * std::sin(a), -15 * std::cos(a)),
				                  QPointF(21 * std::sin(a), -21 * std::cos(a)));
			}
		} else {
			painter->setPen(QPen(rc, 1.2, Qt::DotLine));
			painter->drawEllipse(QPointF(0, 0), 18, 18);
		}
	}
}
