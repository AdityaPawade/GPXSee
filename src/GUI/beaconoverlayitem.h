#ifndef BEACONOVERLAYITEM_H
#define BEACONOVERLAYITEM_H

#include <QGraphicsItem>
#include "common/coordinates.h"
#include "data/telemetry.h"

class Map;

class BeaconOverlayItem : public QGraphicsItem
{
public:
	BeaconOverlayItem(QGraphicsItem *parent = 0);

	QRectF boundingRect() const {return _bound;}
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	void setMap(Map *map) {_map = map;}
	void setData(const Coordinates &pos, const Telemetry &t);
	void clear();

private:
	Coordinates destination(const Coordinates &o, double bearingDeg,
	  double distM) const;

	Map *_map;
	bool _active;
	QPointF _end;
	QRectF _bound;
};

#endif // BEACONOVERLAYITEM_H
