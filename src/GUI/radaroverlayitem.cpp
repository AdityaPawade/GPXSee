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
    _hasTrack(false), _hasContact(false), _components(All), _mode(0)
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
	_mode = std::isnan(t.radarMode) ? 0 : (int)(t.radarMode + 0.5);
	_cone.clear();
	_hasLookRay = false;
	_hasTrack = false;
	_hasContact = false;
	_lookRay = QPointF();
	_track = QPointF();
	_contact = QPointF();

	// Continuous radar state: a raw search-mode field can read OFF while locked,
	// so use radarState + the effective antenna azimuth. The azimuth is present
	// only while the antenna is pointed at a tracked target (LOCK); in SEARCH/OFF
	// it is absent. The FOV wedge and look-ray are anchored on that azimuth, so
	// they are drawn ONLY when it is present (never at a stale/north default).
	bool haveState = !std::isnan(t.radarState);
	bool radarOn = haveState ? ((int)(t.radarState + 0.5) >= 1)
	  : cfg.radarOn(t.radarMode);
	double azimuth = !std::isnan(t.radarAz) ? t.radarAz : t.lookAzimuth;
	bool haveAzimuth = !std::isnan(azimuth);
	double scan = std::isnan(t.radarScan) ? cfg.scanDefaultDeg : t.radarScan;
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

	_color = _trackColor.isValid() ? _trackColor : cfg.radarModeColor(_mode);

	if ((_components & LookRay) && cfg.showLookRay && !std::isnan(azimuth)) {
		_lookRay = _map->ll2xy(destination(pos, azimuth,
		  cfg.fovRangeMeters * 0.75)) - apex;
		_hasLookRay = true;
	}

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

	QRectF b = _cone.boundingRect();
	if (_hasLookRay)
		b |= QRectF(_lookRay, QSizeF(1, 1));
	if (_hasTrack)
		b |= QRectF(_track - QPointF(16, 16), QSizeF(32, 32));
	if (_hasContact)
		b |= QRectF(_contact - QPointF(16, 16), QSizeF(32, 32));
	_bound = b.adjusted(-4, -4, 4, 4);

	_active = _cone.size() > 2 || _hasLookRay || _hasTrack || _hasContact;
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
	QColor lookColor(_trackColor.isValid() ? _trackColor : cfg.lookRayColor);
	if (_hasLookRay) {
		painter->setPen(QPen(lookColor, cfg.lookRayWidthPx, Qt::DashLine));
		painter->drawLine(QPointF(0, 0), _lookRay);
	}

	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(0, 0, 0));
	painter->drawEllipse(QPointF(0, 0), 3, 3);

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
}
