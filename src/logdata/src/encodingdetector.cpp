/*
 * Copyright (C) 2016 -- 2019 Anton Filimonov and other contributors
 *
 * This file is part of klogg.
 *
 * klogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * klogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with klogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "encodingdetector.h"

#include <QStringConverter>

#include "containers.h"
#include "log.h"
#include "textencoding.h"

#include <simdutf.h>
#include <uchardet.h>

namespace {

class UchardetHolder {
  public:
    UchardetHolder()
        : ud_{ uchardet_new() }
    {
    }
    ~UchardetHolder()
    {
        uchardet_delete( ud_ );
    }

    UchardetHolder( const UchardetHolder& ) = delete;
    UchardetHolder& operator=( const UchardetHolder& ) = delete;

    UchardetHolder( UchardetHolder&& other ) = delete;
    UchardetHolder& operator=( UchardetHolder&& other ) = delete;

    int handle_data( const char* data, size_t len )
    {
        return uchardet_handle_data( ud_, data, len );
    }

    void data_end()
    {
        uchardet_data_end( ud_ );
    }

    const char* get_charset()
    {
        return uchardet_get_charset( ud_ );
    }

  private:
    uchardet_t ud_;
};

} // namespace

EncodingParameters::EncodingParameters( const QByteArray& encodingName )
{
    static constexpr QChar LineFeed( QChar::LineFeed );

    isUtf8Compatible = encodingName == "UTF-8" || encodingName == "US-ASCII";
    isUtf16LE = encodingName == "UTF-16LE";

    const QByteArray encodedLineFeed = textencoding::encode( encodingName, QStringView( &LineFeed, 1 ) );

    lineFeedWidth = static_cast<int>( encodedLineFeed.size() );
    lineFeedIndex
        = encodedLineFeed[ 0 ] == '\n' ? 0 : ( static_cast<int>( encodedLineFeed.size() ) - 1 );
}

QByteArray EncodingDetector::detectEncoding( const klogg::vector<char>& block ) const
{
    UniqueLock lock( mutex_ );

    UchardetHolder ud;

    auto rc = ud.handle_data( block.data(), block.size() );
    if ( rc == 0 ) {
        ud.data_end();
    }

    QByteArray uchardetEncoding;
    if ( rc == 0 ) {
        auto uchardetGuess = ud.get_charset();
        LOG_DEBUG << "Uchardet encoding guess " << uchardetGuess;
        uchardetEncoding = textencoding::canonicalName( QLatin1StringView( uchardetGuess ) );
        if ( !uchardetEncoding.isEmpty() ) {
            LOG_DEBUG << "Uchardet codec selected " << uchardetEncoding.constData();
        }
        else {
            LOG_DEBUG << "Uchardet codec not found for guess " << uchardetGuess;
        }
    }

    QByteArray blockArray = QByteArray::fromRawData( block.data(), klogg::isize( block ) );
    QByteArray encodingGuess;
    if ( const auto detectedUnicodeEncoding = QStringConverter::encodingForData( blockArray ) ) {
        encodingGuess = QByteArray( QStringConverter::nameForEncoding( *detectedUnicodeEncoding ) );
    }
    else if ( !uchardetEncoding.isEmpty() ) {
        // Escape-based encodings such as ISO-2022-JP are also valid UTF-8 byte
        // sequences. Preserve the detector's result unless a BOM overrides it.
        encodingGuess = uchardetEncoding;
    }
    else if ( simdutf::validate_utf8( block.data(), block.size() ) ) {
        encodingGuess = QByteArrayLiteral( "UTF-8" );
    }

    if ( encodingGuess.isEmpty() ) {
        encodingGuess = textencoding::systemEncodingName();
    }

    LOG_DEBUG << "Final encoding guess " << encodingGuess.constData();

    return encodingGuess;
}

TextCodecHolder::TextCodecHolder( const QByteArray& encodingName )
    : encodingName_{ encodingName }
    , encodingParams_{ encodingName }
{
    assert( !encodingName.isEmpty() );
}

QByteArray TextCodecHolder::encodingName() const
{
    SharedLock guard( mutex_ );
    return encodingName_;
}

EncodingParameters TextCodecHolder::encodingParameters() const
{
    SharedLock guard( mutex_ );
    return encodingParams_;
}

void TextCodecHolder::setEncodingName( const QByteArray& encodingName )
{
    UniqueLock guard( mutex_ );
    encodingName_ = encodingName;
    encodingParams_ = EncodingParameters{ encodingName_ };
}

TextDecoder TextCodecHolder::makeDecoder() const
{
    SharedLock guard( mutex_ );
    return { textencoding::makeDecoder( encodingName_ ), encodingParams_ };
}
