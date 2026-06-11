#ifndef RADAROVERLAYITEM_H
#define RADAROVERLAYITEM_H

#include <QGraphicsItem>
#include <QPolygonF>
#include "common/coordinates.h"
#include "data/telemetry.h"

class Map;

/* Map overlay drawn at the cursor track position. */
class RadarOverlayItem : public QGraphicsItem
{
public:
	enum Component {
		Fov = 0x1,
		LookRay = 0x2,
		Contacts = 0x4,
		All = Fov | LookRay | Contacts
	};

	RadarOverlayItem(QGraphicsItem *parent = 0);

	QRectF boundingRect() const {return _bound;}
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	void setMap(Map *map) {_map = map;}
	void setComponents(int components) {_components = components;}
	// Optional per-track tint; invalid colour = fall back to the configured
	// radar/look-ray colours.
	void setTrackColor(const QColor &color) {_trackColor = color;}
	void setData(const Coordinates &pos, const Telemetry &t);
	void clear();

private:
	Coordinates destination(const Coordinates &o, double bearingDeg,
	  double distM) const;

	Map *_map;
	bool _active;
	bool _hasLookRay;
	bool _hasTrack;
	bool _hasContact;
	int _components;
	int _mode;
	QPolygonF _cone;     // local coords relative to apex (this item's pos)
	QPointF _lookRay;    // local
	QPointF _track;      // local: recorded radar-track contact, plotted at heading+bearing
	QPointF _contact;    // local: recorded datalink contact, plotted at heading+bearing
	QColor _color;
	QColor _trackColor;  // optional per-track tint (invalid = use cfg colours)
	QRectF _bound;
};

#endif // RADAROVERLAYITEM_H
