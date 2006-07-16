/*
 * Copyright (C) 2004, Mart Kelder (mart.kde@hccnet.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "boxcontaineritem.h"
#include "boxcontaineritemadaptor.h"

#include "mailsubject.h"

#include <kaboutapplication.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kbugreport.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpassivepopup.h>
#include <kmenu.h>
#include <kprocess.h>
#include <kshortcut.h>
#include <kstdaction.h>
#include <ktoolinvocation.h>
#include <kvbox.h>

#include <QtDBus>

#include <QBitmap>
#include <QColor>
#include <QFont>
#include <QDateTime>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <q3ptrlist.h>
#include <QString>
#include <QToolTip>
#include <q3vbox.h>
#include <QMovie>

BoxContainerItem::BoxContainerItem( QObject * parent )
	: AccountManager( parent ),
	_command( new QString )
{
	short i;
	
	for( i = 0; i < 2; ++i )
	{
		_icons[ i ] = 0;
		_anims[ i ] = 0;
		_fgColour[ i ] = 0;
		_bgColour[ i ] = 0;
		_fonts[ i ] = 0;
	}
	
	for( i = 0; i < 3; ++i )
	{
		_recheckSettings[ i ] = false;
		_resetSettings[ i ] = false;
		_viewSettings[ i ] = false;
		_runSettings[ i ] = false;
		_popupSettings[ i ] = false;
	}

	new BoxContainerItemAdaptor( this );
#warning Put some useful DBus object path here
	QDBus::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors );
}

BoxContainerItem::~BoxContainerItem()
{
	delete _command;
}
	
void BoxContainerItem::readConfig( KConfig* config, const int index )
{
	//Read information about how the thing have to look like
	config->setGroup( QString( "korn-%1" ).arg( index ) );
	if( config->readEntry( "hasnormalicon", false ) )
		_icons[ 0 ] = new QString( config->readEntry( "normalicon", "" ) );
	else
		_icons[ 0 ] = 0;
	if( config->readEntry( "hasnewicon", false ) )
		_icons[ 1 ] = new QString( config->readEntry( "newicon", "" ) );
	else
		_icons[ 1 ] = 0;
	
	if( config->readEntry( "hasnormalanim", false ) )
		_anims[ 0 ] = new QString( config->readEntry( "normalanim", "" ) );
	else
		_anims[ 0 ] = 0;
	if( config->readEntry( "hasnewanim", false ) )
		_anims[ 1 ] = new QString( config->readEntry( "newanim", "" ) );
	else
		_anims[ 1 ] = 0;
	
	if( config->readEntry( "hasnormalfgcolour", false ) )
		_fgColour[ 0 ] = new QColor( config->readEntry( "normalfgcolour", QColor() ) );
	else
		_fgColour[ 0 ] = 0;
	if( config->readEntry( "hasnewfgcolour", false ) )
		_fgColour[ 1 ] = new QColor( config->readEntry( "newfgcolour", QColor() ) );
	else
		_fgColour[ 1 ] = 0;
	
	if( config->readEntry( "hasnormalbgcolour", false ) )
		_bgColour[ 0 ] = new QColor( config->readEntry( "normalbgcolour", QColor() ) );
	else
		_bgColour[ 0 ] = 0;
	if( config->readEntry( "hasnewbgcolour", false ) )
		_bgColour[ 1 ] = new QColor( config->readEntry( "newbgcolour", QColor() ) );
	else
		_bgColour[ 1 ] = 0;
	if( config->readEntry( "hasnormalfont", false ) )
		_fonts[ 0 ] = new QFont( config->readEntry( "normalfont", QFont() ) );
	else
		_fonts[ 0 ] = 0;
	if( config->readEntry( "hasnewfont", false ) )
		_fonts[ 1 ] = new QFont( config->readEntry( "newfont", QFont() ) );
	else
		_fonts[ 1 ] = 0;
	
	//Read information about the mappings.
	_recheckSettings[ 0 ] = config->readEntry( "leftrecheck", true );
	_recheckSettings[ 1 ] = config->readEntry( "middlerecheck", false );
	_recheckSettings[ 2 ] = config->readEntry( "rightrecheck", false );
	
	_resetSettings[ 0 ] = config->readEntry( "leftreset", false );
	_resetSettings[ 1 ] = config->readEntry( "middlereset", false );
	_resetSettings[ 2 ] = config->readEntry( "rightreset", false );
	
	_viewSettings[ 0 ] = config->readEntry( "leftview", false );
	_viewSettings[ 1 ] = config->readEntry( "middleview", false );
	_viewSettings[ 2 ] = config->readEntry( "rightview", false );
	
	_runSettings[ 0 ] = config->readEntry( "leftrun", false );
	_runSettings[ 1 ] = config->readEntry( "middlerun", false );
	_runSettings[ 2 ] = config->readEntry( "rightrun", false );
	
	_popupSettings[ 0 ] = config->readEntry( "leftpopup", false );
	_popupSettings[ 1 ] = config->readEntry( "middlepopup", false );
	_popupSettings[ 2 ] = config->readEntry( "rightpopup", true );
	
	//Read the command
	*_command = config->readEntry( "command", "" );
	
	//Sets the object ID for the DBUS-object
#warning Port me to DBus (using DBus object path instead?)
//	this->setObjId( config->readEntry( "name", "" ).toUtf8() );
	
	//Read the settings of the reimplemented class.
	//It is important to read this after the box-settings, because the
	//setCount-function is called in AccountManager::readConfig
	AccountManager::readConfig( config, index );
}

void BoxContainerItem::runCommand( const QString& cmd )
{
	KProcess *process = new KProcess;
	process->setUseShell( true );
	if( hasNewMessages() )
		process->setEnvironment( "HASNEWMESSAGES", "yes" );
	process->setEnvironment( "NUMBEROFMESSAGES", QString::number( totalMessages() ) );
	*process << cmd;
	connect( process, SIGNAL( processExited (KProcess *) ), this, SLOT( processExited( KProcess * ) ) );
	process->start();
}

void BoxContainerItem::mouseButtonPressed( Qt::MouseButton state )
{
	int button;
	if( state & Qt::LeftButton )
		button = 0;
	else if( state & Qt::RightButton )
		button = 2;
	else if( state & Qt::MidButton )
		button = 1;
	else
		return; //Invalid mouse button

	if( _recheckSettings[ button ] )
		doRecheck();
	if( _resetSettings[ button ] )
		doReset();
	if( _viewSettings[ button ] )
		doView();
	if( _runSettings[ button ] )
		runCommand();
	if( _popupSettings[ button ] )
		doPopup();
}

void BoxContainerItem::fillKMenu( KMenu* popupMenu, KActionCollection* actions ) const
{
	/*popupMenu->insertItem( i18n( "&Configure" ), this, SLOT( slotConfigure() ) );
	popupMenu->insertItem( i18n( "&Recheck" ), this, SLOT( slotRecheck() ) );
	popupMenu->insertItem( i18n( "R&eset Counter" ), this, SLOT( slotReset() ) );
	popupMenu->insertItem( i18n( "&View Emails" ), this, SLOT( slotView() ) );
	popupMenu->insertItem( i18n( "R&un Command" ), this, SLOT( slotRunCommand() ) );*/
	
        KAction *action = new KAction( i18n("&Configure"), actions, "configure" );
        connect(action, SIGNAL(triggered(bool)), SLOT( slotConfigure()  ));
	popupMenu->addAction(action);
	action = new KAction( i18n("&Recheck"), actions, "recheck"   );
        connect(action, SIGNAL(triggered(bool)), SLOT( slotRecheck()    ));
        popupMenu->addAction(action);
	action = new KAction( i18n("R&eset Counter"), actions, "reset"     );
        connect(action, SIGNAL(triggered(bool)), SLOT( slotReset()      ));
        popupMenu->addAction(action);
	action = new KAction( i18n("&View Emails"),  actions, "view"      );
        connect(action, SIGNAL(triggered(bool)), SLOT( slotView()       ));
        popupMenu->addAction(action);
	action = new KAction( i18n("R&un Command"),  actions, "run"       );
        connect(action, SIGNAL(triggered(bool)), SLOT( slotRunCommand() ));
        popupMenu->addAction(action);
	popupMenu->addSeparator();
	popupMenu->addAction( KStdAction::help(      this, SLOT( help()      ), actions ) );
	popupMenu->addAction( KStdAction::reportBug( this, SLOT( reportBug() ), actions ) );
	popupMenu->addAction( KStdAction::aboutApp(  this, SLOT( about()     ), actions ) );
}

