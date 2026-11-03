#include "textencoding.h"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <langinfo.h>
#endif

#include <array>

namespace textencoding {
namespace {

struct LegacyEncoding {
    int mib;
    const char* name;
};

constexpr std::array LegacyEncodings = {
    LegacyEncoding{ 106, "UTF-8" },       LegacyEncoding{ 1013, "UTF-16BE" },
    LegacyEncoding{ 1014, "UTF-16LE" },   LegacyEncoding{ 1018, "UTF-32BE" },
    LegacyEncoding{ 1019, "UTF-32LE" },   LegacyEncoding{ 82, "ISO-8859-6" },
    LegacyEncoding{ 2256, "windows-1256" }, LegacyEncoding{ 7, "ISO-8859-4" },
    LegacyEncoding{ 109, "ISO-8859-13" }, LegacyEncoding{ 2257, "windows-1257" },
    LegacyEncoding{ 110, "ISO-8859-14" }, LegacyEncoding{ 8, "ISO-8859-5" },
    LegacyEncoding{ 2084, "KOI8-R" },     LegacyEncoding{ 2088, "KOI8-U" },
    LegacyEncoding{ 2027, "macintosh" },  LegacyEncoding{ 2086, "IBM866" },
    LegacyEncoding{ 2251, "windows-1251" }, LegacyEncoding{ 2250, "windows-1250" },
    LegacyEncoding{ 2026, "Big5" },       LegacyEncoding{ 2025, "GBK" },
    LegacyEncoding{ 5, "ISO-8859-2" },    LegacyEncoding{ 10, "ISO-8859-7" },
    LegacyEncoding{ 2253, "windows-1253" }, LegacyEncoding{ 85, "ISO-8859-8" },
    LegacyEncoding{ 2255, "windows-1255" }, LegacyEncoding{ 17, "Shift_JIS" },
    LegacyEncoding{ 18, "EUC-JP" },       LegacyEncoding{ 39, "ISO-2022-JP" },
    LegacyEncoding{ 38, "windows-949" },  LegacyEncoding{ 2259, "TIS-620" },
    LegacyEncoding{ 6, "ISO-8859-3" },    LegacyEncoding{ 12, "ISO-8859-9" },
    LegacyEncoding{ 2254, "windows-1254" }, LegacyEncoding{ 3, "US-ASCII" },
    LegacyEncoding{ 4, "ISO-8859-1" },    LegacyEncoding{ 111, "ISO-8859-15" },
    LegacyEncoding{ 2009, "IBM850" },     LegacyEncoding{ 2252, "windows-1252" },
    LegacyEncoding{ 2258, "windows-1258" },
};

QByteArray fallbackEncodingName()
{
    return QByteArrayLiteral( "UTF-8" );
}

QString encodingNameView( const QByteArray& encodingName )
{
    return QString::fromLatin1( encodingName );
}

} // namespace

QByteArray canonicalName( QAnyStringView name )
{
    if ( name.isEmpty() ) {
        return {};
    }

    if ( const auto encoding = QStringConverter::encodingForName( name ) ) {
        return QByteArray( QStringConverter::nameForEncoding( *encoding ) );
    }

    QStringDecoder decoder( name );
    if ( !decoder.isValid() ) {
        return {};
    }

    // ICU can accept an alias without supplying a canonical name (e.g. TIS-620).
    // An empty result would turn a valid encoding into Auto/UTF-8 at call sites.
    const QByteArray canonicalName( decoder.name() );
    return canonicalName.isEmpty() ? name.toString().toLatin1() : canonicalName;
}

QByteArray canonicalNameForMib( int mib )
{
    for ( const auto& encoding : LegacyEncodings ) {
        if ( encoding.mib == mib ) {
            return canonicalName( QLatin1StringView( encoding.name ) );
        }
    }

    return {};
}

QByteArray systemEncodingName()
{
#ifdef Q_OS_WIN
    return canonicalName( QString( "CP%1" ).arg( ::GetACP() ) );
#else
    const auto localeEncoding = QByteArray( nl_langinfo( CODESET ) );
    const auto canonicalLocaleEncoding = canonicalName( QString::fromLatin1( localeEncoding ) );
    return canonicalLocaleEncoding.isEmpty() ? fallbackEncodingName() : canonicalLocaleEncoding;
#endif
}

QStringDecoder makeDecoder( const QByteArray& encodingName )
{
    return QStringDecoder( encodingNameView( encodingName.isEmpty() ? fallbackEncodingName()
                                                                  : encodingName ) );
}

QStringEncoder makeEncoder( const QByteArray& encodingName )
{
    return QStringEncoder( encodingNameView( encodingName.isEmpty() ? fallbackEncodingName()
                                                                  : encodingName ) );
}

QByteArray encode( const QByteArray& encodingName, QStringView text )
{
    auto encoder = makeEncoder( encodingName );
    return encoder.encode( text );
}

} // namespace textencoding
