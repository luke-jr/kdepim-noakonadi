/*
 *  prefsettings.h  -  program preference settings
 *  Program:  kalarm
 *
 *  (C) 2001, 2002 by David Jarvie  software@astrojar.org.uk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  As a special exception, permission is given to link this program
 *  with any edition of Qt, and distribute the resulting executable,
 *  without including the source code for Qt in the source distribution.
 */

#ifndef PREFSETTINGS_H
#define PREFSETTINGS_H

#include "kalarm.h"

#include <qobject.h>
#include <qcolor.h>
#include <qfont.h>
#include <qdatetime.h>
class QWidget;

#include "recurrenceedit.h"


// Settings configured in the Preferences dialog
class Settings : public QObject
{
      Q_OBJECT
   public:
      enum MailClient { SENDMAIL, KMAIL };

      Settings(QWidget* parent);

      QColor         defaultBgColour() const          { return mDefaultBgColour; }
      const QFont&   messageFont() const              { return mMessageFont; }
      bool           runInSystemTray() const          { return mRunInSystemTray; }
      bool           disableAlarmsIfStopped() const   { return mDisableAlarmsIfStopped; }
      bool           autostartTrayIcon() const        { return mAutostartTrayIcon; }
      bool           confirmAlarmDeletion() const     { return mConfirmAlarmDeletion; }
      int            daemonTrayCheckInterval() const  { return mDaemonTrayCheckInterval; }
      MailClient     emailClient() const              { return mEmailClient; }
      QColor         expiredColour() const            { return mExpiredColour; }
      int            expiredKeepDays() const          { return mExpiredKeepDays; }
      const QTime&   startOfDay() const               { return mStartOfDay; }
      bool           startOfDayChanged() const        { return mStartOfDayChanged; }
      bool           defaultLateCancel() const        { return mDefaultLateCancel; }
      bool           defaultConfirmAck() const        { return mDefaultConfirmAck; }
      bool           defaultBeep() const              { return mDefaultBeep; }
      bool           defaultEmailBcc() const          { return mDefaultEmailBcc; }
      RecurrenceEdit::RepeatType
                     defaultRecurPeriod() const       { return mDefaultRecurPeriod; }

      void           loadSettings();
      void           saveSettings(bool syncToDisc = true);
      void           updateStartOfDayCheck();
      void           emitSettingsChanged();

      static const QColor     default_defaultBgColour;
      static const QFont      default_messageFont;
      static const QTime      default_startOfDay;
      static const bool       default_runInSystemTray;
      static const bool       default_disableAlarmsIfStopped;
      static const bool       default_autostartTrayIcon;
      static const bool       default_confirmAlarmDeletion;
      static const int        default_daemonTrayCheckInterval;
      static const MailClient default_emailClient;
      static const QColor     default_expiredColour;
      static const int        default_expiredKeepDays;
      static const bool       default_defaultLateCancel;
      static const bool       default_defaultConfirmAck;
      static const bool       default_defaultBeep;
      static const bool       default_defaultEmailBcc;
      static const RecurrenceEdit::RepeatType
                              default_defaultRecurPeriod;
      bool                mRunInSystemTray;
      bool                mDisableAlarmsIfStopped;
      bool                mAutostartTrayIcon;
      bool                mConfirmAlarmDeletion;
      int                 mDaemonTrayCheckInterval;
      MailClient          mEmailClient;
      QTime               mStartOfDay;
      QColor              mDefaultBgColour;
      QFont               mMessageFont;
      QColor              mExpiredColour;
      int                 mExpiredKeepDays;
      // Default settings for Edit Alarm dialog
      bool                mDefaultLateCancel;
      bool                mDefaultConfirmAck;
      bool                mDefaultBeep;
      bool                mDefaultEmailBcc;
      RecurrenceEdit::RepeatType  mDefaultRecurPeriod;
      bool                mStartOfDayChanged;   // start-of-day check value doesn't tally with mStartOfDay

   signals:
      void settingsChanged();
   private:
      int          startOfDayCheck() const;
};

#endif // PREFSETTINGS_H