void BoxContainerItem::showPassivePopup( QWidget* parent, QList< KornMailSubject >* list, int total,
					 const QString &accountName, bool date )
{
	KPassivePopup *popup = new KPassivePopup( parent );
	popup->setObjectName( "Passive popup" );
		
#warning Port objId() call to DBus!
	KVBox *mainvlayout = popup->standardView( QString( "KOrn - %1/%2 (total: %3)" ).arg( /*objId().data()*/"" ).arg( accountName )
			.arg( total ), "", QPixmap(), 0 );
	QWidget *mainglayout_wid = new QWidget( mainvlayout );
	QGridLayout *mainglayout = new QGridLayout( mainglayout_wid );
	
	QLabel *title = new QLabel( "From", mainglayout_wid );
	mainglayout->addWidget( title, 0, 0 );
	QFont font = title->font();
	font.setBold( true );
	title->setFont( font );
		
	title = new QLabel( "Subject", mainglayout_wid );
	mainglayout->addWidget( title, 0, 1 );
	font = title->font();
	font.setBold( true );
	title->setFont( font );
		
	//Display only column 3 if 'date' is true.
	if( date )
	{
		title = new QLabel( "Date", mainglayout_wid );
		mainglayout->addWidget( title, 0, 2 );
		font = title->font();
		font.setBold( true );
		title->setFont( font );
	}
	
	for( int xx = 0; xx < list->size(); ++xx )
	{
		//Make a label, add it to the layout and place it into the right position.
		mainglayout->addWidget( new QLabel( list->at( xx ).getSender(), mainglayout_wid ), xx + 1, 0 );
		mainglayout->addWidget( new QLabel( list->at( xx ).getSubject(), mainglayout_wid ), xx + 1, 1 );
		if( date )
		{
			QDateTime tijd;
			tijd.setTime_t( list->at( xx ).getDate() );
			mainglayout->addWidget( new QLabel( tijd.toString(), mainglayout_wid ), xx + 1, 2 );
		}
	}
	
	popup->setAutoDelete( true ); //Now, now care for deleting these pointers.
	
	popup->setView( mainvlayout );
	
	popup->show(); //Display it
}

