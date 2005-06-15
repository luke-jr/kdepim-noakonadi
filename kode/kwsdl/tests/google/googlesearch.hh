/*
    This file is part of KDE.

    Copyright (c) 2005 Tobias Koenig <tokoe@kde.org>

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    
    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef GOOGLESEARCH_H
#define GOOGLESEARCH_H

#include <qobject.h>
#include "googlesearchservice.h"

class GoogleSearch : public QObject
{
  Q_OBJECT

  public:
    GoogleSearch();

    void cachedPage( const QString &url );
    void spellingSuggestion( const QString &phrase );
    void googleSearch( const QString &query,
                       int start,
                       int maxResults,
                       bool filter,
                       const QString &restrict,
                       bool safeSearch,
                       const QString &lr,
                       const QString &ie,
                       const QString &oe );

  private slots:
    void cachedPageResult( QByteArray* );
    void spellingSuggestionResult( QString* );
    void googleSearchResult( GoogleSearchResult* );

  private:
    QString mKey;
    GoogleSearchService mService;
};

#endif
