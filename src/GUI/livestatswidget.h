#ifndef LIVESTATSWIDGET_H
#define LIVESTATSWIDGET_H

#include <QWidget>
#include "common/coordinates.h"
#include "data/telemetry.h"

class QTableWidget;
class QComboBox;

/* A live readout of the per-point telemetry at the current graph-slider
   instant (heading/attitude/airspeed, radar mode & scan, nav cue, fuel, gear,
   discretes, contact bearing/range, weapon flag). Fed by
   MapView::markerTelemetry. */
class LiveStatsWidget : public QWidget
{
	Q_OBJECT

public:
	LiveStatsWidget(QWidget *parent = 0);

public slots:
	void updateTelemetry(const QString &name, const Coordinates &pos,
	  const Telemetry &t);
	void clear();
	void setFlights(const QStringList &names);

signals:
	void flightChanged(int index);

private:
	enum Row {
		RAircraft, RPos, RYaw, RPitch, RRoll, RAirspeed, RVspeed,
		RRollRate, RYawRate, RRadarMode, RRadarScan, RContactBrg,
		RContactRng, RNavBrg, RNavRng, RFuel, RGear, RWow, RSlats,
		RWeapon, RowCount
	};
	void set(int row, const QString &value, const QColor &color = QColor());
	QTableWidget *_table;
	QComboBox *_flightCombo;
};

#endif // LIVESTATSWIDGET_H