void BoxContainerItem::drawLabel( QLabel *label, const int count, const bool newMessages )
{
	//This would fail if bool have fome other values.
	short index = newMessages ? 1 : 0;
	
	bool hasAnim = _anims[ index ] && !_anims[ index ]->isEmpty();
	bool hasIcon = _icons[ index ] && !_icons[ index ]->isEmpty();
	bool hasBg = _bgColour[ index ] && _bgColour[ index ]->isValid();
	bool hasFg = _fgColour[ index ] && _fgColour[ index ]->isValid();
	
	QPixmap pixmap;
	QPalette palette = label->palette();
	
	if( label->movie() ) //Delete movie pointer
		delete label->movie();
	//label->setMovie( 0 ); //TODO: crash in KDE4!
	
	label->setText( "" );
	//label->setToolTip( this->getTooltip() );
	
	if( hasAnim )
	{ //An animation can't have a foreground-colour and can't have a icon.
		setAnimIcon( label, *_anims[ index ] );

		hasFg = false;
		hasIcon = false;
	}
	
	if( hasIcon )
		pixmap = KGlobal::iconLoader()->loadIcon( *_icons[ index ], K3Icon::Desktop, K3Icon::SizeSmallMedium );
	
	if( hasIcon && hasFg )
	{
		if( hasBg )
		{
			label->setPixmap( calcComplexPixmap( pixmap, *_fgColour[ index ], _fonts[ index ], count ) );
			//label->setBackgroundMode( Qt::FixedColor );
			//label->setPaletteBackgroundColor( *_bgColour[ index ] );
			label->setBackgroundRole( QPalette::Window );
			palette.setColor( label->backgroundRole(), *_bgColour[ index ] );
		} else
		{
			label->setPixmap( calcComplexPixmap( pixmap, *_fgColour[ index ], _fonts[ index ], count ) );
		}
		return;
	}
	
	if( hasBg )
	{
		//label->setBackgroundMode( Qt::FixedColor );
		//label->setPaletteBackgroundColor( *_bgColour[ index ] );
		label->setBackgroundRole( QPalette::Window );
		palette.setColor( label->backgroundRole(), *_bgColour[ index ] );
	} else
	{
		//label->setBackgroundMode( Qt::X11ParentRelative );
		label->setBackgroundRole( QPalette::NoRole );
		palette.setColor( QPalette::Window, Qt::transparent );
	}
	
	if( hasIcon )
	{
		label->setPixmap( pixmap );
	}
	
	if( hasFg )
	{
		if( _fonts[ index ] )
			label->setFont( *_fonts[ index ] );
		palette.setColor( label->foregroundRole(), *_fgColour[ index ] );
		//label->setPaletteForegroundColor( *_fgColour[ index ] );
		label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
		label->setText( QString::number( count ) );
	}

	label->setPalette( palette );
	label->setAutoFillBackground( true );
	
	if( hasFg || hasBg || hasIcon || hasAnim )
		label->show();
	else
		label->hide();
}

