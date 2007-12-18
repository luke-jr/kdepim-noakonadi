/* -*- mode: c++; c-basic-offset:4 -*-
    uiserver/certificateresolver.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
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

#include "certificateresolver.h"

#include "kleo-assuan.h"

#include <models/keycache.h>

#include <gpgme++/key.h>

#include <kmime/kmime_header_parsing.h>

#include <KConfig>
#include <KConfigGroup>

#include <QByteArray>
#include <QHash>
#include <QSet>

#include <boost/bind.hpp>

#include <algorithm>
#include <iterator>

using namespace Kleo;
using namespace boost;
using namespace GpgME;
using namespace KMime::Types;
using namespace KMime::HeaderParsing;

std::vector< std::vector<Key> > CertificateResolver::resolveRecipients( const std::vector<Mailbox> & recipients, Protocol proto ) {
    std::vector< std::vector<Key> > result;
    std::transform( recipients.begin(), recipients.end(),
                    std::back_inserter( result ), bind( &resolveRecipient, _1, proto ) );
    return result;
}

std::vector<Key> CertificateResolver::resolveRecipient( const Mailbox & recipient, Protocol proto ) {
    std::vector<Key> result = PublicKeyCache::instance()->findByEMailAddress( recipient.address() );
    std::vector<Key>::iterator end = std::remove_if( result.begin(), result.end(), 
                                                     !bind( &Key::canEncrypt, _1 ) );
    
    if ( proto != UnknownProtocol )
        end = std::remove_if( result.begin(), end,
                              bind( &Key::protocol, _1 ) != proto );

    result.erase( end, result.end() );
    return result;
}

std::vector< std::vector<Key> > CertificateResolver::resolveSigners( const std::vector<Mailbox> & signers, Protocol proto ) {
    std::vector< std::vector<Key> > result;
    std::transform( signers.begin(), signers.end(),
                    std::back_inserter( result ), bind( &resolveSigner, _1, proto ) );
    return result;
}

std::vector<Key> CertificateResolver::resolveSigner( const Mailbox & signer, Protocol proto ) {
    std::vector<Key> result = SecretKeyCache::instance()->findByEMailAddress( signer.address() );
    std::vector<Key>::iterator end
        = std::remove_if( result.begin(), result.end(),
                          !bind( &Key::hasSecret, _1 ) );
    end = std::remove_if( result.begin(), end,
                          !bind( &Key::canSign, _1 ) );
    if ( proto != UnknownProtocol )
        end = std::remove_if( result.begin(), end,
                              bind( &Key::protocol, _1 ) != proto );
    result.erase( end, result.end() );
    return result;
}

class KConfigBasedRecipientPreferences::Private {
    friend class Kleo::KConfigBasedRecipientPreferences;
    KConfigBasedRecipientPreferences* const q;
public:
    explicit Private( KSharedConfigPtr config, KConfigBasedRecipientPreferences* qq );
    ~Private();
    
private:
    void ensurePrefsParsed() const;
    void writePrefs();
    
private:
    KSharedConfigPtr m_config;
    
    mutable QHash<QByteArray, QByteArray> pgpPrefs;
    mutable QHash<QByteArray, QByteArray> cmsPrefs;
    mutable bool m_parsed;
    mutable bool m_dirty;
};

KConfigBasedRecipientPreferences::Private::Private( KSharedConfigPtr config , KConfigBasedRecipientPreferences* qq ) : q( qq ), m_config( config ), m_parsed( false ), m_dirty( false ) 
{
    assert( m_config );
}

KConfigBasedRecipientPreferences::Private::~Private()
{
    writePrefs();
}

void KConfigBasedRecipientPreferences::Private::writePrefs()
{
    if ( !m_dirty )
        return;
    const QSet<QByteArray> keys = pgpPrefs.keys().toSet() + cmsPrefs.keys().toSet();
    KConfigGroup general( m_config, "General" );
    general.writeEntry( "numberOfEncryptionPreferences", keys.count() );
    int n = 0;
    Q_FOREACH ( const QByteArray& i, keys )
    {
        KConfigGroup group( m_config, QString( "EncryptionPreference_%1" ).arg( n++ ) );
        group.writeEntry( "email", i );
        const QByteArray pgp = pgpPrefs.value( i );
        if ( !pgp.isEmpty() )
            group.writeEntry( "pgpCertificate", pgp );
        const QByteArray cms = cmsPrefs.value( i );
        if ( !cms.isEmpty() )
            group.writeEntry( "cmsCertificate", cms );
    } 
    // TODO: ensure that there are no old entries with n > keys.count()  
    m_dirty = false;
}
void KConfigBasedRecipientPreferences::Private::ensurePrefsParsed() const
{
    if ( m_parsed )
        return;
    const KConfigGroup general( m_config, "General" );
    const int num = general.readEntry( "numberOfEncryptionPreferences", 0 );
    for ( int i = 0; i < num; ++i )
    {
        const KConfigGroup group( m_config, QString( "EncryptionPreference_%1" ).arg( i ) );
        const QByteArray id = group.readEntry( "email", QByteArray() );
        if ( id.isEmpty() )
            continue;
        pgpPrefs.insert( id, group.readEntry( "pgpCertificate", QByteArray() ) );
        cmsPrefs.insert( id, group.readEntry( "cmsCertificate", QByteArray() ) );   
    }
    m_parsed = true;
}

KConfigBasedRecipientPreferences::KConfigBasedRecipientPreferences( KSharedConfigPtr config ) : d( new Private( config, this ) )
{
}


KConfigBasedRecipientPreferences::~KConfigBasedRecipientPreferences()
{
}

Key KConfigBasedRecipientPreferences::preferredCertificate( const Mailbox& recipient, Protocol protocol )
{
    d->ensurePrefsParsed();
    
    const QByteArray keyId = ( protocol == CMS ? d->cmsPrefs : d->pgpPrefs ).value( recipient.address() );
    return PublicKeyCache::instance()->findByKeyIDOrFingerprint( keyId );
}

void KConfigBasedRecipientPreferences::setPreferredCertificate( const Mailbox& recipient, Protocol protocol, const Key& certificate )
{
    d->ensurePrefsParsed();
    if ( !recipient.hasAddress() )
        return;
    ( protocol == CMS ? d->cmsPrefs : d->pgpPrefs ).insert( recipient.address(), certificate.keyID() );
    d->m_dirty = true;
}

