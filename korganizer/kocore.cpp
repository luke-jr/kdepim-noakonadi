/*
    This file is part of KOrganizer.
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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

// $Id$

#include <qwidget.h>

#include <klibloader.h>
#include <kdebug.h>

#include <calendar/plugin.h>
#include <korganizer/part.h>

#include "koprefs.h"

#include "kocore.h"

KOCore *KOCore::mSelf = 0;

KOCore *KOCore::self()
{
  if (!mSelf) {
    mSelf = new KOCore;
  }
  
  return mSelf;
}

KOCore::KOCore() :
  mCalendarDecorationsLoaded(false),
  mPartsLoaded(false), mHolidaysLoaded(false)
{
}

KTrader::OfferList KOCore::availablePlugins(const QString &type)
{
  return KTrader::self()->query(type);
}

KOrg::Plugin *KOCore::loadPlugin(KService::Ptr service)
{
  kdDebug() << "loadPlugin: library: " << service->library() << endl;

  KLibFactory *factory = KLibLoader::self()->factory(service->library());

  if (!factory) {
    kdDebug() << "KOCore::loadPlugin(): Factory creation failed" << endl;
    return 0;
  }
  
  KOrg::PluginFactory *pluginFactory = dynamic_cast<KOrg::PluginFactory *>(factory);
  
  if (!pluginFactory) {
    kdDebug() << "KOCore::loadPlugin(): Cast to KOrg::PluginFactory failed" << endl; 
    return 0;
  }
  
  return pluginFactory->create();
}

KOrg::Plugin *KOCore::loadPlugin(const QString &name)
{
  KTrader::OfferList list = availablePlugins("Calendar/Plugin");
  KTrader::OfferList::ConstIterator it;
  for(it = list.begin(); it != list.end(); ++it) {
    if ((*it)->desktopEntryName() == name) {
      return loadPlugin(*it);
    }
  }
  return 0;
}

KOrg::CalendarDecoration *KOCore::loadCalendarDecoration(KService::Ptr service)
{
  kdDebug() << "loadCalendarDecoration: library: " << service->library() << endl;

  KLibFactory *factory = KLibLoader::self()->factory(service->library());

  if (!factory) {
    kdDebug() << "KOCore::loadCalendarDecoration(): Factory creation failed" << endl;
    return 0;
  }
  
  KOrg::CalendarDecorationFactory *pluginFactory =
      dynamic_cast<KOrg::CalendarDecorationFactory *>(factory);
  
  if (!pluginFactory) {
    kdDebug() << "KOCore::loadCalendarDecoration(): Cast failed" << endl;
    return 0;
  }
  
  return pluginFactory->create();
}

KOrg::CalendarDecoration *KOCore::loadCalendarDecoration(const QString &name)
{
  KTrader::OfferList list = availablePlugins("Calendar/Decoration");
  KTrader::OfferList::ConstIterator it;
  for(it = list.begin(); it != list.end(); ++it) {
    if ((*it)->desktopEntryName() == name) {
      return loadCalendarDecoration(*it);
    }
  }
  return 0;  
}

KOrg::Part *KOCore::loadPart(KService::Ptr service, KOrg::MainWindow *parent)
{
  kdDebug() << "loadPart: library: " << service->library() << endl;

  KLibFactory *factory = KLibLoader::self()->factory(service->library());

  if (!factory) {
    kdDebug() << "KOCore::loadPart(): Factory creation failed" << endl;
    return 0;
  }
  
  KOrg::PartFactory *pluginFactory =
      dynamic_cast<KOrg::PartFactory *>(factory);
  
  if (!pluginFactory) {
    kdDebug() << "KOCore::loadPart(): Cast failed" << endl;
    return 0;
  }
  
  return pluginFactory->create(parent);
}

KOrg::Part *KOCore::loadPart(const QString &name,KOrg::MainWindow *parent)
{
  KTrader::OfferList list = availablePlugins("KOrg::MainWindow/Part");
  KTrader::OfferList::ConstIterator it;
  for(it = list.begin(); it != list.end(); ++it) {
    if ((*it)->desktopEntryName() == name) {
      return loadPart(*it,parent);
    }
  }
  return 0;  
}

KOrg::CalendarDecoration::List KOCore::calendarDecorations()
{
  if (!mCalendarDecorationsLoaded) {
    QStringList selectedPlugins = KOPrefs::instance()->mSelectedPlugins;

    mCalendarDecorations.clear();
    KTrader::OfferList plugins = availablePlugins("Calendar/Decoration");
    KTrader::OfferList::ConstIterator it;
    for(it = plugins.begin(); it != plugins.end(); ++it) {
      if ((*it)->hasServiceType("Calendar/Decoration")) {
        if (selectedPlugins.find((*it)->desktopEntryName()) != selectedPlugins.end()) {
          mCalendarDecorations.append(loadCalendarDecoration(*it));
        }
      }
    }
    mCalendarDecorationsLoaded = true;
  }
  
  return mCalendarDecorations;
}

KOrg::Part::List KOCore::parts(KOrg::MainWindow *parent)
{
  if (!mPartsLoaded) {
    mParts.clear();
    KTrader::OfferList plugins = availablePlugins("KOrganizer/Part");
    KTrader::OfferList::ConstIterator it;
    for(it = plugins.begin(); it != plugins.end(); ++it) {
      mParts.append(loadPart(*it,parent));
    }
    mPartsLoaded = true;
  }
  
  return mParts;
}

void KOCore::reloadPlugins()
{
  mCalendarDecorationsLoaded = false;
  calendarDecorations();
}

QString KOCore::holiday(const QDate &date)
{
  if (!mHolidaysLoaded) {
    mHolidays = dynamic_cast<KOrg::CalendarDecoration *>(loadPlugin("holidays"));
    mHolidaysLoaded = true;
  }
  
  if (mHolidays)
    return mHolidays->shortText(date);
  else
    return QString::null;
}
