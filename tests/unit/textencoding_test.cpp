#include <catch2/catch.hpp>

#include "encodingdetector.h"
#include "textencoding.h"

TEST_CASE( "Thai codec aliases survive canonicalization and legacy settings migration",
           "[encoding]" )
{
    const auto name = GENERATE( QByteArray( "TIS-620" ), QByteArray( "windows-874" ),
                                textencoding::canonicalNameForMib( 2259 ) );
    const auto canonical = textencoding::canonicalName( QString::fromLatin1( name ) );
    REQUIRE_FALSE( canonical.isEmpty() );

    // Known TIS-620 bytes for ภาษาไทย; do not generate the fixture with the encoder.
    const auto bytes = QByteArray::fromHex( "c0d2c9d2e4b7c2" );
    const auto expected = QString::fromUtf8( "ภาษาไทย" );
    auto decoder = textencoding::makeDecoder( canonical );
    REQUIRE( QString( decoder.decode( bytes ) ) == expected );
    REQUIRE_FALSE( decoder.hasError() );
    REQUIRE( textencoding::encode( canonical, expected ) == bytes );
}

TEST_CASE( "Escape-based Japanese text must not be detected as UTF-8", "[encoding]" )
{
    // ISO-2022-JP for 日本語のログです followed by a newline, all bytes below 0x80.
    const auto bytes = QByteArray::fromHex( "1b2442467c4b5c386c244e256d2530244724391b28420a" );
    const klogg::vector<char> block( bytes.begin(), bytes.end() );
    const auto name = EncodingDetector::getInstance().detectEncoding( block );
    auto decoder = textencoding::makeDecoder( name );
    REQUIRE( QString( decoder.decode( bytes ) ) == QString::fromUtf8( "日本語のログです\n" ) );
    REQUIRE_FALSE( decoder.hasError() );
}
