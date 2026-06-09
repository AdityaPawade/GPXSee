#include <QPainter>
#include <cmath>
#include "markeritem.h"


#define SIZE  8
#define WIDTH 2

MarkerItem::MarkerItem(QGraphicsItem *parent) : QGraphicsItem(parent)
{
	_color = Qt::red;
	_heading = NAN;
}

QRectF MarkerItem::boundingRect() const
{
	// A touch larger than the crosshair so the rotated chevron always fits.
	return QRectF(-SIZE, -SIZE, 2 * SIZE, 2 * SIZE);
}

void MarkerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
  QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	if (std::isnan(_heading)) {
		// No heading available: keep the plain crosshair marker (crisp).
		painter->setRenderHint(QPainter::Antialiasing, false);
		painter->setPen(QPen(_color, WIDTH));
		painter->drawLine(-SIZE/2, 0, SIZE/2, 0);
		painter->drawLine(0, -SIZE/2, 0, SIZE/2);
		return;
	}

	// Generic platform symbol: an aircraft-like chevron with the nose pointing
	// up before rotation; rotate clockwise by the heading (0 = north).
	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->rotate(_heading);

	QPolygonF chevron;
	chevron << QPointF(0, -SIZE)            // nose
	  << QPointF(SIZE * 0.7, SIZE * 0.8)    // right wing tip
	  << QPointF(0, SIZE * 0.35)            // tail notch
	  << QPointF(-SIZE * 0.7, SIZE * 0.8);  // left wing tip

	painter->setPen(QPen(_color, 1.0));
	painter->setBrush(_color);
	painter->drawPolygon(chevron);

	painter->restore();
}

void MarkerItem::setColor(const QColor &color)
{
	_color = color;
	update();
}

void MarkerItem::setHeading(qreal headingDeg)
{
	if ((std::isnan(headingDeg) && std::isnan(_heading))
	  || headingDeg == _heading)
		return;
	prepareGeometryChange();
	_heading = headingDeg;
	update();
}
