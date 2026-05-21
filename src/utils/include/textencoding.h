#ifndef KLOGG_TEXTENCODING_H
#define KLOGG_TEXTENCODING_H

#include <QByteArray>
#include <QString>
#include <QStringConverter>

namespace textencoding {

QByteArray canonicalName( QAnyStringView name );
QByteArray canonicalNameForMib( int mib );
QByteArray systemEncodingName();

QStringDecoder makeDecoder( const QByteArray& encodingName );
QStringEncoder makeEncoder( const QByteArray& encodingName );
QByteArray encode( const QByteArray& encodingName, QStringView text );

} // namespace textencoding

#endif
