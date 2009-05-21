/*
    Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Copyright (c) 2009 Andras Mantia <andras@kdab.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef TASKSHANDLER_H
#define TASKSHANDLER_H

#include "kolabhandler.h"
#include <kcal/todo.h>
#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KCal::Todo> TodoPtr;

/**
	@author Andras Mantia <amantia@kde.org>
*/
class TasksHandler : public KolabHandler {
public:
    TasksHandler(const QString& timezoneId);

    virtual ~TasksHandler();

    virtual Akonadi::Item::List translateItems(const Akonadi::Item::List & addrs);
    virtual void toKolabFormat(const Akonadi::Item& item, Akonadi::Item &imapItem);
    virtual QStringList contentMimeTypes();

private:
    KCal::Todo *todoFromKolab(MessagePtr data);
    KMime::Content *findContentByName(MessagePtr data, const QString &name, QByteArray &type
    );

};

#endif