/*
    qgpgmeimportjob.cpp

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qgpgmeimportjob.h"

#include <qgpgme/eventloopinteractor.h>
#include <qgpgme/dataprovider.h>

#include <gpgmepp/context.h>
#include <gpgmepp/importresult.h>
#include <gpgmepp/data.h>

#include <assert.h>

Kleo::QGpgMEImportJob::QGpgMEImportJob( GpgME::Context * context )
  : ImportJob( QGpgME::EventLoopInteractor::instance(), "Kleo::QGpgMEImportJob" ),
    QGpgMEJob( this, context )
{
  assert( context );
}

Kleo::QGpgMEImportJob::~QGpgMEImportJob() {
}

void Kleo::QGpgMEImportJob::setup( const QByteArray & keyData ) {
  assert( !mInData );

  createInData( keyData );
}

GpgME::Error Kleo::QGpgMEImportJob::start( const QByteArray & keyData ) {
  setup( keyData );

  hookupContextToEventLoopInteractor();

  const GpgME::Error err = mCtx->startKeyImport( *mInData );
						  
  if ( err )
    deleteLater();
  return err;
}

GpgME::ImportResult Kleo::QGpgMEImportJob::exec( const QByteArray & keyData ) {
  setup( keyData );
  const GpgME::ImportResult res = mCtx->importKeys( *mInData );
  getAuditLog();
  return res;
}

void Kleo::QGpgMEImportJob::doOperationDoneEvent( const GpgME::Error & ) {
  const GpgME::ImportResult res = mCtx->importResult();
  getAuditLog();
  emit result( res );
}


#include "qgpgmeimportjob.moc"
