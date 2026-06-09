#include <QPainter>
#include <cmath>
#include "map/map.h"
#include "vizconfig.h"
#include "beaconoverlayitem.h"

#define DEG2RAD(x) ((x) * M_PI / 180.0)
#define RAD2DEG(x) ((x) * 180.0 / M_PI)
#define EARTH_R   6371000.0
#define Z_VALUE   3.0

BeaconOverlayItem::BeaconOverlayItem(QGraphicsItem *parent)
  : QGraphicsItem(parent), _map(0), _active(false)
{
	setZValue(Z_VALUE);
	setVisible(false);
}

Coordinates BeaconOverlayItem::destination(const Coordinates &o,
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

void BeaconOverlayItem::clear()
{
	if (_active || isVisible()) {
		prepareGeometryChange();
		_active = false;
		_end = QPointF();
		_bound = QRectF();
		setVisible(false);
		update();
	}
}

void BeaconOverlayItem::setData(const Coordinates &pos, const Telemetry &t)
{
	if (!_map || !pos.isValid() || std::isnan(t.beaconBearing)
	  || std::isnan(t.beaconRange) || t.beaconRange <= 0) {
		clear();
		return;
	}

	prepareGeometryChange();
	QPointF origin(_map->ll2xy(pos));
	setPos(origin);
	_end = _map->ll2xy(destination(pos, t.beaconBearing,
	  t.beaconRange * 1000.0)) - origin;
	_bound = QRectF(QPointF(0, 0), _end).normalized()
	  .adjusted(-10, -10, 10, 10);
	_active = true;
	setVisible(true);
	update();
}

void BeaconOverlayItem::paint(QPainter *painter,
  const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (!_active)
		return;

	const VizConfig &cfg = VizConfig::instance();
	QColor color(_trackColor.isValid() ? _trackColor : cfg.beaconColor);
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(QPen(color, 2.0, Qt::DashDotLine));
	painter->setBrush(Qt::NoBrush);
	painter->drawLine(QPointF(0, 0), _end);

	painter->setPen(QPen(color, 2.0));
	painter->setBrush(QBrush(color));
	painter->drawEllipse(_end, 5.0, 5.0);
	painter->drawLine(_end + QPointF(-9, 0), _end + QPointF(9, 0));
	painter->drawLine(_end + QPointF(0, -9), _end + QPointF(0, 9));
}
