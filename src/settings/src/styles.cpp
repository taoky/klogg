/*
 * Copyright (C) 2021 Anton Filimonov and other contributors
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

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPointer>
#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#endif
#include <qcolor.h>

#include "configuration.h"
#include "log.h"
#include "styles.h"

QStringList StyleManager::availableStyles()
{
    QStringList styles;
#ifdef Q_OS_WIN
    styles << VistaKey;
    styles << WindowsKey;
    styles << FusionKey;
#else
    styles << QStyleFactory::keys();
#endif

    auto removedStyles = std::remove_if( styles.begin(), styles.end(), []( const QString& style ) {
        return style.startsWith( Gtk2Key, Qt::CaseInsensitive )
               || style.startsWith( Bb10Key, Qt::CaseInsensitive );
    } );

    styles.erase( removedStyles, styles.end() );

    styles << SystemStyleKey << DarkStyleKey;

#ifndef Q_OS_MACOS
    styles << DarkWindowsStyleKey;
#endif

    std::sort( styles.begin(), styles.end(), []( const auto& lhs, const auto& rhs ) {
        return lhs.compare( rhs, Qt::CaseInsensitive ) < 0;
    } );

    return styles;
}

QString StyleManager::defaultPlatformStyle()
{
#if defined( Q_OS_WIN )
    return VistaKey;
#elif defined( Q_OS_MACOS )
    return MacintoshKey;
#else
    return SystemStyleKey;
#endif
}

static void renderStyle( const QString& style )
{
    LOG_INFO << "Setting style to " << style;

    if ( style == StyleManager::DarkStyleKey || style == StyleManager::DarkWindowsStyleKey ) {
        const auto palette = Configuration::get().darkPalette();

        QPalette darkPalette;
        darkPalette.setColor( QPalette::Window, QColor( palette.at( "Window" ) ) );
        darkPalette.setColor( QPalette::WindowText, QColor( palette.at( "WindowText" ) ) );
        darkPalette.setColor( QPalette::Base, QColor( palette.at( "Base" ) ) );
        darkPalette.setColor( QPalette::AlternateBase, QColor( palette.at( "AlternateBase" ) ) );
        darkPalette.setColor( QPalette::ToolTipBase, QColor( palette.at( "ToolTipBase" ) ) );
        darkPalette.setColor( QPalette::ToolTipText, QColor( palette.at( "ToolTipText" ) ) );
        darkPalette.setColor( QPalette::Text, QColor( palette.at( "Text" ) ) );
        darkPalette.setColor( QPalette::Button, QColor( palette.at( "Button" ) ) );
        darkPalette.setColor( QPalette::ButtonText, QColor( palette.at( "ButtonText" ) ) );
        darkPalette.setColor( QPalette::Link, QColor( palette.at( "Link" ) ) );
        darkPalette.setColor( QPalette::Highlight, QColor( palette.at( "Highlight" ) ) );
        darkPalette.setColor( QPalette::HighlightedText,
                              QColor( palette.at( "HighlightedText" ) ) );

        darkPalette.setColor( QPalette::Active, QPalette::Button,
                              QColor( palette.at( "ActiveButton" ) ) );
        darkPalette.setColor( QPalette::Disabled, QPalette::ButtonText,
                              QColor( palette.at( "DisabledButtonText" ) ) );
        darkPalette.setColor( QPalette::Disabled, QPalette::WindowText,
                              QColor( palette.at( "DisabledWindowText" ) ) );
        darkPalette.setColor( QPalette::Disabled, QPalette::Text,
                              QColor( palette.at( "DisabledText" ) ) );
        darkPalette.setColor( QPalette::Disabled, QPalette::Light,
                              QColor( palette.at( "DisabledLight" ) ) );

        if ( style == StyleManager::DarkWindowsStyleKey ) {
            qApp->setStyle( QStyleFactory::create( StyleManager::WindowsKey ) );
        }
        else {
            qApp->setStyle( QStyleFactory::create( StyleManager::FusionKey ) );
        }

        qApp->setPalette( darkPalette );
    }
    else {
        // Clear explicit dark colors so Qt can resolve the current platform palette.
        qApp->setPalette( QPalette() );
        qApp->setStyle( style );
        qApp->setStyleSheet( "" );
    }
}


// Keep the desktop preference separate from the user's explicit style choice.
class SystemStyleWatcher : public QObject {
    Q_OBJECT
  public:
    explicit SystemStyleWatcher( QObject* parent ) : QObject( parent )
    {
        connect( qApp->styleHints(), &QStyleHints::colorSchemeChanged, this,
                 [ this ] { refresh(); }, Qt::QueuedConnection );
#ifdef Q_OS_LINUX
        auto bus = QDBusConnection::sessionBus();
        bus.connect( "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
                     "org.freedesktop.portal.Settings", "SettingChanged", this,
                     SLOT( settingChanged( QString, QString, QDBusVariant ) ) );
        auto* service = new QDBusServiceWatcher( "org.freedesktop.portal.Desktop", bus,
                                                QDBusServiceWatcher::WatchForOwnerChange, this );
        connect( service, &QDBusServiceWatcher::serviceOwnerChanged, this,
                 [ this ] {
                     ++revision_;
                     portalPreference_ = -1;
                     refresh();
                     readPreference();
                 } );
        readPreference();
#endif
    }

    void apply( const QString& style )
    {
        style_ = style;
        render();
    }

  private:
    void refresh()
    {
        if ( style_ == StyleManager::SystemStyleKey )
            render();
    }

    void render()
    {
        auto style = style_;
        if ( style == StyleManager::SystemStyleKey ) {
            bool dark = qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#ifdef Q_OS_LINUX
            // 0 means no preference; 1 dark; 2 light. Do not cast to Qt's enum:
            // Qt assigns different numeric values to Light and Dark.
            if ( portalPreference_ >= 0 )
                dark = portalPreference_ == 1;
#endif
            style = dark ? StyleManager::DarkStyleKey : StyleManager::FusionKey;
        }
        renderStyle( style );
    }

#ifdef Q_OS_LINUX
    void readPreference()
    {
        auto message = QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Settings", "Read" );
        message << "org.freedesktop.appearance" << "color-scheme";
        auto* call = new QDBusPendingCallWatcher(
            QDBusConnection::sessionBus().asyncCall( message ), this );
        const auto revision = revision_;
        connect( call, &QDBusPendingCallWatcher::finished, this, [ this, revision, call ] {
            const QDBusPendingReply<QDBusVariant> reply = *call;
            if ( !reply.isError() && revision == revision_ )
                updatePreference( reply.value() );
            call->deleteLater();
        } );
    }

    void updatePreference( const QDBusVariant& value )
    {
        bool ok = false;
        const auto preference = value.variant().toUInt( &ok );
        if ( !ok || preference > 2 )
            return;
        portalPreference_ = static_cast<int>( preference );
        refresh();
    }

  private Q_SLOTS:
    void settingChanged( const QString& group, const QString& key, const QDBusVariant& value )
    {
        if ( group == "org.freedesktop.appearance" && key == "color-scheme" ) {
            ++revision_;
            updatePreference( value );
        }
    }

  private:
    int portalPreference_ = -1;
    unsigned revision_ = 0;
#endif
    QString style_;
};

void StyleManager::applyStyle( const QString& style )
{
    static QPointer<SystemStyleWatcher> watcher;
    if ( !watcher )
        watcher = new SystemStyleWatcher( qApp );
    watcher->apply( style );
}

#include "styles.moc"
