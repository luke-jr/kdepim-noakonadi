/* This file is part of the KDE project
   Copyright (C) 2005 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef TESTLDAPCLIENT_H
#define TESTLDAPCLIENT_H

#include <qobject.h>

#include "../ldapclient.h"
typedef KPIM::LdapClient LdapClient;

class TestLDAPClient : public QObject
{
    Q_OBJECT

public:
    TestLDAPClient() {}
    void setup();
    void runAll();
    void cleanup();

    // tests
    void testIntevation();

private slots:
    void slotLDAPResult( const KPIM::LdapObject& );
    void slotLDAPError( const QString& );
    void slotLDAPDone();

private:
    bool check(const QString& txt, QString a, QString b);

    LdapClient* mClient;
};

#endif
