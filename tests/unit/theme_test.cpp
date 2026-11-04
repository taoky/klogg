#include <catch2/catch.hpp>

#include <QApplication>
#include <QImage>
#include <QSignalSpy>
#include <QStyle>
#include <QTemporaryFile>

#include "highlighterset.h"
#include "logdata.h"
#include "logmainview.h"
#include "quickfindpattern.h"
#include "styles.h"

namespace {
struct RestoreAppearance {
    QString style = qApp->style()->objectName();
    QPalette palette = qApp->palette();

    ~RestoreAppearance()
    {
        qApp->setStyle( style );
        qApp->setPalette( palette );
    }
};
}

TEST_CASE( "Leaving Dark restores the platform palette without explicit color overrides", "[theme]" )
{
    RestoreAppearance restore;
    qApp->setPalette( QPalette() );
    StyleManager::applyStyle( "Fusion" );
    const auto platformPalette = qApp->palette();

    for ( const auto& darkStyle : { "Dark", "Windows Dark" } ) {
        StyleManager::applyStyle( darkStyle );
        REQUIRE( qApp->palette().resolveMask() != 0 );
        StyleManager::applyStyle( "Fusion" );
        CHECK( qApp->palette() == platformPalette );
        CHECK( qApp->palette().resolveMask() == platformPalette.resolveMask() );
    }
}

TEST_CASE( "Palette changes repaint cached log lines without scrolling or manual refresh", "[theme]" )
{
    HighlighterSetCollection::getSynced();
    QTemporaryFile file;
    REQUIRE( file.open() );
    REQUIRE( file.write( "A cached log line\n" ) > 0 );
    REQUIRE( file.flush() );

    LogData data;
    QSignalSpy loaded( &data, &LogData::loadingFinished );
    data.attachFile( file.fileName() );
    REQUIRE( ( !loaded.isEmpty() || loaded.wait() ) );
    REQUIRE( data.getNbLine().get() > 0 );

    QuickFindPattern quickFind;
    OverviewWidget overviewWidget;
    LogMainView view( &data, &quickFind, nullptr, &overviewWidget );
    view.resize( 640, 200 );
    view.updateData();
    view.show();

    auto palette = view.palette();
    palette.setColor( QPalette::Base, Qt::white );
    palette.setColor( QPalette::Text, Qt::black );
    view.setPalette( palette );
    const auto light = view.viewport()->grab().toImage();

    palette.setColor( QPalette::Base, QColor( "#282828" ) );
    palette.setColor( QPalette::Text, Qt::white );
    view.setPalette( palette );
    const auto dark = view.viewport()->grab().toImage();
    view.forceRefresh();
    CHECK( dark == view.viewport()->grab().toImage() );
    CHECK( dark != light );

    palette.setColor( QPalette::Base, Qt::white );
    palette.setColor( QPalette::Text, Qt::black );
    view.setPalette( palette );
    CHECK( view.viewport()->grab().toImage() == light );
}
