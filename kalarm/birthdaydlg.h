/*
 *  birthdaydlg.h  -  dialog to pick birthdays from address book
 *  Program:  kalarm
 *  (C) 2002 by David Jarvie  software@astrojar.org.uk
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
#ifndef BIRTHDAYDLG_H
#define BIRTHDAYDLG_H

#include <kdialogbase.h>
#include <kabc/addressee.h>

#include "msgevent.h"

class KListView;
class QLineEdit;
class SpinBox;
class CheckBox;
class ColourCombo;
class SoundPicker;
namespace KABC { class AddressBook; }


class BirthdayDlg : public KDialogBase
{
		Q_OBJECT
	public:
		BirthdayDlg(QWidget* parent = 0);
		virtual ~BirthdayDlg();
		QValueList<KAlarmEvent> events() const;

	protected slots:
		virtual void            slotOk();
	private slots:
		void                    slotSelectionChanged();

	private:
		KListView*            mAddresseeList;
		QLineEdit*            mPrefix;
		QLineEdit*            mSuffix;
		SpinBox*              mAdvance;
		SoundPicker*          mSoundPicker;
		ColourCombo*          mBgColourChoose;
		CheckBox*             mConfirmAck;
		CheckBox*             mLateCancel;
		KABC::AddressBook*    mAddressBook;
		int                   mFlags;        // event flag bits
};

#endif // BIRTHDAYDLG_H
