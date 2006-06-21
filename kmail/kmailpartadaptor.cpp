/*
 * This file was generated by dbusidl2cpp version 0.6
 * Command line was: dbusidl2cpp -m -a kmailpartadaptor -- org.kde.kmail.kmailpart.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#include "kmailpartadaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class KmailpartAdaptor
 */

KmailpartAdaptor::KmailpartAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

KmailpartAdaptor::~KmailpartAdaptor()
{
    // destructor
}

void KmailpartAdaptor::exit()
{
    // handle method call org.kde.kmail.kmailpart.exit
    QMetaObject::invokeMethod(parent(), "exit");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->exit();
}

void KmailpartAdaptor::save()
{
    // handle method call org.kde.kmail.kmailpart.save
    QMetaObject::invokeMethod(parent(), "save");

    // Alternative:
    //static_cast<YourObjectType *>(parent())->save();
}


#include "kmailpartadaptor.moc"
