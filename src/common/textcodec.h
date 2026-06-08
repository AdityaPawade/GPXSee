#ifndef TEXTCODEC_H
#define TEXTCODEC_H

#include <QString>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) \
  || defined(Q_OS_ANDROID) || defined(Q_OS_MAC)
#include <QTextCodec>
#else // QT 5 || ANDROID || MAC
#include <QStringConverter>
#define TEXT_READER_JOIN3(a, b, c) a##b##c
#define TEXT_READER_TYPE QT_PREPEND_NAMESPACE(TEXT_READER_JOIN3(QString, De, coder))
#endif // QT 5 || ANDROID || MAC

class TextCodec
{
public:
	TextCodec();
	TextCodec(int codepage);

	QString toString(const QByteArray &ba);

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) \
  || defined(Q_OS_ANDROID) || defined(Q_OS_MAC)
	QTextCodec *_codec;
#else // QT 5 || ANDROID || MAC
	TEXT_READER_TYPE _textReader;
#endif // QT 5 || ANDROID || MAC
};

#endif // TEXTCODEC_H
