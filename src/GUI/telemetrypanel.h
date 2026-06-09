#ifndef TELEMETRYPANEL_H
#define TELEMETRYPANEL_H

#include <functional>
#include <QColor>
#include <QHash>
#include <QScrollArea>
#include "common/coordinates.h"
#include "data/telemetry.h"

class QCheckBox;
class QComboBox;
class QTableWidget;
class CollapsibleSection;

class TelemetryPanel : public QScrollArea
{
	Q_OBJECT

public:
	enum Capability {
		CapAttitude, CapEngine, CapDiscretes, CapRadar, CapTargets, CapBeacon
	};

	TelemetryPanel(QWidget *parent = 0);

	void setOverlayStateProvider(const std::function<bool(int, int)> &provider);
	// Provider answers "does the selected track expose capability C anywhere?"
	// Section visibility is decided from this (whole-track), not the cursor sample.
	void setCapabilityProvider(const std::function<bool(int, int)> &provider);

public slots:
	void updateTelemetry(const QString &name, const Coordinates &pos,
	  const Telemetry &t);
	void clear();
	void setFlights(const QStringList &names);
	void reloadOverlayState();

signals:
	void flightChanged(int index);
	void overlayToggled(int trackId, int overlayType, bool on);

private:
	QTableWidget *createTable(const QStringList &keys) const;
	void set(QTableWidget *table, int row, const QString &value,
	  const QColor &color = QColor()) const;
	QString num(qreal v, int prec, const QString &unit = QString()) const;
	QString rangeKm(qreal meters) const;
	QString bearingPair(qreal rel, qreal abs) const;
	QString boolStr(int v, const QString &on, const QString &off) const;
	QCheckBox *addOverlay(CollapsibleSection *section, const QString &key,
	  int type);
	void updateSection(CollapsibleSection *section, QTableWidget *table,
	  const QStringList &values, const QList<QColor> &colors, bool visible);
	void applyCapabilities();

	QComboBox *_flightCombo;
	CollapsibleSection *_flightSection;
	CollapsibleSection *_attitudeSection;
	CollapsibleSection *_engineSection;
	CollapsibleSection *_discretesSection;
	CollapsibleSection *_radarSection;
	CollapsibleSection *_targetsSection;
	CollapsibleSection *_beaconSection;
	QTableWidget *_flightTable;
	QTableWidget *_attitudeTable;
	QTableWidget *_engineTable;
	QTableWidget *_discretesTable;
	QTableWidget *_radarTable;
	QTableWidget *_targetsTable;
	QTableWidget *_beaconTable;
	QHash<int, QCheckBox*> _overlayChecks;
	std::function<bool(int, int)> _overlayStateProvider;
	std::function<bool(int, int)> _capabilityProvider;
};

#endif // TELEMETRYPANEL_H
