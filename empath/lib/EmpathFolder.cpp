/*
    Empath - Mailer for KDE
    
    Copyright (C) 1998, 1999 Rik Hemsley rik@kde.org
    
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
*/

// Qt includes
#include <qregexp.h>

// KDE includes
#include <kglobal.h>
#include <kstddirs.h>

// Local includes
#include "Empath.h"
#include "EmpathFolderList.h"
#include "EmpathFolder.h"
#include "EmpathDefines.h"
#include "EmpathMailbox.h"
#include "EmpathIndex.h"
#include "EmpathUtilities.h"
#include "EmpathIndexAllocator.h"

EmpathFolder::EmpathFolder()
    :    QObject()
{
    empathDebug("default ctor !");
    pixmapName_ = "mini-folder-grey";
}

EmpathFolder::EmpathFolder(const EmpathURL & url)
    :   QObject(),
        url_(url)
{
    empathDebug("ctor with url == \"" + url_.asString() + "\"");
    
    index_ = new EmpathIndex(url);
	
    QObject::connect(this, SIGNAL(countUpdated(Q_UINT32, Q_UINT32)),
        empath->mailbox(url_), SLOT(s_countUpdated(Q_UINT32, Q_UINT32)));
    
    pixmapName_ = "mini-folder-grey";
}

    bool
EmpathFolder::operator == (const EmpathFolder &) const
{
    empathDebug("operator == () called");
    return false;
}

EmpathFolder::~EmpathFolder()
{
    empathDebug("dtor");
}

    void
EmpathFolder::setPixmap(const QString & p)
{
    pixmapName_ = p;
}

    const EmpathIndexRecord *
EmpathFolder::record(const QCString & key)
{
    return index_->record(key);
}

    void
EmpathFolder::update()
{
    EmpathMailbox * m = empath->mailbox(url_);
    if (m == 0) return;
    m->sync(url_);
    Q_UINT32 unread = index_->countUnread();
    Q_UINT32 all = index_->count();

    empathDebug("emitting countUpdated(" + QString().setNum(unread) + ", " + QString().setNum(all) + ")");
    emit(countUpdated(unread , all));
}

    EmpathFolder *
EmpathFolder::parent() const
{
    QString f = url_.folderPath();
    QString m = url_.mailboxName();
    
    if (f.right(1) == "/")
        f.remove(f.length() - 1, 1);
    
    if (!f.contains("/"))
        return 0;
    
    f = f.left(f.findRev("/"));
    f += "/";
    
    EmpathURL u(url_.mailboxName(), f, QString::null);
    return empath->folder(u);
}

// vim:ts=4:sw=4:tw=78
