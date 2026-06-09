#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include "collapsiblesection.h"

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent)
  : QWidget(parent), _content(0)
{
	_button = new QToolButton(this);
	_button->setText(title);
	_button->setCheckable(true);
	_button->setChecked(true);
	_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	_button->setArrowType(Qt::DownArrow);
	_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	_button->setStyleSheet("QToolButton { border: none; font-weight: bold; }");
	connect(_button, &QToolButton::toggled, this,
	  &CollapsibleSection::toggled);

	QWidget *header = new QWidget(this);
	_headerLayout = new QHBoxLayout(header);
	_headerLayout->setContentsMargins(0, 0, 0, 0);
	_headerLayout->addWidget(_button);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 4, 0, 4);
	layout->setSpacing(2);
	layout->addWidget(header);
	setLayout(layout);
}

void CollapsibleSection::setContent(QWidget *content)
{
	if (_content)
		layout()->removeWidget(_content);
	_content = content;
	if (_content) {
		layout()->addWidget(_content);
		_content->setVisible(_button->isChecked());
	}
}

void CollapsibleSection::addHeaderWidget(QWidget *widget)
{
	_headerLayout->addWidget(widget);
}

void CollapsibleSection::setExpanded(bool expanded)
{
	_button->setChecked(expanded);
	toggled(expanded);
}

bool CollapsibleSection::isExpanded() const
{
	return _button->isChecked();
}

void CollapsibleSection::toggled(bool checked)
{
	_button->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
	if (_content)
		_content->setVisible(checked);
}
