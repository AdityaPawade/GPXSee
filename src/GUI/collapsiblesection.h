#ifndef COLLAPSIBLESECTION_H
#define COLLAPSIBLESECTION_H

#include <QWidget>

class QHBoxLayout;
class QToolButton;

class CollapsibleSection : public QWidget
{
	Q_OBJECT

public:
	CollapsibleSection(const QString &title, QWidget *parent = 0);

	void setContent(QWidget *content);
	void addHeaderWidget(QWidget *widget);
	void setExpanded(bool expanded);
	bool isExpanded() const;

private slots:
	void toggled(bool checked);

private:
	QToolButton *_button;
	QHBoxLayout *_headerLayout;
	QWidget *_content;
};

#endif // COLLAPSIBLESECTION_H
