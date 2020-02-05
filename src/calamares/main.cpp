/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2014, Teo Mrnjavac <teo@kde.org>
 *   Copyright 2017-2018, Adriaan de Groot <groot@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */


#include "CalamaresApplication.h"

#include "CalamaresConfig.h"
#include "utils/Dirs.h"
#include "utils/Logger.h"

#include "3rdparty/kdsingleapplicationguard/kdsingleapplicationguard.h"

#include <KF5/KCoreAddons/KAboutData>
#ifdef WITH_KF5Crash
#include <KF5/KCrash/KCrash>
#endif

#include <QCommandLineParser>
#include <QDebug>
#include <QDir>

static void
handle_args( CalamaresApplication& a )
{
    QCommandLineOption debugOption( QStringList { "d", "debug" },
                                    "Also look in current directory for configuration. Implies -D8." );
    QCommandLineOption debugLevelOption(
        QStringLiteral( "D" ), "Verbose output for debugging purposes (0-8).", "level" );
    QCommandLineOption configOption(
        QStringList { "c", "config" }, "Configuration directory to use, for testing purposes.", "config" );
    QCommandLineOption xdgOption( QStringList { "X", "xdg-config" }, "Use XDG_{CONFIG,DATA}_DIRS as well." );

    QCommandLineParser parser;
    parser.setApplicationDescription( "Distribution-independent installer framework" );
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption( debugOption );
    parser.addOption( debugLevelOption );
    parser.addOption( configOption );
    parser.addOption( xdgOption );

    parser.process( a );

    a.setDebug( parser.isSet( debugOption ) );
    if ( parser.isSet( debugOption ) )
    {
        Logger::setupLogLevel( Logger::LOGVERBOSE );
    }
    else if ( parser.isSet( debugLevelOption ) )
    {
        bool ok = true;
        int l = parser.value( debugLevelOption ).toInt( &ok );
        unsigned int dlevel = 0;
        if ( !ok || ( l < 0 ) )
        {
            dlevel = Logger::LOGVERBOSE;
        }
        else
        {
            dlevel = static_cast< unsigned int >( l );  // l >= 0
        }
        Logger::setupLogLevel( dlevel );
    }
    if ( parser.isSet( configOption ) )
    {
        CalamaresUtils::setAppDataDir( QDir( parser.value( configOption ) ) );
    }
    if ( parser.isSet( xdgOption ) )
    {
        CalamaresUtils::setXdgDirs();
    }
}

int
main( int argc, char* argv[] )
{
    CalamaresApplication a( argc, argv );

    KAboutData aboutData( "calamares",
                          "Calamares",
                          a.applicationVersion(),
                          "The universal system installer",
                          KAboutLicense::GPL_V3,
                          QString(),
                          QString(),
                          "https://calamares.io",
                          "https://github.com/calamares/calamares/issues" );
    KAboutData::setApplicationData( aboutData );
    a.setApplicationDisplayName( QString() );  // To avoid putting an extra "Calamares/" into the log-file

#ifdef WITH_KF5Crash
    KCrash::initialize();
    // KCrash::setCrashHandler();
    KCrash::setDrKonqiEnabled( true );
    KCrash::setFlags( KCrash::SaferDialog | KCrash::AlwaysDirectly );
    // TODO: umount anything in /tmp/calamares-... as an emergency save function
#endif

    handle_args( a );
    KDSingleApplicationGuard guard( KDSingleApplicationGuard::AutoKillOtherInstances );

    if ( guard.isPrimaryInstance() )
    {
        a.init();
        return a.exec();
    }
    else
    {
        // Here we have not yet set-up the logger system, so qDebug() is ok
        auto instancelist = guard.instances();
        qDebug() << "Calamares is already running, shutting down.";
        if ( instancelist.count() > 0 )
        {
            qDebug() << "Other running Calamares instances:";
        }
        for ( const auto& i : instancelist )
        {
            qDebug() << "  " << i.isValid() << i.pid() << i.arguments();
        }
        return 69;  // EX_UNAVAILABLE on FreeBSD
    }
}
