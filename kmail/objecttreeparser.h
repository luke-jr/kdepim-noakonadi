/*  -*- c++ -*-
    objecttreeparser.h

    KMail, the KDE mail client.
    Copyright (c) 2002-2003 Karl-Heinz Zimmer <khz@kde.org>
    Copyright (c) 2003      Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/


#ifndef _KMAIL_OBJECTTREEPARSER_H_
#define _KMAIL_OBJECTTREEPARSER_H_

#include <cryptplugwrapper.h>

class KMReaderWin;
class QCString;
class QString;
class QWidget;
class partNode;
template <typename T> class QMemArray;
typedef QMemArray<char> QByteArray;

namespace KMail {

  class ObjectTreeParser {
  public:
    ObjectTreeParser( KMReaderWin * reader );
    virtual ~ObjectTreeParser();

    /** Parse beginning at a given node and recursively parsing
        the children of that node and it's next sibling. */
    //  Function is called internally by "parseMsg(KMMessage* msg)"
    //  and it will be replaced once KMime is alive.
    void parseObjectTree( QCString * resultStringPtr,
			  CryptPlugWrapper * useThisCryptPlug, partNode * node,
			  bool showOneMimePart=false,
			  bool keepEncryptions=false,
			  bool includeSignatures=true );

    /** Save a QByteArray into a new temp. file using the extention
        given in dirExt to compose the directory name and
        the name given in fileName as file name
        and return the path+filename.
        If parameter reader is valid the directory and file names are added
        to the reader's temp directories and temp files lists. */
    static QString byteArrayToTempFile( KMReaderWin* reader,
                                        const QString& dirExt,
                                        const QString& fileName,
                                        const QByteArray& theBody );

  private:
    /** 1. Create a new partNode using 'content' data and Content-Description
            found in 'cntDesc'.
        2. Make this node the child of 'node'.
        3. Insert the respective entries in the Mime Tree Viewer.
        3. Parse the 'node' to display the content. */
    //  Function will be replaced once KMime is alive.
    void insertAndParseNewChildNode( QCString * resultString,
				     CryptPlugWrapper * useThisCryptPlug,
				     partNode & node,
				     const char * content,
				     const char * cntDesc,
				     bool append=false );
    /** if data is 0:
	Feeds the HTML widget with the contents of the opaque signed
            data found in partNode 'sign'.
        if data is set:
            Feeds the HTML widget with the contents of the given
            multipart/signed object.
        Signature is tested.  May contain body parts.

        Returns whether a signature was found or not: use this to
        find out if opaque data is signed or not. */
    bool writeOpaqueOrMultipartSignedData( QCString * resultString,
					   CryptPlugWrapper * useThisCryptPlug,
					   partNode * data,
					   partNode & sign,
					   const QString & fromAddress,
					   bool doCheck=true,
					   QCString * cleartextData=0,
					   struct CryptPlugWrapper::SignatureMetaData * paramSigMeta=0,
					   bool hideErrors=false );

    /** find a plugin matching a given libName */
    bool foundMatchingCryptPlug( const QString & libName,
				 CryptPlugWrapper** useThisCryptPlug_ref,
				 const QString & verboseName=QString::null );

    /** Returns the contents of the given multipart/encrypted
        object. Data is decypted.  May contain body parts. */
    bool okDecryptMIME( CryptPlugWrapper*     useThisCryptPlug,
			partNode& data,
			QCString& decryptedData,
			bool& signatureFound,
			struct CryptPlugWrapper::SignatureMetaData& sigMeta,
			bool showWarning,
			bool& passphraseError,
			QString& aErrorText );

  private:
    KMReaderWin * mReader;
  };

}; // namespace KMail

#endif // _KMAIL_OBJECTTREEPARSER_H_

