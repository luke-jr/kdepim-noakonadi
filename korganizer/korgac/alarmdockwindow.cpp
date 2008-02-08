/*
  This file is part of KOrganizer.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "alarmdockwindow.h"
#include "koalarmclient.h"

#include <kactioncollection.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kiconeffect.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kstandardaction.h>
#include <ktoolinvocation.h>
#include <kglobal.h>

#include <QFile>
#include <QMouseEvent>

#include <stdlib.h>

AlarmDockWindow::AlarmDockWindow()
  : KSystemTrayIcon( 0 )
{
  // Read the autostart status from the config file
  KConfigGroup config( KGlobal::config(), "General" );
  bool autostart = config.readEntry( "Autostart", true );
  bool alarmsEnabled = config.readEntry( "Enabled", true );

  mName = i18n( "KOrganizer Reminder Daemon" );
  setToolTip( mName );

  // Set up icons
  KIconLoader::global()->addAppDir( "korgac" );
  mIconEnabled  = loadIcon( "korgac" );
  if ( mIconEnabled.isNull() ) {
    KMessageBox::sorry( parentWidget(),
                        i18nc( "@info", "Cannot load system tray icon." ) );
  } else {
    KIconLoader loader;
    QImage iconDisabled =
      mIconEnabled.pixmap( loader.currentSize( KIconLoader::Panel ) ).toImage();
    KIconEffect::toGray( iconDisabled, 1.0 );
    mIconDisabled = QIcon( QPixmap::fromImage( iconDisabled ) );
  }

  setIcon( alarmsEnabled ? mIconEnabled : mIconDisabled );

  // Set up the context menu
  mSuspendAll = contextMenu()->addAction( i18n( "Suspend All" ),
                                          this, SLOT(slotSuspendAll()) );
  mDismissAll = contextMenu()->addAction( i18n( "Dismiss All" ),
                                          this, SLOT(slotDismissAll()) );
  mSuspendAll->setEnabled( false );
  mDismissAll->setEnabled( false );

  contextMenu()->addSeparator();
  mAlarmsEnabled = contextMenu()->addAction( i18n( "Reminders Enabled" ), this,
                                             SLOT(toggleAlarmsEnabled()) );
  mAutostart = contextMenu()->addAction( i18n( "Start Reminder Daemon at Login" ),
                                         this, SLOT(toggleAutostart()) );
  mAutostart->setEnabled( autostart );
  mAlarmsEnabled->setEnabled( alarmsEnabled );

  // Disable standard quit behaviour. We have to intercept the quit even, if the
  // main window is hidden.
  KActionCollection *ac = actionCollection();
  const char *quitName = KStandardAction::name( KStandardAction::Quit );
  QAction *quit = ac->action( quitName );
  if ( !quit ) {
    kDebug() << "No Quit standard action.";
  } else {
    quit->disconnect( SIGNAL(triggered(bool)), this, SLOT(maybeQuit()) );
    connect( quit, SIGNAL(activated()), SLOT(slotQuit()) );
  }

  connect( this, SIGNAL(activated( QSystemTrayIcon::ActivationReason )),
           SLOT(slotActivated( QSystemTrayIcon::ActivationReason )) );

  setToolTip( mName );
}

AlarmDockWindow::~AlarmDockWindow()
{
}

void AlarmDockWindow::slotUpdate( int reminders )
{
  mSuspendAll->setEnabled( reminders > 0 );
  mDismissAll->setEnabled( reminders > 0 );
  if ( reminders > 0 ) {
    setToolTip( i18np( "There is 1 active reminder.",
                       "There are %1 active reminders.", reminders ) );
  } else {
    setToolTip( mName );
  }
}

void AlarmDockWindow::toggleAlarmsEnabled()
{
  kDebug();

  KConfigGroup config( KGlobal::config(), "General" );

  bool enabled = !mAlarmsEnabled->isChecked();
  mAlarmsEnabled->setChecked( enabled );
  setIcon( enabled ? mIconEnabled : mIconDisabled );

  config.writeEntry( "Enabled", enabled );
  config.sync();
}

void AlarmDockWindow::toggleAutostart()
{
  bool autostart = !mAutostart->isChecked();
  enableAutostart( autostart );
}

void AlarmDockWindow::slotSuspendAll()
{
  emit suspendAllSignal();
}

void AlarmDockWindow::slotDismissAll()
{
  emit dismissAllSignal();
}

void AlarmDockWindow::enableAutostart( bool enable )
{
  KConfigGroup config( KGlobal::config(), "General" );
  config.writeEntry( "Autostart", enable );
  config.sync();

  mAutostart->setChecked( enable );
}

void AlarmDockWindow::slotActivated( QSystemTrayIcon::ActivationReason reason )
{
  if ( reason == QSystemTrayIcon::Trigger ) {
    KToolInvocation::startServiceByDesktopName( "korganizer", QString() );
  }
}

void AlarmDockWindow::slotQuit()
{
  int result = KMessageBox::questionYesNoCancel(
    parentWidget(),
    i18n( "Do you want to start the KOrganizer reminder daemon at login "
          "(note that you will not get reminders whilst the daemon is not running)?" ),
    i18n( "Close KOrganizer Reminder Daemon" ),
    KGuiItem( i18nc( "@action:button start the reminder daemon", "Start" ) ), KGuiItem( i18nc( "@action:button do not start the reminder daemon", "Do Not Start" ) ), KStandardGuiItem::cancel(),
    QString::fromLatin1( "AskForStartAtLogin" ) );

  bool autostart = true;
  if ( result == KMessageBox::No ) {
    autostart = false;
  }
  enableAutostart( autostart );

  if ( result != KMessageBox::Cancel ) {
    emit quitSignal();
  }
}

#include "alarmdockwindow.moc"
