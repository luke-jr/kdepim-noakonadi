/*
    This file is part of kdepim.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kcal_resourcefeatureplan.h"
#include "kcal_resourcefeatureplanconfig.h"

#include <kglobal.h>
#include <klocale.h>

using namespace KCal;

typedef KRES::PluginFactory< ResourceFeaturePlan, ResourceFeaturePlanConfig > FeatureplanFactory;
// FIXME K_EXPORT_COMPONENT_FACTORY( kcal_resourcefeatureplan, FeatureplanFactory )
// Problem: How to insert the catalog!
extern "C"
{
  KDE_EXPORT void* init_kcal_resourcefeatureplan()
  {
    KGlobal::locale()->insertCatalog( "kres_featureplan" );
    return new FeatureplanFactory;
  }
}
