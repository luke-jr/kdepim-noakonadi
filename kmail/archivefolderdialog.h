/* Copyright 2009 Klarälvdalens Datakonsult AB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ARCHIVEFOLDERDIALOG_H
#define ARCHIVEFOLDERDIALOG_H

#include <kdialogbase.h>

class KURLRequester;
class KComboBox;
class KMFolder;

namespace KMail
{
class FolderRequester;

class ArchiveFolderDialog : public KDialogBase
{
  Q_OBJECT

  public:

    ArchiveFolderDialog( QWidget *parent = 0 );
    void setFolder( KMFolder *defaultFolder );

  protected slots:

    void slotFixFileExtension();

    /** reimp */
    virtual void slotOk();

  private:

    QWidget *mParentWidget;
    FolderRequester *mFolderRequester;
    KComboBox *mFormatComboBox;
    KURLRequester *mUrlRequester;
};

}

#endif
