/*
� � This file is part of the OPIE Project
� � Copyright (c)  2002 Holger Freyther <zecke@handhelds.org>
� �                2002 Maximilian Rei� <harlekin@handhelds.org>



 �             =.            
� � � � � � �.=l.            
� � � � � �.>+-=
�_;:, � � .> � �:=|.         This library is free software; you can 
.> <`_, � > �. � <=          redistribute it and/or  modify it under
:`=1 )Y*s>-.-- � :           the terms of the GNU Library General Public
.="- .-=="i, � � .._         License as published by the Free Software
�- . � .-<_> � � .<>         Foundation; either version 2 of the License,
� � �._= =} � � � :          or (at your option) any later version.
� � .%`+i> � � � _;_.        
� � .i_,=:_. � � �-<s.       This library is distributed in the hope that  
� � �+ �. �-:. � � � =       it will be useful,  but WITHOUT ANY WARRANTY;
� � : .. � �.:, � � . . .    without even the implied warranty of
� � =_ � � � �+ � � =;=|`    MERCHANTABILITY or FITNESS FOR A
� _.=:. � � � : � �:=>`:     PARTICULAR PURPOSE. See the GNU
..}^=.= � � � = � � � ;      Library General Public License for more
++= � -. � � .` � � .:       details.
�: � � = �...= . :.=-        
�-. � .:....=;==+<;          You should have received a copy of the GNU
� -_. . . � )=. �=           Library General Public License along with
� � -- � � � �:-=`           this library; see the file COPYING.LIB. 
                             If not, write to the Free Software Foundation,
                             Inc., 59 Temple Place - Suite 330,
                             Boston, MA 02111-1307, USA.

*/



#ifndef ksync_manipulator_h
#define ksync_manipulator_h

#include <qpixmap.h>
#include <qstring.h>
#include <kparts/part.h>
#include <qptrlist.h>
#include <qstringlist.h>


#include <ksyncentry.h>

namespace KitchenSync {

  class ManipulatorPart : public KParts::Part {
    Q_OBJECT
    public:
     ManipulatorPart(QObject *parent = 0, const char *name  = 0 );
     virtual ~ManipulatorPart() {};
     // the Type this Part understands/ is able to interpret
     virtual QString type()const {return QString::null; };

     virtual int progress()const { return 0; };
     //virtual QString identifier()const { return QString::null; };
     virtual QString name()const { return QString::null; };

     virtual QString description()const { return QString::null; };
     virtual QPixmap *pixmap() { return 0l; };
     virtual QString iconName() const {return QString::null; };

     virtual bool partIsVisible()const { return false; }
     virtual bool configIsVisible()const { return true; }

     virtual QWidget *configWidget(){ return 0l; };

     virtual QPtrList<KSyncEntry> processEntry(QPtrList<KSyncEntry>* ) {
       QPtrList<KSyncEntry> ent;  return ent;
     };
    public slots:
     virtual void slotConfigOk() { };
  };
};

#endif
