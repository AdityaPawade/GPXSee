#ifndef RADAROVERLAYITEM_H
#define RADAROVERLAYITEM_H

#include <QGraphicsItem>
#include <QPolygonF>
#include "common/coordinates.h"
#include "data/telemetry.h"

class Map;

/* Map overlay drawn at the cursor aircraft position. Renders the radar
   field-of-view wedge (heading +- scan/2, coloured by mode) and, when a contact
   is present, a target marker projected at (heading+contactBearing, contactRange). */
class RadarOverlayItem : public QGraphicsItem
{
public:
	RadarOverlayItem(QGraphicsItem *parent = 0);

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
	bool _hasTarget;
	int _mode;
	QPolygonF _cone;     // local coords relative to apex (this item's pos)
	QPointF _boresight;  // local
	QPointF _target;     // local
	QColor _color;
	QRectF _bound;
};

#endif // RADAROVERLAYITEM_H