//This function makes a pixmap which is based on icon, but has a number painted on it.
QPixmap BoxContainerItem::calcComplexPixmap( const QPixmap &icon, const QColor& fgColour, const QFont* font, const int count )
{
	QPixmap result( icon );
	QPainter p;

	p.setCompositionMode( QPainter::CompositionMode_DestinationOver );
	p.begin( &result );
	p.setPen( fgColour );
	if( font )
		p.setFont( *font );
	p.drawText( icon.rect(), Qt::AlignCenter, QString::number( count ) );
	
	return result;
}

void BoxContainerItem::setAnimIcon( QLabel* label, const QString& anim )
{
	label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	label->setMovie( new QMovie( anim ) );
	label->show();
}

void BoxContainerItem::recheck()
{
	doRecheck();
}

void BoxContainerItem::reset()
{
	doReset();
}

void BoxContainerItem::view()
{
	doView();
}

void BoxContainerItem::runCommand()//Possible_unsafe?
{	
	if( _command->isEmpty() )
		return; //Don't execute an empty command
	runCommand( *_command );
}

void BoxContainerItem::help()
{
	KToolInvocation::invokeHelp();
}

void BoxContainerItem::reportBug()
{
	KBugReport bug( 0, true );
	bug.exec(); //modal: it doesn't recheck anymore
}

void BoxContainerItem::about()
{
	KAboutApplication about( KGlobal::instance()->aboutData(), 0, true );
	about.exec();  //modal: it doesn't recheck anymore
} 

void BoxContainerItem::popup()
{
	doPopup();
}

void BoxContainerItem::showConfig()
{
	emit showConfiguration();
}

void BoxContainerItem::startTimer()
{
	doStartTimer();
}

void BoxContainerItem::stopTimer()
{
	doStopTimer();
}

void BoxContainerItem::processExited( KProcess* proc )
{
	delete proc;
}

#include "boxcontaineritem.moc"
