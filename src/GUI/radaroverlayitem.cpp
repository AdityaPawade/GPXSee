#include <QPainter>
#include <cmath>
#include "map/map.h"
#include "vizconfig.h"
#include "radaroverlayitem.h"

#define DEG2RAD(x) ((x) * M_PI / 180.0)
#define RAD2DEG(x) ((x) * 180.0 / M_PI)
#define EARTH_R   6371000.0   // WGS84 mean radius (physical constant)
#define Z_VALUE   3.0         // Qt scene draw order (above tracks/markers)
// All telemetry semantics (FOV range, scan default, mode colours,
// target colour/size) come from VizConfig (viz.cfg) — nothing hard-coded.

RadarOverlayItem::RadarOverlayItem(QGraphicsItem *parent)
  : QGraphicsItem(parent), _map(0), _active(false), _hasTarget(false), _mode(0)
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
		_hasTarget = false;
		_bound = QRectF();
		setVisible(false);
		update();
	}
}

void RadarOverlayItem::setData(const Coordinates &pos, const Telemetry &t)
{
	if (!_map || !pos.isValid() || std::isnan(t.yaw)) {
		clear();
		return;
	}

	prepareGeometryChange();

	QPointF apex(_map->ll2xy(pos));
	setPos(apex);

	const VizConfig &cfg = VizConfig::instance();
	_mode = std::isnan(t.radarMode) ? 0 : (int)(t.radarMode + 0.5);
	_cone.clear();
	_hasTarget = false;

	// Radar FOV wedge (only when the radar is on and a scan width is known).
	bool radarOn = cfg.radarOn(t.radarMode);
	double scan = std::isnan(t.radarScan) ? cfg.scanDefaultDeg : t.radarScan;
	if (radarOn && scan > 0) {
		double half = scan / 2.0;
		_cone << QPointF(0, 0);
		const int steps = 12;
		for (int i = 0; i <= steps; i++) {
			double a = t.yaw - half + (scan * i / steps);
			QPointF p(_map->ll2xy(destination(pos, a, cfg.fovRangeMeters)));
			_cone << (p - apex);
		}
		_cone << QPointF(0, 0);
	}
	// boresight (nose direction) tick
	_boresight = _map->ll2xy(destination(pos, t.yaw,
	  cfg.fovRangeMeters * 0.55)) - apex;

	// wedge colour by radar mode (from viz.cfg)
	_color = cfg.radarModeColor(_mode);

	// Contact -> project to a geographic point and mark it.
	if (!std::isnan(t.contactBearing) && !std::isnan(t.contactRange)
	  && t.contactRange > 0) {
		Coordinates c(destination(pos, t.yaw + t.contactBearing, t.contactRange));
		_target = _map->ll2xy(c) - apex;
		_hasTarget = true;
	}

	// bounding rect = union of cone, boresight, target (+margin)
	QRectF b = _cone.boundingRect();
	b |= QRectF(_boresight, QSizeF(1, 1));
	if (_hasTarget)
		b |= QRectF(_target - QPointF(12, 12), QSizeF(24, 24));
	_bound = b.adjusted(-4, -4, 4, 4);

	_active = true;
	setVisible(true);
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

	// FOV wedge
	if (_cone.size() > 2) {
		QColor fill(_color); fill.setAlpha(45);
		painter->setPen(QPen(_color, 1.5));
		painter->setBrush(fill);
		painter->drawPolygon(_cone);
	}
	// boresight
	painter->setPen(QPen(_color, 1.0, Qt::DashLine));
	painter->drawLine(QPointF(0, 0), _boresight);

	// aircraft apex dot
	painter->setPen(Qt::NoPen);
	painter->setBrush(QColor(0, 0, 0));
	painter->drawEllipse(QPointF(0, 0), 3, 3);

	// contact / target (colour + marker size from viz.cfg)
	if (_hasTarget) {
		const VizConfig &cfg = VizConfig::instance();
		QColor tc(cfg.targetColor);
		painter->setPen(QPen(tc, 1.0, Qt::DotLine));
		painter->setBrush(Qt::NoBrush);
		painter->drawLine(QPointF(0, 0), _target);

		painter->setPen(QPen(tc, 2.0));
		double r = cfg.targetMarkerRadiusPx;
		painter->drawEllipse(_target, r, r);
		painter->drawLine(_target + QPointF(-r - 3, 0), _target + QPointF(r + 3, 0));
		painter->drawLine(_target + QPointF(0, -r - 3), _target + QPointF(0, r + 3));
	}
}
