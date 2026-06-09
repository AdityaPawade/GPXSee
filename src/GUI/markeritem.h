#ifndef MARKERITEM_H
#define MARKERITEM_H

#include <QGraphicsItem>
#include <QColor>

class MarkerItem : public QGraphicsItem
{
public:
	MarkerItem(QGraphicsItem *parent = 0);

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	void setColor(const QColor &color);
	// Heading (deg, 0 = north, clockwise) used to orient a platform chevron at
	// the marker. NAN -> fall back to the plain crosshair marker.
	void setHeading(qreal headingDeg);

private:
	QColor _color;
	qreal _heading;
};

#endif // MARKERITEM_H
