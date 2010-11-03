/*
 * Logger.cpp - a global clas for easily logging messages to log files
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <italcconfig.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QSettings>

#include "Logger.h"


Logger::LogLevel Logger::logLevel = Logger::LogLevelDefault;
Logger *Logger::instance = NULL;


Logger::Logger( const QString &appName ) :
	m_appName( appName ),
	m_logFile( NULL )
{
	instance = this;

	QSettings s;
	if( s.contains( "logging/loglevel" ) )
	{
		int ll = s.value( "logging/loglevel" ).toInt();
		if( ll >= LogLevelMin && ll <= LogLevelMax )
		{
			logLevel = static_cast<LogLevel>( ll );
		}
	}

	initLogFile();

	qInstallMsgHandler( qtMsgHandler );

	if( QCoreApplication::instance() )
	{
		// log current application start up
		LogStream() << "Startup" << QCoreApplication::arguments();
	}
	else
	{
		ilog( Info, "Startup without QCoreApplication instance" );
	}
}




Logger::~Logger()
{
	LogStream() << "Shutdown";

	delete m_logFile;
}




void Logger::initLogFile()
{
	QString tmpPath = QDir::rootPath() +
#ifdef ITALC_BUILD_WIN32
							"temp"
#else
							"tmp"
#endif
				;

	foreach( const QString s, QProcess::systemEnvironment() )
	{
		if( s.toLower().left( 5 ) == "temp=" )
		{
			tmpPath = s.toLower().mid( 5 );
			break;
		}
		else if( s.toLower().left( 4 ) == "tmp=" )
		{
			tmpPath = s.toLower().mid( 4 );
			break;
		}
	}

	if( !QDir( tmpPath ).exists() )
	{
		if( QDir( QDir::rootPath() ).mkdir( tmpPath ) )
		{
			QFile::setPermissions( tmpPath,
						QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
						QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
						QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
						QFile::ReadOther | QFile::WriteOther | QFile::ExeOther );
		}
	}

	const QString logPath = tmpPath + QDir::separator();
	m_logFile = new QFile( logPath + QString( "%1.log" ).arg( m_appName ) );
	m_logFile->open( QFile::WriteOnly | QFile::Append | QFile::Unbuffered );
}




QString Logger::formatMessage( LogLevel ll, const QString &msg )
{
#ifdef ITALC_BUILD_WIN32
	static const char *linebreak = "\r\n";
#else
	static const char *linebreak = "\n";
#endif

	QString msgType;
	switch( ll )
	{
		case LogLevelDebug: msgType = "DEBUG"; break;
		case LogLevelInfo: msgType = "INFO"; break;
		case LogLevelWarning: msgType = "WARN"; break;
		case LogLevelError: msgType = "ERR"; break;
		case LogLevelCritical: msgType = "CRIT"; break;
		default: break;
	}

	return QString( "%1: [%2] %3%4" ).
				arg( QDateTime::currentDateTime().toString() ).
				arg( msgType ).
				arg( msg.trimmed() ).
				arg( linebreak );
}




void Logger::qtMsgHandler( QtMsgType msgType, const char *msg )
{
	if( instance == NULL )
	{
		return;
	}
	if( logLevel == LogLevelNothing )
	{
		return ;
	}
	QString out;
	switch( msgType )
	{
		case QtDebugMsg:
			if( logLevel >= LogLevelDebug )
			{
				out = formatMessage( LogLevelDebug, msg );
			}
			break;
		case QtWarningMsg:
			if( logLevel >= LogLevelWarning )
			{
				out = formatMessage( LogLevelWarning, msg );
			}
			break;
		case QtCriticalMsg:
			if( logLevel >= LogLevelError )
			{
				out = formatMessage( LogLevelError, msg );
			}
			break;
		case QtFatalMsg:
			if( logLevel >= LogLevelCritical )
			{
				out = formatMessage( LogLevelCritical, msg );
			}
			break;
		default:
			out = formatMessage( LogLevelDefault, msg );
			break;
	}

	instance->outputMessage( out );
}




void Logger::log( LogLevel ll, const QString &msg )
{
	if( logLevel >= ll )
	{
		instance->outputMessage( formatMessage( ll, msg ) );
	}
}




void Logger::log( LogLevel ll, const char *format, ... )
{
	va_list args;
	va_start( args, format );

	QString message;
	message.vsprintf( format, args );

	va_end(args);

	log( ll, message );
}




void Logger::outputMessage( const QString &msg )
{
	m_logFile->write( msg.toUtf8() );
	m_logFile->flush();

	fprintf( stderr, "%s", msg.toUtf8().constData() );
	fflush( stderr );
}

