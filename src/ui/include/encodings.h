//
// Created by omni on 16.12.2019.
//

#ifndef KLOGG_ENCODINGS_H
#define KLOGG_ENCODINGS_H

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // Q_OS_WIN

#include <QActionGroup>
#include <QApplication>
#include <QMenu>
#include <QString>

#include <configuration.h>
#include <mainwindowtext.h>
#include <textencoding.h>

class EncodingMenu {
  public:
    static std::map<QString, std::vector<QByteArray>> supportedEncodings()
    {
        return std::map<QString, std::vector<QByteArray>>{
            { "Unicode", { "UTF-8", "UTF-16BE", "UTF-16LE", "UTF-32BE", "UTF-32LE" } },
            { "Arabic", { "ISO-8859-6", "windows-1256" } },
            { "Baltic", { "ISO-8859-4", "ISO-8859-13", "windows-1257" } },
            { "Celtic", { "ISO-8859-14" } },
            { "Cyrillic", { "ISO-8859-5", "KOI8-R", "KOI8-U", "macintosh", "IBM866", "windows-1251" } },
            { "Central European", { "windows-1250" } },
            { "Chinese", { "Big5", "GBK" } },
            { "Eastern European", { "ISO-8859-2" } },
            { "Greek", { "ISO-8859-7", "windows-1253" } },
            { "Hebrew", { "ISO-8859-8", "windows-1255" } },
            { "Japanese", { "Shift_JIS", "EUC-JP", "ISO-2022-JP" } },
            { "Korean", { "windows-949" } },
            { "Thai", { "TIS-620" } },
            { "Turkish", { "ISO-8859-3", "ISO-8859-9", "windows-1254" } },
            { "Western European", { "US-ASCII", "ISO-8859-1", "ISO-8859-15", "IBM850", "windows-1252" } },
            { "Vietnamese", { "windows-1258" } }
        };
    }

    static QMenu* generate( QActionGroup* actionGroup )
    {
        const auto supportedEncodings = EncodingMenu::supportedEncodings();
        const auto defaultEncodingName = Configuration::get().defaultEncodingName();

        using namespace klogg::mainwindow;
        QMenu* encodingsMenu = new QMenu(
            QApplication::translate( "klogg::mainwindow::menu", menu::encodingTitle ) );

        auto autoEncoding = encodingsMenu->addAction(
            QApplication::translate( "klogg::mainwindow::action", action::autoEncodingText ) );
        autoEncoding->setStatusTip(
            QApplication::translate( "klogg::mainwindow::action", action::autoEncodingStatusTip ) );
        autoEncoding->setCheckable( true );
        autoEncoding->setActionGroup( actionGroup );
        autoEncoding->setChecked( defaultEncodingName.isEmpty() );

        encodingsMenu->addSeparator();

        const auto systemEncodingName = textencoding::systemEncodingName();

        auto systemEncodingLabel
            = QApplication::tr( "System (%1)" ).arg( QString::fromLatin1( systemEncodingName ) );

        auto systemEncoding = encodingsMenu->addAction( systemEncodingLabel );
        systemEncoding->setCheckable( true );
        systemEncoding->setActionGroup( actionGroup );
        systemEncoding->setData( systemEncodingName );

        encodingsMenu->addSeparator();

        for ( const auto& group : supportedEncodings ) {
            auto menu = encodingsMenu->addMenu( group.first );
            for ( const auto& encodingName : group.second ) {
                auto action = menu->addAction( QString::fromLatin1( encodingName ) );
                action->setActionGroup( actionGroup );
                action->setData( encodingName );
                action->setCheckable( true );
                action->setChecked( defaultEncodingName == encodingName );
            }
        }

        return encodingsMenu;
    }
};

#endif // KLOGG_ENCODINGS_H
