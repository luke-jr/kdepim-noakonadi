/*
    This file is part of KAddressBook.
    Copyright (c) 2002 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef ADDHOSTDIALOG_H
#define ADDHOSTDIALOG_H

#include <kdialogbase.h>

class QLineEdit;
class QPushButton;
class QSpinBox;

class AddHostDialog : public KDialogBase
{
    Q_OBJECT

  public:
    AddHostDialog( QWidget* parent = 0, const char* name = 0 );
    ~AddHostDialog();

    void setHost( const QString &host );
    void setPort( int port );
    void setBaseDN( const QString &baseDN );

    QString host() const;
    int port() const;
    QString baseDN() const;

  private slots:
    void slotHostEditChanged( const QString& );

  private:
    QLineEdit* mHostEdit;
    QSpinBox* mPortSpinBox;
    QLineEdit* mBaseEdit;
};

#endif // ADDHOSTDIALOG_H
