#ifndef SENSORTARGETSWIDGET_H
#define SENSORTARGETSWIDGET_H

#include <QWidget>
#include "common/coordinates.h"
#include "data/telemetry.h"

class QTableWidget;

class SensorTargetsWidget : public QWidget
{
	Q_OBJECT

public:
	enum Mode {
		SensorMode,
		TargetsMode
	};

	SensorTargetsWidget(Mode mode, QWidget *parent = 0);

public slots:
	void updateTelemetry(const QString &name, const Coordinates &pos,
	  const Telemetry &t);
	void clear();

private:
	void set(int row, const QString &value, const QColor &color = QColor());
	QString range(qreal meters) const;

	Mode _mode;
	QTableWidget *_table;
};

#endif // SENSORTARGETSWIDGET_H
