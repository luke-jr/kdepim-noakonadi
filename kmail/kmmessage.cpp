// -*- mode: C++; c-file-style: "gnu" -*-
// kmmessage.cpp

#include "kmmessage.h"

#include <config-kmail.h>

// if you do not want GUI elements in here then set ALLOW_GUI to 0.
// needed temporarily until KMime is replacing the partNode helper class:
#include "partNode.h"
#define ALLOW_GUI 1
#include "kmkernel.h"
#include "mailinglist-magic.h"
#include "messageproperty.h"
using KMail::MessageProperty;
#include "objecttreeparser.h"
using KMail::ObjectTreeParser;
#include "kmfolderindex.h"
#include "undostack.h"
#include "kmversion.h"
#include <version-kmail.h>
#include "kmmessagetag.h"
#include "headerstrategy.h"
#include "globalsettings.h"
using KMail::HeaderStrategy;
#include "kmaddrbook.h"
#include "kcursorsaver.h"
#include "templateparser.h"
using KMail::TemplateParser;
#include "stringutil.h"
#include "mdnadvicedialog.h"

#include <kpimidentities/identity.h>
#include <kpimidentities/identitymanager.h>
#include <kpimutils/email.h>

#include <libkpgp/kpgpblock.h>
#include <kaddrbookexternal.h>

#include <kglobal.h>
#include <kascii.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kconfig.h>
#include <khtml_part.h>
#include <kuser.h>
#include <kconfiggroup.h>
#include <KProtocolManager>

#include <QList>
#include <QCursor>
#include <QMessageBox>
#include <QTextCodec>
#include <QByteArray>
#include <QHostInfo>

#include <kmime/kmime_dateformatter.h>
#include <kmime/kmime_charfreq.h>
#include <kmime/kmime_header_parsing.h>
using KMime::HeaderParsing::parseAddressList;
using namespace KMime::Types;

#include <mimelib/body.h>
#include <mimelib/field.h>
#include <mimelib/mimepp.h>
#include <mimelib/string.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <klocale.h>
#include <stdlib.h>
#include <unistd.h>

#if ALLOW_GUI
#include <kmessagebox.h>
#include "util.h"
#endif

using namespace KMail;
using namespace KMime;

struct KMMessageStaticData
{
  KMMessageStaticData()
    : emptyString(""), headerStrategy( HeaderStrategy::rich() )
  {
  }

  DwString emptyString;

  // Values that are set from the config file with KMMessage::readConfig()
  bool smartQuote : 1, wordWrap : 1;
  int wrapCol;
  QStringList prefCharsets;
  const HeaderStrategy* headerStrategy;
  QList<KMMessage*> pendingDeletes;
};

K_GLOBAL_STATIC( KMMessageStaticData, s )

//helper
static void applyHeadersToMessagePart( DwHeaders& headers, KMMessagePart* aPart );

//-----------------------------------------------------------------------------
KMMessage::KMMessage(DwMessage* aMsg)
  : KMMsgBase()
{
  init( aMsg );
  // aMsg might need assembly
  mNeedsAssembly = true;
}

//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMFolder* parent): KMMsgBase(parent)
{
  init();
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(KMMsgInfo& msgInfo): KMMsgBase()
{
  init();
  // now overwrite a few from the msgInfo
  mMsgSize = msgInfo.msgSize();
  mFolderOffset = msgInfo.folderOffset();
  mStatus = msgInfo.status();
  mEncryptionState = msgInfo.encryptionState();
  mSignatureState = msgInfo.signatureState();
  mMDNSentState = msgInfo.mdnSentState();
  mDate = msgInfo.date();
  mFileName = msgInfo.fileName();
  if ( msgInfo.tagList() ) {
    if ( !mTagList )
      mTagList = new KMMessageTagList();
    *mTagList = *msgInfo.tagList();
  } else {
    delete mTagList;
    mTagList = 0;
  }
  KMMsgBase::assign(&msgInfo);
}


//-----------------------------------------------------------------------------
KMMessage::KMMessage(const KMMessage& other) :
    KMMsgBase( other ),
    ISubject(),
    mMsg(0)
{
  init(); // to be safe
  assign( other );
}

void KMMessage::init( DwMessage* aMsg )
{
  mNeedsAssembly = false;
  if ( aMsg ) {
    mMsg = aMsg;
  } else {
  mMsg = new DwMessage;
  }
  mOverrideCodec = 0;
  mDecodeHTML = false;
  mComplete = true;
  mReadyToShow = true;
  mMsgSize = 0;
  mMsgLength = 0;
  mFolderOffset = 0;
  mStatus.clear();
  mStatus.setNew();
  mEncryptionState = KMMsgEncryptionStateUnknown;
  mSignatureState = KMMsgSignatureStateUnknown;
  mMDNSentState = KMMsgMDNStateUnknown;
  mDate    = 0;
  mUnencryptedMsg = 0;
  mLastUpdated = 0;
  mTagList = 0;
  mCursorPos = 0;
  mIsParsed = false;
}

void KMMessage::assign( const KMMessage& other )
{
  MessageProperty::forget( this );
  delete mMsg;
  delete mUnencryptedMsg;

  mNeedsAssembly = true;//other.mNeedsAssembly;
  if( other.mMsg )
    mMsg = new DwMessage( *(other.mMsg) );
  else
    mMsg = 0;
  mOverrideCodec = other.mOverrideCodec;
  mDecodeHTML = other.mDecodeHTML;
  mMsgSize = other.mMsgSize;
  mMsgLength = other.mMsgLength;
  mFolderOffset = other.mFolderOffset;
  mStatus  = other.mStatus;
  mEncryptionState = other.mEncryptionState;
  mSignatureState = other.mSignatureState;
  mMDNSentState = other.mMDNSentState;
  mIsParsed = other.mIsParsed;
  mDate    = other.mDate;
  if( other.hasUnencryptedMsg() )
    mUnencryptedMsg = new KMMessage( *other.unencryptedMsg() );
  else
    mUnencryptedMsg = 0;
  if ( other.tagList() ) {
    if ( !mTagList )
      mTagList = new KMMessageTagList();
    *mTagList = *other.tagList();
  } else {
    delete mTagList;
    mTagList = 0;
  }
  setDrafts( other.drafts() );
  setTemplates( other.templates() );
  //mFileName = ""; // we might not want to copy the other messages filename (?)
  //KMMsgBase::assign( &other );
}

//-----------------------------------------------------------------------------
KMMessage::~KMMessage()
{
  delete mMsg;
  kmkernel->undoStack()->msgDestroyed( this );
}


//-----------------------------------------------------------------------------
void KMMessage::setReferences(const QByteArray& aStr)
{
  if (aStr.isNull()) return;
  mMsg->Headers().References().FromString(aStr);
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::id() const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasMessageId())
    return header.MessageId().AsString().c_str();
  else
    return "";
}


//-----------------------------------------------------------------------------
//WARNING: This method updates the memory resident cache of serial numbers
//WARNING: held in MessageProperty, but it does not update the persistent
//WARNING: store of serial numbers on the file system that is managed by
//WARNING: KMMsgDict
void KMMessage::setMsgSerNum(unsigned long newMsgSerNum)
{
  MessageProperty::setSerialCache( this, newMsgSerNum );
}


//-----------------------------------------------------------------------------
bool KMMessage::isMessage() const
{
  return true;
}

//-----------------------------------------------------------------------------
bool KMMessage::transferInProgress() const
{
  return MessageProperty::transferInProgress( getMsgSerNum() );
}


//-----------------------------------------------------------------------------
void KMMessage::setTransferInProgress(bool value, bool force)
{
  MessageProperty::setTransferInProgress( getMsgSerNum(), value, force );
  if ( !transferInProgress() && s->pendingDeletes.contains( this ) ) {
    s->pendingDeletes.removeAll( this );
    if ( parent() ) {
      int idx = parent()->find( this );
      if ( idx > 0 ) {
        parent()->removeMsg( idx );
      }
    }
  }
}



bool KMMessage::isUrgent() const {
  return headerField( "Priority" ).contains( "urgent", Qt::CaseSensitive )
    || headerField( "X-Priority" ).startsWith( '2' )
    || headerField( "X-Priority" ).startsWith( '1' );
}

//-----------------------------------------------------------------------------
void KMMessage::setUnencryptedMsg( KMMessage* unencrypted )
{
  delete mUnencryptedMsg;
  mUnencryptedMsg = unencrypted;
}

//-----------------------------------------------------------------------------
const DwString& KMMessage::asDwString() const
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = false;
    mMsg->Assemble();
  }
  return mMsg->AsString();
}

//-----------------------------------------------------------------------------
const DwMessage* KMMessage::asDwMessage()
{
  if (mNeedsAssembly)
  {
    mNeedsAssembly = false;
    mMsg->Assemble();
  }
  return mMsg;
}

//-----------------------------------------------------------------------------
QByteArray KMMessage::asString() const {
  return QByteArray( asDwString().c_str() );
}


QByteArray KMMessage::asSendableString() const
{
  KMMessage msg( new DwMessage( *this->mMsg ) );
  msg.removePrivateHeaderFields();
  msg.removeHeaderField("Bcc");
  return msg.asString();
}

QByteArray KMMessage::headerAsSendableString() const
{
  KMMessage msg( new DwMessage( *this->mMsg ) );
  msg.removePrivateHeaderFields();
  msg.removeHeaderField("Bcc");
  return msg.headerAsString().toLatin1();
}

void KMMessage::removePrivateHeaderFields() {
  removeHeaderField("Status");
  removeHeaderField("X-Status");
  removeHeaderField("X-KMail-EncryptionState");
  removeHeaderField("X-KMail-SignatureState");
  removeHeaderField("X-KMail-MDN-Sent");
  removeHeaderField("X-KMail-Transport");
  removeHeaderField("X-KMail-Identity");
  removeHeaderField("X-KMail-Fcc");
  removeHeaderField("X-KMail-Redirect-From");
  removeHeaderField("X-KMail-Link-Message");
  removeHeaderField("X-KMail-Link-Type");
  removeHeaderField( "X-KMail-QuotePrefix" );
}

//-----------------------------------------------------------------------------
void KMMessage::setStatusFields()
{
  char str[2] = { 0, 0 };

  setHeaderField( "Status", mStatus.isNew() ? "R" : "RO" );
  setHeaderField( "X-Status", mStatus.getStatusStr() );

  str[0] = (char)encryptionState();
  setHeaderField("X-KMail-EncryptionState", str);

  str[0] = (char)signatureState();
  //kDebug() << "Setting SignatureState header field to" << str[0];
  setHeaderField("X-KMail-SignatureState", str);

  str[0] = static_cast<char>( mdnSentState() );
  setHeaderField("X-KMail-MDN-Sent", str);

  // We better do the assembling ourselves now to prevent the
  // mimelib from changing the message *body*.  (khz, 10.8.2002)
  mNeedsAssembly = false;
  mMsg->Headers().Assemble();
  mMsg->Assemble( mMsg->Headers(),
                  mMsg->Body() );
}


//----------------------------------------------------------------------------
QString KMMessage::headerAsString() const
{
  DwHeaders& header = mMsg->Headers();
  header.Assemble();
  if ( header.AsString().empty() )
    return QString();
  return QString::fromLatin1( header.AsString().c_str() );
}


//-----------------------------------------------------------------------------
DwMediaType& KMMessage::dwContentType()
{
  return mMsg->Headers().ContentType();
}

void KMMessage::fromString( const QByteArray & ba, bool setStatus ) {
  return fromDwString( DwString( ba.data(), ba.size() ), setStatus );
}

void KMMessage::fromDwString(const DwString& str, bool aSetStatus)
{
  delete mMsg;
  mMsg = new DwMessage;
  mMsg->FromString( str );
  mMsg->Parse();

  if (aSetStatus) {
    setStatus(headerField("Status").toLatin1(), headerField("X-Status").toLatin1());
    if ( !headerField( "X-KMail-EncryptionState" ).isEmpty() )
      setEncryptionStateChar( headerField( "X-KMail-EncryptionState" ).at(0) );
    if ( !headerField( "X-KMail-SignatureState" ).isEmpty() )
      setSignatureStateChar( headerField( "X-KMail-SignatureState" ).at(0));
    if ( !headerField("X-KMail-MDN-Sent").isEmpty() )
      setMDNSentState( static_cast<KMMsgMDNSentState>( headerField("X-KMail-MDN-Sent").at(0).toLatin1() ) );
  }
  if (attachmentState() == KMMsgAttachmentUnknown && readyToShow())
    updateAttachmentState();

  mNeedsAssembly = false;
  mDate = date();
}

//-----------------------------------------------------------------------------
void KMMessage::parseTextStringFromDwPart( partNode * root,
                                           QByteArray& parsedString,
                                           const QTextCodec*& codec,
                                           bool& isHTML ) const
{
  if ( !root )
    return;

  isHTML = false;
  partNode * curNode = root->findType( DwMime::kTypeText,
                                       DwMime::kSubtypeUnknown,
                                       true, false );
  kDebug() << ( curNode ? "text part found!\n" : "sorry, no text node!\n" );
  if( curNode ) {
    isHTML = DwMime::kSubtypeHtml == curNode->subType();
    // now parse the TEXT message part we want to quote
    ObjectTreeParser otp( 0, 0, true, false, true );
    otp.parseObjectTree( curNode );
    parsedString = otp.rawReplyString();
    codec = curNode->msgPart().codec();
  }
}

//-----------------------------------------------------------------------------

QString KMMessage::asPlainTextFromObjectTree( partNode *root, bool aStripSignature,
                                              bool allowDecryption ) const
{
  Q_ASSERT( root );
  Q_ASSERT( root->processed() );

  QByteArray parsedString;
  bool isHTML = false;
  const QTextCodec * codec = 0;

  if ( !root ) return QString();
  parseTextStringFromDwPart( root, parsedString, codec, isHTML );

  if ( mOverrideCodec || !codec )
    codec = this->codec();

  if ( parsedString.isEmpty() )
    return QString();

  bool clearSigned = false;
  QString result;

  // decrypt
  if ( allowDecryption ) {
    QList<Kpgp::Block> pgpBlocks;
    QList<QByteArray>  nonPgpBlocks;
    if ( Kpgp::Module::prepareMessageForDecryption( parsedString,
                                                    pgpBlocks,
                                                    nonPgpBlocks ) ) {
      // Only decrypt/strip off the signature if there is only one OpenPGP
      // block in the message
      if ( pgpBlocks.count() == 1 ) {
        Kpgp::Block &block = pgpBlocks.first();
        if ( block.type() == Kpgp::PgpMessageBlock ||
             block.type() == Kpgp::ClearsignedBlock ) {
          if ( block.type() == Kpgp::PgpMessageBlock ) {
            // try to decrypt this OpenPGP block
            block.decrypt();
          } else {
            // strip off the signature
            block.verify();
            clearSigned = true;
          }

          result = codec->toUnicode( nonPgpBlocks.first() )
              + codec->toUnicode( block.text() )
              + codec->toUnicode( nonPgpBlocks.last() );
        }
      }
    }
  }

  if ( result.isEmpty() ) {
    result = codec->toUnicode( parsedString );
    if ( result.isEmpty() )
      return result;
  }

  // html -> plaintext conversion, if necessary:
  if ( isHTML && mDecodeHTML ) {
    KHTMLPart htmlPart;
    htmlPart.setOnlyLocalReferences( true );
    htmlPart.setMetaRefreshEnabled( false );
    htmlPart.setPluginsEnabled( false );
    htmlPart.setJScriptEnabled( false );
    htmlPart.setJavaEnabled( false );
    htmlPart.begin();
    htmlPart.write( result );
    htmlPart.end();
    htmlPart.selectAll();
    result = htmlPart.selectedText();
  }

  // strip the signature (footer):
  if ( aStripSignature )
    return StringUtil::stripSignature( result, clearSigned );
  else
    return result;
}

QString KMMessage::asPlainText( bool aStripSignature, bool allowDecryption ) const
{
  partNode *root = partNode::fromMessage( this );
  if ( !root )
    return QString();

  ObjectTreeParser otp;
  otp.parseObjectTree( root );
  QString result = asPlainTextFromObjectTree( root, aStripSignature, allowDecryption );
  delete root;
  return result;
}

//-----------------------------------------------------------------------------

QString KMMessage::asQuotedString( const QString& aIndentStr,
                                   const QString& selection /*.clear() */,
                                   bool aStripSignature /* = true */,
                                   bool allowDecryption /* = true */) const
{
  QString content = selection.isEmpty() ?
    asPlainText( aStripSignature, allowDecryption ) : selection ;

  // Remove blank lines at the beginning:
  const int firstNonWS = content.indexOf( QRegExp( "\\S" ) );
  const int lineStart = content.lastIndexOf( '\n', firstNonWS );
  if ( lineStart >= 0 )
    content.remove( 0, static_cast<unsigned int>( lineStart ) );

  const QString indentStr = StringUtil::formatString( aIndentStr, from() );

  if ( s->smartQuote && s->wordWrap )
    content = StringUtil::smartQuote( content, s->wrapCol - indentStr.length() );

  content.replace( '\n', '\n' + indentStr );
  content.prepend( indentStr );
  content += '\n';

  return content;
}

//-----------------------------------------------------------------------------
KMMessage::MessageReply KMMessage::createReply( ReplyStrategy replyStrategy,
                                                const QString &selection /*.clear() */,
                                                bool noQuote /* = false */,
                                                bool allowDecryption /* = true */,
                                                bool selectionIsBody /* = false */,
                                                const QString &tmpl /* = QString() */ )
{
  KMMessage* msg = new KMMessage;
  QString str, mailingListStr, replyToStr, toStr;
  QStringList mailingListAddresses;
  QByteArray refStr, headerName;
  bool replyAll = true;

  msg->initFromMessage(this);
  MailingList::name(this, headerName, mailingListStr);
  replyToStr = replyTo();

  msg->setCharset("utf-8");

  // determine the mailing list posting address
  if ( parent() && parent()->isMailingListEnabled() &&
       !parent()->mailingListPostAddress().isEmpty() ) {
    mailingListAddresses << parent()->mailingListPostAddress();
  }
  if ( headerField("List-Post").contains( "mailto:", Qt::CaseInsensitive ) ) {
    QString listPost = headerField("List-Post");
    QRegExp rx( "<mailto:([^@>]+)@([^>]+)>", Qt::CaseInsensitive );
    if ( rx.indexIn( listPost, 0 ) != -1 ) // matched
      mailingListAddresses << rx.cap(1) + '@' + rx.cap(2);
  }

  switch( replyStrategy ) {
  case ReplySmart : {
    if ( !headerField( "Mail-Followup-To" ).isEmpty() ) {
      toStr = headerField( "Mail-Followup-To" );
    }
    else if ( !replyToStr.isEmpty() ) {
      toStr = replyToStr;
      // use the ReplyAll template only when it's a reply to a mailing list
      if ( mailingListAddresses.isEmpty() )
        replyAll = false;
    }
    else if ( !mailingListAddresses.isEmpty() ) {
      toStr = mailingListAddresses[0];
    }
    else {
      // doesn't seem to be a mailing list, reply to From: address
      toStr = from();
      replyAll = false;
    }
    // strip all my addresses from the list of recipients
    QStringList recipients = KPIMUtils::splitAddressList( toStr );
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");
    // ... unless the list contains only my addresses (reply to self)
    if ( toStr.isEmpty() && !recipients.isEmpty() )
      toStr = recipients[0];

    break;
  }
  case ReplyList : {
    if ( !headerField( "Mail-Followup-To" ).isEmpty() ) {
      toStr = headerField( "Mail-Followup-To" );
    }
    else if ( !mailingListAddresses.isEmpty() ) {
      toStr = mailingListAddresses[0];
    }
    else if ( !replyToStr.isEmpty() ) {
      // assume a Reply-To header mangling mailing list
      toStr = replyToStr;
    }
    // strip all my addresses from the list of recipients
    QStringList recipients = KPIMUtils::splitAddressList( toStr );
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");

    break;
  }
  case ReplyAll : {
    QStringList recipients;
    QStringList ccRecipients;

    // add addresses from the Reply-To header to the list of recipients
    if( !replyToStr.isEmpty() ) {
      recipients += KPIMUtils::splitAddressList( replyToStr );
      // strip all possible mailing list addresses from the list of Reply-To
      // addresses
      for ( QStringList::const_iterator it = mailingListAddresses.constBegin();
            it != mailingListAddresses.constEnd();
            ++it ) {
        recipients = StringUtil::stripAddressFromAddressList( *it, recipients );
      }
    }

    if ( !mailingListAddresses.isEmpty() ) {
      // this is a mailing list message
      if ( recipients.isEmpty() && !from().isEmpty() ) {
        // The sender didn't set a Reply-to address, so we add the From
        // address to the list of CC recipients.
        ccRecipients += from();
        kDebug() << "Added" << from() <<"to the list of CC recipients";
      }
      // if it is a mailing list, add the posting address
      recipients.prepend( mailingListAddresses[0] );
    }
    else {
      // this is a normal message
      if ( recipients.isEmpty() && !from().isEmpty() ) {
        // in case of replying to a normal message only then add the From
        // address to the list of recipients if there was no Reply-to address
        recipients += from();
        kDebug() << "Added" << from() <<"to the list of recipients";
      }
    }

    // strip all my addresses from the list of recipients
    toStr = StringUtil::stripMyAddressesFromAddressList( recipients ).join(", ");

    // merge To header and CC header into a list of CC recipients
    if( !cc().isEmpty() || !to().isEmpty() ) {
      QStringList list;
      if (!to().isEmpty())
        list += KPIMUtils::splitAddressList(to());
      if (!cc().isEmpty())
        list += KPIMUtils::splitAddressList(cc());
      for( QStringList::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it ) {
        if(    !StringUtil::addressIsInAddressList( *it, recipients )
            && !StringUtil::addressIsInAddressList( *it, ccRecipients ) ) {
          ccRecipients += *it;
          kDebug() << "Added" << *it <<"to the list of CC recipients";
        }
      }
    }

    if ( !ccRecipients.isEmpty() ) {
      // strip all my addresses from the list of CC recipients
      ccRecipients = StringUtil::stripMyAddressesFromAddressList( ccRecipients );

      // in case of a reply to self toStr might be empty. if that's the case
      // then propagate a cc recipient to To: (if there is any).
      if ( toStr.isEmpty() && !ccRecipients.isEmpty() ) {
        toStr = ccRecipients[0];
        ccRecipients.pop_front();
      }

      msg->setCc( ccRecipients.join(", ") );
    }

    if ( toStr.isEmpty() && !recipients.isEmpty() ) {
      // reply to self without other recipients
      toStr = recipients[0];
    }
    break;
  }
  case ReplyAuthor : {
    if ( !replyToStr.isEmpty() ) {
      QStringList recipients = KPIMUtils::splitAddressList( replyToStr );
      // strip the mailing list post address from the list of Reply-To
      // addresses since we want to reply in private
      for ( QStringList::const_iterator it = mailingListAddresses.constBegin();
            it != mailingListAddresses.constEnd();
            ++it ) {
        recipients = StringUtil::stripAddressFromAddressList( *it, recipients );
      }
      if ( !recipients.isEmpty() ) {
        toStr = recipients.join(", ");
      }
      else {
        // there was only the mailing list post address in the Reply-To header,
        // so use the From address instead
        toStr = from();
      }
    }
    else if ( !from().isEmpty() ) {
      toStr = from();
    }
    replyAll = false;
    break;
  }
  case ReplyNone : {
    // the addressees will be set by the caller
  }
  }

  msg->setTo(toStr);

  refStr = getRefStr();
  if (!refStr.isEmpty())
    msg->setReferences(refStr);
  //In-Reply-To = original msg-id
  msg->setReplyToId(msgId());

  msg->setSubject( replySubject() );

  // If the reply shouldn't be blank, apply the template to the message
  if ( !noQuote ) {
    TemplateParser parser( msg, (replyAll ? TemplateParser::ReplyAll : TemplateParser::Reply),
                           selection, s->smartQuote, allowDecryption, selectionIsBody );
    if ( !tmpl.isEmpty() )
      parser.process( tmpl, this );
    else
      parser.process( this );
  }

  msg->link( this, MessageStatus::statusReplied() );

  if ( parent() && parent()->putRepliesInSameFolder() )
    msg->setFcc( parent()->idString() );

  // replies to an encrypted message should be encrypted as well
  if ( encryptionState() == KMMsgPartiallyEncrypted ||
       encryptionState() == KMMsgFullyEncrypted ) {
    msg->setEncryptionState( KMMsgFullyEncrypted );
  }

  MessageReply reply;
  reply.msg = msg;
  reply.replyAll = replyAll;
  return reply;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::getRefStr() const
{
  QByteArray firstRef, lastRef, refStr, retRefStr;
  int i, j;

  refStr = headerField("References").trimmed().toLatin1();

  if (refStr.isEmpty())
    return headerField("Message-Id").toLatin1();

  i = refStr.indexOf('<');
  j = refStr.indexOf('>');
  firstRef = refStr.mid(i, j-i+1);
  if (!firstRef.isEmpty())
    retRefStr = firstRef + ' ';

  i = refStr.lastIndexOf('<');
  j = refStr.lastIndexOf('>');

  lastRef = refStr.mid(i, j-i+1);
  if (!lastRef.isEmpty() && lastRef != firstRef)
    retRefStr += lastRef + ' ';

  retRefStr += headerField("Message-Id").toLatin1();
  return retRefStr;
}


KMMessage* KMMessage::createRedirect( const QString &toStr )
{
  // copy the message 1:1
  KMMessage* msg = new KMMessage( new DwMessage( *this->mMsg ) );
  KMMessagePart msgPart;

  uint id = 0;
  QString strId = msg->headerField( "X-KMail-Identity" ).trimmed();
  if ( !strId.isEmpty())
    id = strId.toUInt();
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( id );

  // X-KMail-Redirect-From: content
  QString strByWayOf = QString("%1 (by way of %2 <%3>)")
    .arg( from() )
    .arg( ident.fullName() )
    .arg( ident.emailAddr() );

  // Resent-From: content
  QString strFrom = QString("%1 <%2>")
    .arg( ident.fullName() )
    .arg( ident.emailAddr() );

  // format the current date to be used in Resent-Date:
  QString origDate = msg->headerField( "Date" );
  msg->setDateToday();
  QString newDate = msg->headerField( "Date" );
  // make sure the Date: header is valid
  if ( origDate.isEmpty() )
    msg->removeHeaderField( "Date" );
  else
    msg->setHeaderField( "Date", origDate );

  // prepend Resent-*: headers (c.f. RFC2822 3.6.6)
  msg->setHeaderField( "Resent-Message-ID", StringUtil::generateMessageId( msg->sender() ),
                       Structured, true);
  msg->setHeaderField( "Resent-Date", newDate, Structured, true );
  msg->setHeaderField( "Resent-To",   toStr,   Address, true );
  msg->setHeaderField( "Resent-From", strFrom, Address, true );

  msg->setHeaderField( "X-KMail-Redirect-From", strByWayOf );
  msg->setHeaderField( "X-KMail-Recipients", toStr, Address );

  msg->link( this, MessageStatus::statusForwarded() );

  return msg;
}

void KMMessage::sanitizeHeaders( const QStringList& whiteList )
{
   // Strip out all headers apart from the content description and other
   // whitelisted ones, because we don't want to inherit them.
   DwHeaders& header = mMsg->Headers();
   DwField* field = header.FirstField();
   DwField* nextField;
   while (field)
   {
     nextField = field->Next();
     if ( field->FieldNameStr().find( "ontent" ) == DwString::npos
             && !whiteList.contains( QString::fromLatin1( field->FieldNameStr().c_str() ) ) )
       header.RemoveField(field);
     field = nextField;
   }
   mMsg->Assemble();
}

//-----------------------------------------------------------------------------
KMMessage* KMMessage::createForward( const QString &tmpl /* = QString() */ )
{
  KMMessage* msg = new KMMessage();

  // This is a non-multipart, non-text mail (e.g. text/calendar). Construct
  // a multipart/mixed mail and add the original body as an attachment.
  if ( type() != DwMime::kTypeMultipart &&
       ( type() != DwMime::kTypeText ||
         ( type() == DwMime::kTypeText &&
           subtype() != DwMime::kSubtypeHtml && subtype() != DwMime::kSubtypePlain ) ) ) {


    msg->initFromMessage( this );
    msg->removeHeaderField("Content-Type");
    msg->removeHeaderField("Content-Transfer-Encoding");
    // Modify the ContentType directly (replaces setAutomaticFields(true))
    DwHeaders & header = msg->mMsg->Headers();
    header.MimeVersion().FromString("1.0");
    DwMediaType & contentType = msg->dwContentType();
    contentType.SetType( DwMime::kTypeMultipart );
    contentType.SetSubtype( DwMime::kSubtypeMixed );
    contentType.CreateBoundary(0);
    contentType.Assemble();

    // empty text part
    KMMessagePart msgPart;
    bodyPart( 0, &msgPart );
    msg->addBodyPart(&msgPart);
    // the old contents of the mail
    KMMessagePart secondPart;
    secondPart.setType( type() );
    secondPart.setSubtype( subtype() );
    secondPart.setBody( mMsg->Body().AsString().c_str() );
    // use the headers of the original mail
    applyHeadersToMessagePart( mMsg->Headers(), &secondPart );
    msg->addBodyPart(&secondPart);
    msg->mNeedsAssembly = true;
    msg->cleanupHeader();
  }

  // Normal message (multipart or text/plain|html)
  // Just copy the message, the template parser will do the hard work of
  // replacing the body text in TemplateParser::addProcessedBodyToMessage()
  else {
    msg->fromDwString( this->asDwString() );
    DwMediaType oldContentType = msg->mMsg->Headers().ContentType();
    msg->sanitizeHeaders();
    msg->initFromMessage( this );

    // restore the content type, initFromMessage() sets the contents type to
    // text/plain, via initHeader(), for unclear reasons
    msg->mMsg->Headers().ContentType().FromString( oldContentType.AsString() );
    msg->mMsg->Headers().ContentType().Parse();
    msg->mMsg->Assemble();
  }

  msg->setSubject( forwardSubject() );

  TemplateParser parser( msg, TemplateParser::Forward,
                         QString(),
                         false, false, false);
  if ( !tmpl.isEmpty() )
    parser.process( tmpl, this );
  else
    parser.process( this );

  msg->link( this, MessageStatus::statusForwarded() );
  return msg;
}

static const struct {
  const char * dontAskAgainID;
  bool         canDeny;
  const char * text;
} mdnMessageBoxes[] = {
  { "mdnNormalAsk", true,
    I18N_NOOP("This message contains a request to return a notification "
              "about your reception of the message.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnUnknownOption", false,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message.\n"
              "It contains a processing instruction that is marked as "
              "\"required\", but which is unknown to KMail.\n"
              "You can either ignore the request or let KMail send a "
              "\"failed\" response.") },
  { "mdnMultipleAddressesInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but it is requested to send the notification to more "
              "than one address.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnReturnPathEmpty", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but there is no return-path set.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
  { "mdnReturnPathNotInReceiptTo", true,
    I18N_NOOP("This message contains a request to send a notification "
              "about your reception of the message,\n"
              "but the return-path address differs from the address "
              "the notification was requested to be sent to.\n"
              "You can either ignore the request or let KMail send a "
              "\"denied\" or normal response.") },
};

static const int numMdnMessageBoxes
      = sizeof mdnMessageBoxes / sizeof *mdnMessageBoxes;

static int requestAdviceOnMDN( const char * what ) {
  for ( int i = 0 ; i < numMdnMessageBoxes ; ++i )
    if ( !qstrcmp( what, mdnMessageBoxes[i].dontAskAgainID ) ) {
      const KCursorSaver saver( Qt::ArrowCursor );
      MDNAdviceDialog::MDNAdvice answer;
      answer = MDNAdviceDialog::questionIgnoreSend( mdnMessageBoxes[i].text,
                                                    mdnMessageBoxes[i].canDeny );
      switch ( answer ) {
        case MDNAdviceDialog::MDNSend:
          return 3;

        case MDNAdviceDialog::MDNSendDenied:
          return 2;

        default:
        case MDNAdviceDialog::MDNIgnore:
          return 0;
      }
    }
  kWarning() <<"didn't find data for message box \""
                 << what << "\"";
  return 0;
}

KMMessage* KMMessage::createMDN( MDN::ActionMode a,
                                 MDN::DispositionType d,
                                 bool allowGUI,
                                 QList<MDN::DispositionModifier> m )
{
  // RFC 2298: At most one MDN may be issued on behalf of each
  // particular recipient by their user agent.  That is, once an MDN
  // has been issued on behalf of a recipient, no further MDNs may be
  // issued on behalf of that recipient, even if another disposition
  // is performed on the message.
//#define MDN_DEBUG 1
#ifndef MDN_DEBUG
  if ( mdnSentState() != KMMsgMDNStateUnknown &&
       mdnSentState() != KMMsgMDNNone )
    return 0;
#else
  char st[2]; st[0] = (char)mdnSentState(); st[1] = 0;
  kDebug() << "mdnSentState() == '" << st <<"'";
#endif

  // RFC 2298: An MDN MUST NOT be generated in response to an MDN.
  if ( findDwBodyPart( DwMime::kTypeMessage,
                       DwMime::kSubtypeDispositionNotification ) ) {
    setMDNSentState( KMMsgMDNIgnore );
    return 0;
  }

  // extract where to send to:
  QString receiptTo = headerField("Disposition-Notification-To");
  if ( receiptTo.trimmed().isEmpty() ) return 0;
  receiptTo.remove( '\n' );


  MDN::SendingMode s = MDN::SentAutomatically; // set to manual if asked user
  QString special; // fill in case of error, warning or failure
  KConfigGroup mdnConfig( KMKernel::config(), "MDN" );

  // default:
  int mode = mdnConfig.readEntry( "default-policy", 0 );
  if ( !mode || mode < 0 || mode > 3 ) {
    // early out for ignore:
    setMDNSentState( KMMsgMDNIgnore );
    return 0;
  }
  if ( mode == 1 /* ask */ && !allowGUI )
    return 0; // don't setMDNSentState here!

  // RFC 2298: An importance of "required" indicates that
  // interpretation of the parameter is necessary for proper
  // generation of an MDN in response to this request.  If a UA does
  // not understand the meaning of the parameter, it MUST NOT generate
  // an MDN with any disposition type other than "failed" in response
  // to the request.
  QString notificationOptions = headerField("Disposition-Notification-Options");
  if ( notificationOptions.contains( "required", Qt::CaseSensitive ) ) {
    // ### hacky; should parse...
    // There is a required option that we don't understand. We need to
    // ask the user what we should do:
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnUnknownOption" );
    s = MDN::SentManually;

    special = i18n("Header \"Disposition-Notification-Options\" contained "
                   "required, but unknown parameter");
    d = MDN::Failed;
    m.clear(); // clear modifiers
  }

  // RFC 2298: [ Confirmation from the user SHOULD be obtained (or no
  // MDN sent) ] if there is more than one distinct address in the
  // Disposition-Notification-To header.
  kDebug() << "KPIMUtils::splitAddressList(receiptTo):" // krazy:exclude=kdebug
           << KPIMUtils::splitAddressList(receiptTo).join("\n");
  if ( KPIMUtils::splitAddressList(receiptTo).count() > 1 ) {
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( "mdnMultipleAddressesInReceiptTo" );
    s = MDN::SentManually;
  }

  // RFC 2298: MDNs SHOULD NOT be sent automatically if the address in
  // the Disposition-Notification-To header differs from the address
  // in the Return-Path header. [...] Confirmation from the user
  // SHOULD be obtained (or no MDN sent) if there is no Return-Path
  // header in the message [...]
  AddrSpecList returnPathList = extractAddrSpecs("Return-Path");
  QString returnPath = returnPathList.isEmpty() ? QString()
    : returnPathList.front().localPart + '@' + returnPathList.front().domain ;
  kDebug() << "clean return path:" << returnPath;
  if ( returnPath.isEmpty() || !receiptTo.contains( returnPath, Qt::CaseSensitive ) ) {
    if ( !allowGUI ) return 0; // don't setMDNSentState here!
    mode = requestAdviceOnMDN( returnPath.isEmpty() ?
                               "mdnReturnPathEmpty" :
                               "mdnReturnPathNotInReceiptTo" );
    s = MDN::SentManually;
  }

  if ( true ) {
    if ( mode == 1 ) { // ask
      assert( allowGUI );
      mode = requestAdviceOnMDN( "mdnNormalAsk" );
      s = MDN::SentManually; // asked user
    }

    switch ( mode ) {
      case 0: // ignore:
        setMDNSentState( KMMsgMDNIgnore );
        return 0;
      default:
      case 1:
        kFatal() << "The \"ask\" mode should never appear here!";
        break;
      case 2: // deny
        d = MDN::Denied;
        m.clear();
        break;
      case 3:
        break;
    }
  }


  // extract where to send from:
  QString finalRecipient = kmkernel->identityManager()
    ->identityForUoidOrDefault( identityUoid() ).fullEmailAddr();

  //
  // Generate message:
  //

  KMMessage * receipt = new KMMessage();
  receipt->initFromMessage( this );
  receipt->removeHeaderField("Content-Type");
  receipt->removeHeaderField("Content-Transfer-Encoding");
  // Modify the ContentType directly (replaces setAutomaticFields(true))
  DwHeaders & header = receipt->mMsg->Headers();
  header.MimeVersion().FromString("1.0");
  DwMediaType & contentType = receipt->dwContentType();
  contentType.SetType( DwMime::kTypeMultipart );
  contentType.SetSubtype( DwMime::kSubtypeReport );
  contentType.CreateBoundary(0);
  receipt->mNeedsAssembly = true;
  receipt->setContentTypeParam( "report-type", "disposition-notification" );

  QString description = replaceHeadersInString( MDN::descriptionFor( d, m ) );

  // text/plain part:
  KMMessagePart firstMsgPart;
  firstMsgPart.setTypeStr( "text" );
  firstMsgPart.setSubtypeStr( "plain" );
  firstMsgPart.setBodyFromUnicode( description );
  receipt->addBodyPart( &firstMsgPart );

  // message/disposition-notification part:
  KMMessagePart secondMsgPart;
  secondMsgPart.setType( DwMime::kTypeMessage );
  secondMsgPart.setSubtype( DwMime::kSubtypeDispositionNotification );
  //secondMsgPart.setCharset( "us-ascii" );
  //secondMsgPart.setCteStr( "7bit" );
  secondMsgPart.setBodyEncoded( MDN::dispositionNotificationBodyContent(
                            finalRecipient,
                            rawHeaderField("Original-Recipient"),
                            id(), /* Message-ID */
                            d, a, s, m, special ) );
  receipt->addBodyPart( &secondMsgPart );

  // message/rfc822 or text/rfc822-headers body part:
  int num = mdnConfig.readEntry( "quote-message", 0 );
  if ( num < 0 || num > 2 ) num = 0;
  /* 0=> Nothing, 1=>Full Message, 2=>HeadersOnly*/

  KMMessagePart thirdMsgPart;
  switch ( num ) {
  case 1:
    thirdMsgPart.setTypeStr( "message" );
    thirdMsgPart.setSubtypeStr( "rfc822" );
    thirdMsgPart.setBody( asSendableString() );
    receipt->addBodyPart( &thirdMsgPart );
    break;
  case 2:
    thirdMsgPart.setTypeStr( "text" );
    thirdMsgPart.setSubtypeStr( "rfc822-headers" );
    thirdMsgPart.setBody( headerAsSendableString() );
    receipt->addBodyPart( &thirdMsgPart );
    break;
  case 0:
  default:
    break;
  };

  receipt->setTo( receiptTo );
  receipt->setSubject( "Message Disposition Notification" );
  receipt->setReplyToId( msgId() );
  receipt->setReferences( getRefStr() );

  receipt->cleanupHeader();

  kDebug() << "final message:" + receipt->asString();

  //
  // Set "MDN sent" status:
  //
  KMMsgMDNSentState state = KMMsgMDNStateUnknown;
  switch ( d ) {
  case MDN::Displayed:   state = KMMsgMDNDisplayed;  break;
  case MDN::Deleted:     state = KMMsgMDNDeleted;    break;
  case MDN::Dispatched:  state = KMMsgMDNDispatched; break;
  case MDN::Processed:   state = KMMsgMDNProcessed;  break;
  case MDN::Denied:      state = KMMsgMDNDenied;     break;
  case MDN::Failed:      state = KMMsgMDNFailed;     break;
  };
  setMDNSentState( state );

  return receipt;
}

QString KMMessage::replaceHeadersInString( const QString & s ) const {
    QString result = s;
    QRegExp rx( "\\$\\{([a-z0-9-]+)\\}", Qt::CaseInsensitive );
    Q_ASSERT( rx.isValid() );

    QRegExp rxDate( "\\$\\{date\\}" );
    Q_ASSERT( rxDate.isValid() );

    QString sDate = KMime::DateFormatter::formatDate(
        KMime::DateFormatter::Localized, date() );

    int idx = 0;
    if( ( idx = rxDate.indexIn( result, idx ) ) != -1  ) {
      result.replace( idx, rxDate.matchedLength(), sDate );
    }

    idx = 0;
    while ( ( idx = rx.indexIn( result, idx ) ) != -1 ) {
      QString replacement = headerField( rx.cap(1).toLatin1() );
      result.replace( idx, rx.matchedLength(), replacement );
      idx += replacement.length();
    }
    return result;
}


KMMessage* KMMessage::createDeliveryReceipt() const
{
  QString str, receiptTo;
  KMMessage *receipt;

  receiptTo = headerField("Disposition-Notification-To");
  if ( receiptTo.trimmed().isEmpty() ) return 0;
  receiptTo.remove( '\n' );

  receipt = new KMMessage;
  receipt->initFromMessage(this);
  receipt->setTo(receiptTo);
  receipt->setSubject(i18n("Receipt: ") + subject());

  str  = "Your message was successfully delivered.";
  str += "\n\n---------- Message header follows ----------\n";
  str += headerAsString();
  str += "--------------------------------------------\n";
  // Conversion to toLatin1 is correct here as Mail headers should contain
  // ascii only
  receipt->setBody(str.toLatin1());
  receipt->setAutomaticFields();

  return receipt;
}


void KMMessage::applyIdentity( uint id )
{
  const KPIMIdentities::Identity & ident =
    kmkernel->identityManager()->identityForUoidOrDefault( id );

  if(ident.fullEmailAddr().isEmpty())
    setFrom("");
  else
    setFrom(ident.fullEmailAddr());

  if(ident.replyToAddr().isEmpty())
    setReplyTo("");
  else
    setReplyTo(ident.replyToAddr());

  if(ident.bcc().isEmpty())
    setBcc("");
  else
    setBcc(ident.bcc());

  if (ident.organization().isEmpty())
    removeHeaderField("Organization");
  else
    setHeaderField("Organization", ident.organization());

  if (ident.isDefault())
    removeHeaderField("X-KMail-Identity");
  else
    setHeaderField("X-KMail-Identity", QString::number( ident.uoid() ));

  if (ident.transport().isEmpty())
    removeHeaderField("X-KMail-Transport");
  else
    setHeaderField("X-KMail-Transport", ident.transport());

  setFcc( QString() );
  setDrafts( QString() );
  setTemplates( QString() );

  /* KPIMIdentities::Identity::fcc(), KPIMIdentities::Identity::drafts() and KPIMIdentities::Identity::templates()
     using akonadi, so read values from config file directly */
  const KConfig config( "emailidentities" );
  const QStringList identities = config.groupList().filter( QRegExp( "^Identity #\\d+$" ) );
  for ( QStringList::const_iterator group = identities.constBegin(); group != identities.constEnd(); ++group ) {
    const KConfigGroup configGroup( &config, *group );
    if ( configGroup.readEntry( "uoid", 0U ) == ident.uoid() ) {
      setFcc( configGroup.readEntry( "Fcc2", QString() ) );
      setDrafts( configGroup.readEntry( "Drafts2", QString() ) );
      setTemplates( configGroup.readEntry( "Templates2", QString() ) );
      break;
    }
  }

}

//-----------------------------------------------------------------------------
void KMMessage::initHeader( uint id )
{
  applyIdentity( id );
  setTo("");
  setSubject("");
  setDateToday();

  // user agent, e.g. KMail/1.9.50 (Windows/5.0; KDE/3.97.1; i686; svn-762186; 2008-01-15)
  QStringList extraInfo;
# if defined KMAIL_SVN_REVISION_STRING && defined KMAIL_SVN_LAST_CHANGE
  extraInfo << KMAIL_SVN_REVISION_STRING << KMAIL_SVN_LAST_CHANGE;
#else
#error forgot to include version-kmail.h
# endif
  setHeaderField("User-Agent",
    KProtocolManager::userAgentForApplication( "KMail", KMAIL_VERSION, extraInfo )
  );

  // This will allow to change Content-Type:
  setHeaderField("Content-Type","text/plain");
}

uint KMMessage::identityUoid() const {
  QString idString = headerField("X-KMail-Identity").trimmed();
  bool ok = false;
  int id = idString.toUInt( &ok );

  if ( !ok || id == 0 )
    id = kmkernel->identityManager()->identityForAddress( to() + ", " + cc() ).uoid();
  if ( id == 0 && parent() )
    id = parent()->identity();

  return id;
}


//-----------------------------------------------------------------------------
void KMMessage::initFromMessage(const KMMessage *msg, bool idHeaders)
{
  uint id = msg->identityUoid();

  if ( idHeaders ) initHeader(id);
  else setHeaderField("X-KMail-Identity", QString::number(id));
  if (!msg->headerField("X-KMail-Transport").isEmpty())
    setHeaderField("X-KMail-Transport", msg->headerField("X-KMail-Transport"));
}


//-----------------------------------------------------------------------------
void KMMessage::cleanupHeader()
{
  DwHeaders& header = mMsg->Headers();
  DwField* field = header.FirstField();
  DwField* nextField;

  if (mNeedsAssembly) mMsg->Assemble();
  mNeedsAssembly = false;

  while (field)
  {
    nextField = field->Next();
    if (field->FieldBody()->AsString().empty())
    {
      header.RemoveField(field);
      mNeedsAssembly = true;
    }
    field = nextField;
  }
}


//-----------------------------------------------------------------------------
void KMMessage::setAutomaticFields(bool aIsMulti)
{
  DwHeaders& header = mMsg->Headers();
  header.MimeVersion().FromString("1.0");

  if (aIsMulti || numBodyParts() > 1)
  {
    // Set the type to 'Multipart' and the subtype to 'Mixed'
    DwMediaType& contentType = dwContentType();
    contentType.SetType(   DwMime::kTypeMultipart);
    contentType.SetSubtype(DwMime::kSubtypeMixed );

    // Create a random printable string and set it as the boundary parameter
    contentType.CreateBoundary(0);
  }
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
QString KMMessage::dateStr() const
{
  KConfigGroup general( KMKernel::config(), "General" );
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  //kDebug()<<"####  Date ="<<header.Date().AsString().c_str();

  return KMime::DateFormatter::formatDate(
      static_cast<KMime::DateFormatter::FormatType>(general.readEntry( "dateFormat",
          int( KMime::DateFormatter::Fancy ) )),
      unixTime, general.readEntry( "customDateFormat" ));
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::dateShortStr() const
{
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  QByteArray result = ctime(&unixTime);

  if (result[result.length()-1]=='\n')
    result.truncate(result.length()-1);

  return result;
}


//-----------------------------------------------------------------------------
QString KMMessage::dateIsoStr() const
{
  DwHeaders& header = mMsg->Headers();
  time_t unixTime;

  if (!header.HasDate()) return "";
  unixTime = header.Date().AsUnixTime();

  char cstr[64];
  strftime(cstr, 63, "%Y-%m-%d %H:%M:%S", localtime(&unixTime));
  return QString(cstr);
}


//-----------------------------------------------------------------------------
time_t KMMessage::date() const
{
  time_t res = ( time_t )-1;
  DwHeaders& header = mMsg->Headers();
  if (header.HasDate())
    res = header.Date().AsUnixTime();
  return res;
}


//-----------------------------------------------------------------------------
void KMMessage::setDateToday()
{
  struct timeval tval;
  gettimeofday(&tval, 0);
  setDate((time_t)tval.tv_sec);
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(time_t aDate)
{
  mDate = aDate;
  mMsg->Headers().Date().FromCalendarTime(aDate);
  mMsg->Headers().Date().Assemble();
  mNeedsAssembly = true;
  mDirty = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setDate(const QByteArray& aStr)
{
  DwHeaders& header = mMsg->Headers();

  header.Date().FromString(aStr);
  header.Date().Parse();
  mNeedsAssembly = true;
  mDirty = true;

  if (header.HasDate())
    mDate = header.Date().AsUnixTime();
}


//-----------------------------------------------------------------------------
QString KMMessage::to() const
{
  // handle To same as Cc below, bug 80747
  QList<QByteArray> rawHeaders = rawHeaderFields( "To" );
  QStringList headers;
  for ( QList<QByteArray>::Iterator it = rawHeaders.begin(); it != rawHeaders.end(); ++it ) {
    headers << QString::fromUtf8(*it);
  }
  return KPIMUtils::normalizeAddressesAndDecodeIdn( headers.join( ", " ) );
}


//-----------------------------------------------------------------------------
void KMMessage::setTo(const QString& aStr)
{
  setHeaderField( "To", aStr, Address );
}

//-----------------------------------------------------------------------------
QString KMMessage::toStrip() const
{
  return StringUtil::stripEmailAddr( to() );
}

//-----------------------------------------------------------------------------
QString KMMessage::replyTo() const
{
  return KPIMUtils::normalizeAddressesAndDecodeIdn( QString::fromUtf8(rawHeaderField("Reply-To")) );
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(const QString& aStr)
{
  setHeaderField( "Reply-To", aStr, Address );
}


//-----------------------------------------------------------------------------
void KMMessage::setReplyTo(KMMessage* aMsg)
{
  setHeaderField( "Reply-To", aMsg->from(), Address );
}


//-----------------------------------------------------------------------------
QString KMMessage::cc() const
{
  // get the combined contents of all Cc headers (as workaround for invalid
  // messages with multiple Cc headers)
  QList<QByteArray> rawHeaders = rawHeaderFields( "Cc" );
  QStringList headers;
  for ( QList<QByteArray>::Iterator it = rawHeaders.begin(); it != rawHeaders.end(); ++it ) {
    headers << QString::fromUtf8(*it);
  }
  return KPIMUtils::normalizeAddressesAndDecodeIdn( headers.join( ", " ) );
}


//-----------------------------------------------------------------------------
void KMMessage::setCc(const QString& aStr)
{
  setHeaderField( "Cc", aStr, Address );
}


//-----------------------------------------------------------------------------
QString KMMessage::ccStrip() const
{
  return StringUtil::stripEmailAddr( cc() );
}


//-----------------------------------------------------------------------------
QString KMMessage::bcc() const
{
  return KPIMUtils::normalizeAddressesAndDecodeIdn( QString::fromUtf8(rawHeaderField("Bcc")) );
}


//-----------------------------------------------------------------------------
void KMMessage::setBcc(const QString& aStr)
{
  setHeaderField( "Bcc", aStr, Address );
}

//-----------------------------------------------------------------------------
QString KMMessage::fcc() const
{
  return headerField( "X-KMail-Fcc" );
}


//-----------------------------------------------------------------------------
void KMMessage::setFcc(const QString& aStr)
{
  setHeaderField( "X-KMail-Fcc", aStr );
}

//-----------------------------------------------------------------------------
void KMMessage::setDrafts(const QString& aStr)
{
  mDrafts = aStr;
}

//-----------------------------------------------------------------------------
void KMMessage::setTemplates(const QString& aStr)
{
  mTemplates = aStr;
}

//-----------------------------------------------------------------------------
QString KMMessage::who() const
{
  if (mParent)
    return KPIMUtils::normalizeAddressesAndDecodeIdn( QString::fromUtf8(rawHeaderField(mParent->whoField().toUtf8())) );
  return from();
}


//-----------------------------------------------------------------------------
QString KMMessage::from() const
{
  return KPIMUtils::normalizeAddressesAndDecodeIdn( QString::fromUtf8(rawHeaderField("From")) );
}


//-----------------------------------------------------------------------------
void KMMessage::setFrom(const QString& bStr)
{
  QString aStr = bStr;
  if (aStr.isNull())
    aStr = "";
  setHeaderField( "From", aStr, Address );
  mDirty = true;
}


//-----------------------------------------------------------------------------
QString KMMessage::fromStrip() const
{
  return StringUtil::stripEmailAddr( from() );
}

//-----------------------------------------------------------------------------
QString KMMessage::sender() const {
  AddrSpecList asl = extractAddrSpecs( "Sender" );
  if ( asl.empty() )
    asl = extractAddrSpecs( "From" );
  if ( asl.empty() )
    return QString();
  return asl.front().asString();
}

//-----------------------------------------------------------------------------
QString KMMessage::subject() const
{
  return headerField("Subject");
}


//-----------------------------------------------------------------------------
void KMMessage::setSubject(const QString& aStr)
{
  setHeaderField("Subject",aStr);
  mDirty = true;
}

//Reimplement virtuals from KMMsgBase
//-----------------------------------------------------------------------------
//Different from KMMsgInfo as that reads from the index
QString KMMessage::tagString() const
{
  if ( mTagList )
    return mTagList->join( "," );
  return QString();
}

KMMessageTagList *KMMessage::tagList() const { return mTagList; }

//-----------------------------------------------------------------------------
QString KMMessage::xmark() const
{
  return headerField("X-KMail-Mark");
}


//-----------------------------------------------------------------------------
void KMMessage::setXMark(const QString& aStr)
{
  setHeaderField("X-KMail-Mark", aStr);
  mDirty = true;
}


//-----------------------------------------------------------------------------
QString KMMessage::replyToId() const
{
  int leftAngle, rightAngle;
  QString replyTo, references;

  replyTo = headerField("In-Reply-To");
  // search the end of the (first) message id in the In-Reply-To header
  rightAngle = replyTo.indexOf( '>' );
  if (rightAngle != -1)
    replyTo.truncate( rightAngle + 1 );
  // now search the start of the message id
  leftAngle = replyTo.lastIndexOf( '<' );
  if (leftAngle != -1)
    replyTo = replyTo.mid( leftAngle );

  // if we have found a good message id we can return immediately
  // We ignore mangled In-Reply-To headers which are created by a
  // misconfigured Mutt. They look like this <"from foo"@bar.baz>, i.e.
  // they contain double quotes and spaces. We only check for '"'.
  if (!replyTo.isEmpty() && (replyTo[0] == '<') &&
      ( !replyTo.contains( '"' ) ) )
    return replyTo;

  references = headerField("References");
  leftAngle = references.lastIndexOf( '<' );
  if (leftAngle != -1)
    references = references.mid( leftAngle );
  rightAngle = references.indexOf( '>' );
  if (rightAngle != -1)
    references.truncate( rightAngle + 1 );

  // if we found a good message id in the References header return it
  if (!references.isEmpty() && references[0] == '<')
    return references;
  // else return the broken message id we found in the In-Reply-To header
  else
    return replyTo;
}


//-----------------------------------------------------------------------------
QString KMMessage::replyToIdMD5() const {
  return base64EncodedMD5( replyToId() );
}

//-----------------------------------------------------------------------------
QString KMMessage::references() const
{
  int leftAngle, rightAngle;
  QString references = headerField( "References" );

  // keep the last two entries for threading
  leftAngle = references.lastIndexOf( '<' );
  leftAngle = references.lastIndexOf( '<', leftAngle - 1 );
  if( leftAngle != -1 )
    references = references.mid( leftAngle );
  rightAngle = references.lastIndexOf( '>' );
  if( rightAngle != -1 )
    references.truncate( rightAngle + 1 );

  if( !references.isEmpty() && references[0] == '<' )
    return references;
  else
    return QString();
}

//-----------------------------------------------------------------------------
QString KMMessage::replyToAuxIdMD5() const
{
  QString result = references();
  // references contains two items, use the first one
  // (the second to last reference)
  const int rightAngle = result.indexOf( '>' );
  if( rightAngle != -1 )
    result.truncate( rightAngle + 1 );

  return base64EncodedMD5( result );
}

//-----------------------------------------------------------------------------
QString KMMessage::strippedSubjectMD5() const {
  return base64EncodedMD5( stripOffPrefixes( subject() ), true /*utf8*/ );
}

//-----------------------------------------------------------------------------
QString KMMessage::subjectMD5() const {
  return base64EncodedMD5( subject(), true /*utf8*/ );
}

//-----------------------------------------------------------------------------
bool KMMessage::subjectIsPrefixed() const {
  return subjectMD5() != strippedSubjectMD5();
}

//-----------------------------------------------------------------------------
void KMMessage::setReplyToId(const QString& aStr)
{
  setHeaderField("In-Reply-To", aStr);
  mDirty = true;
}


//-----------------------------------------------------------------------------
QString KMMessage::msgId() const
{
  QString msgId = headerField("Message-Id");

  // search the end of the message id
  const int rightAngle = msgId.indexOf( '>' );
  if (rightAngle != -1)
    msgId.truncate( rightAngle + 1 );
  // now search the start of the message id
  const int leftAngle = msgId.lastIndexOf( '<' );
  if (leftAngle != -1)
    msgId = msgId.mid( leftAngle );
  return msgId;
}


//-----------------------------------------------------------------------------
QString KMMessage::msgIdMD5() const {
  return base64EncodedMD5( msgId() );
}


//-----------------------------------------------------------------------------
void KMMessage::setMsgId(const QString& aStr)
{
  setHeaderField("Message-Id", aStr);
  mDirty = true;
}

//-----------------------------------------------------------------------------
size_t KMMessage::msgSizeServer() const {
  return headerField( "X-Length" ).toULong();
}


//-----------------------------------------------------------------------------
void KMMessage::setMsgSizeServer(size_t size)
{
  setHeaderField("X-Length", QByteArray::number((qlonglong)size));
  mDirty = true;
}

//-----------------------------------------------------------------------------
ulong KMMessage::UID() const {
  return headerField( "X-UID", NoEncoding ).toULong();
}


//-----------------------------------------------------------------------------
void KMMessage::setUID(ulong uid)
{
  setHeaderField( "X-UID", QByteArray::number((qlonglong)uid), Unstructured, false, NoEncoding );
  mDirty = true;
}

//-----------------------------------------------------------------------------
AddressList KMMessage::headerAddrField( const QByteArray & aName ) const {
  return StringUtil::splitAddrField( rawHeaderField( aName ) );
}

AddrSpecList KMMessage::extractAddrSpecs( const QByteArray & header ) const {
  AddressList al = headerAddrField( header );
  AddrSpecList result;
  for ( AddressList::const_iterator ait = al.constBegin() ; ait != al.constEnd() ; ++ait )
    for ( MailboxList::const_iterator mit = (*ait).mailboxList.constBegin() ; mit != (*ait).mailboxList.constEnd() ; ++mit )
      result.push_back( (*mit).addrSpec() );
  return result;
}

QByteArray KMMessage::rawHeaderField( const QByteArray &name ) const
{
  if ( !name.isEmpty() && mMsg ) {
    DwHeaders &header = mMsg->Headers();
    DwField *field = header.FindField( name.constData() );

    if ( !field ) {
      return QByteArray();
    }

    return header.FieldBody( name.data() ).AsString().c_str();
  }
  return QByteArray();
}

QList<QByteArray> KMMessage::rawHeaderFields( const QByteArray& field ) const
{
  if ( field.isEmpty() || !mMsg->Headers().FindField( field ) )
    return QList<QByteArray>();

  std::vector<DwFieldBody*> v = mMsg->Headers().AllFieldBodies( field.data() );
  QList<QByteArray> headerFields;
  for ( uint i = 0; i < v.size(); ++i ) {
    headerFields.append( v[i]->AsString().c_str() );
  }

  return headerFields;
}

QString KMMessage::headerField( const QByteArray& aName, EncodingMode encodingMode ) const
{
  if ( aName.isEmpty() ) {
    return QString();
  }

  if ( !mMsg || !mMsg->hasHeaders() || !mMsg->Headers().FindField( aName ) ) {
    return QString();
  }

  const char *fieldValue = mMsg->Headers().FieldBody( aName.data() ).AsString().c_str();
  if ( encodingMode == NoEncoding ) {
    return QString( fieldValue );
  }
  else {
    return decodeRFC2047String( fieldValue, charset() );
  }
}

QStringList KMMessage::headerFields( const QByteArray& field ) const
{
  if ( field.isEmpty() || !mMsg->hasHeaders() || !mMsg->Headers().FindField( field ) )
    return QStringList();

  std::vector<DwFieldBody*> v = mMsg->Headers().AllFieldBodies( field.data() );
  QStringList headerFields;
  for ( uint i = 0; i < v.size(); ++i ) {
    headerFields.append( decodeRFC2047String( v[i]->AsString().c_str(), charset() ) );
  }

  return headerFields;
}

//-----------------------------------------------------------------------------
void KMMessage::removeHeaderField(const QByteArray& aName)
{
  DwHeaders & header = mMsg->Headers();
  DwField * field = header.FindField(aName);
  if (!field) return;

  header.RemoveField(field);
  mNeedsAssembly = true;
}

//-----------------------------------------------------------------------------
void KMMessage::removeHeaderFields(const QByteArray& aName)
{
  DwHeaders & header = mMsg->Headers();
  while ( DwField * field = header.FindField(aName) ) {
    header.RemoveField(field);
    mNeedsAssembly = true;
  }
}


//-----------------------------------------------------------------------------
void KMMessage::setHeaderField( const QByteArray& aName, const QString& bValue,
                                HeaderFieldType type, bool prepend, EncodingMode encodingMode )
{
#if 0
  if ( type != Unstructured )
    kDebug() << "( \"" << aName <<"\", \"" << bValue << "\"," << type << ")";
#endif
  if (aName.isEmpty()) return;

  DwHeaders& header = mMsg->Headers();

  DwString str;
  DwField* field;
  QByteArray aValue;
  if (!bValue.isEmpty())
  {
    QString value = bValue;
    if ( type == Address )
      value = KPIMUtils::normalizeAddressesAndEncodeIdn( value );
#if 0
    if ( type != Unstructured )
      kDebug() << "value: \"" << value <<"\"";
#endif
    if ( encodingMode == NoEncoding ) {
      aValue = value.toAscii();
      Q_ASSERT( QString::fromAscii( aValue ) == bValue );
    }
    else {
      QByteArray encoding = autoDetectCharset( charset(), s->prefCharsets, value );
      if (encoding.isEmpty())
        encoding = "utf-8";
      aValue = encodeRFC2047String( value, encoding );
    }
#if 0
    if ( type != Unstructured )
      kDebug() << "aValue: \"" << aValue <<"\"";
#endif
  }
  // FIXME PORTING
  str = (const char*)aName;
  if (str[str.length()-1] != ':') str += ": ";
  else str += ' ';
  if ( !aValue.isEmpty() )
    str += (const char*)aValue;
  if (str[str.length()-1] != '\n') str += '\n';

  field = new DwField(str, mMsg);
  field->Parse();

  if ( prepend )
    header.AddFieldAt( 1, field );
  else
    header.AddOrReplaceField( field );
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::typeStr() const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().TypeStr().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::type( DwEntity *entity ) const
{
  if ( !entity )
    entity = mMsg;
  DwHeaders& header = entity->Headers();
  if ( header.HasContentType() )
    return header.ContentType().Type();
  else return DwMime::kTypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setTypeStr(const QByteArray& aStr)
{
  dwContentType().SetTypeStr(DwString(aStr));
  dwContentType().Parse();
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setType(int aType)
{
  dwContentType().SetType(aType);
  dwContentType().Assemble();
  mNeedsAssembly = true;
}



//-----------------------------------------------------------------------------
QByteArray KMMessage::subtypeStr() const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().SubtypeStr().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::subtype() const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentType()) return header.ContentType().Subtype();
  else return DwMime::kSubtypeNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtypeStr(const QByteArray& aStr)
{
  dwContentType().SetSubtypeStr(DwString(aStr));
  dwContentType().Parse();
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setSubtype(int aSubtype)
{
  dwContentType().SetSubtype(aSubtype);
  dwContentType().Assemble();
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setDwMediaTypeParam( DwMediaType &mType,
                                     const QByteArray& attr,
                                     const QByteArray& val )
{
  mType.Parse();
  DwParameter *param = mType.FirstParameter();
  while(param) {
    if (!kasciistricmp(param->Attribute().c_str(), attr))
      break;
    else
      param = param->Next();
  }
  if (!param){
    param = new DwParameter;
    param->SetAttribute(DwString( attr ));
    mType.AddParameter( param );
  }
  else
    mType.SetModified();
  param->SetValue(DwString( val ));
  mType.Assemble();
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTypeParam(const QByteArray& attr, const QByteArray& val)
{
  if (mNeedsAssembly) mMsg->Assemble();
  mNeedsAssembly = false;
  setDwMediaTypeParam( dwContentType(), attr, val );
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::contentTransferEncodingStr() const
{
  DwHeaders& header = mMsg->Headers();
  if (header.HasContentTransferEncoding())
    return header.ContentTransferEncoding().AsString().c_str();
  else return "";
}


//-----------------------------------------------------------------------------
int KMMessage::contentTransferEncoding( DwEntity *entity ) const
{
  if ( !entity )
    entity = mMsg;

  DwHeaders& header = entity->Headers();
  if ( header.HasContentTransferEncoding() )
    return header.ContentTransferEncoding().AsEnum();
  else return DwMime::kCteNull;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncodingStr( const QByteArray& cteString,
                                               DwEntity *entity )
{
  if ( !entity )
    entity = mMsg;

  entity->Headers().ContentTransferEncoding().FromString( cteString );
  entity->Headers().ContentTransferEncoding().Parse();
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setContentTransferEncoding( int cte, DwEntity *entity )
{
  if ( !entity )
    entity = mMsg;

  entity->Headers().ContentTransferEncoding().FromEnum( cte );
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
DwHeaders& KMMessage::headers() const
{
  return mMsg->Headers();
}


//-----------------------------------------------------------------------------
void KMMessage::setNeedsAssembly()
{
  mNeedsAssembly = true;
}

//-----------------------------------------------------------------------------
void KMMessage::assembleIfNeeded()
{
  Q_ASSERT( mMsg );

  if ( mNeedsAssembly ) {
    mMsg->Assemble();
    mNeedsAssembly = false;
  }
}

//-----------------------------------------------------------------------------
QByteArray KMMessage::body() const
{
  DwString body = mMsg->Body().AsString();
  QByteArray str = body.c_str();
  if ( str.length() != static_cast<int>( body.length() ) ) {
    kWarning() << "Body is binary but used as text!";
  }
  return str;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::bodyDecodedBinary() const
{
  DwString dwstr;
  DwString dwsrc = mMsg->Body().AsString();

  switch (cte())
  {
  case DwMime::kCteBase64:
    DwDecodeBase64(dwsrc, dwstr);
    break;
  case DwMime::kCteQuotedPrintable:
    DwDecodeQuotedPrintable(dwsrc, dwstr);
    break;
  default:
    dwstr = dwsrc;
    break;
  }

  int len = dwstr.size();
  QByteArray ba(len,'\0');
  memcpy(ba.data(),dwstr.data(),len);
  return ba;
}


//-----------------------------------------------------------------------------
QByteArray KMMessage::bodyDecoded() const
{
  DwString dwstr;
  DwString dwsrc = mMsg->Body().AsString();

  switch (cte())
  {
  case DwMime::kCteBase64:
    DwDecodeBase64(dwsrc, dwstr);
    break;
  case DwMime::kCteQuotedPrintable:
    DwDecodeQuotedPrintable(dwsrc, dwstr);
    break;
  default:
    dwstr = dwsrc;
    break;
  }

  QByteArray result = Util::ByteArray( dwstr );
  return result;
}

//-----------------------------------------------------------------------------
void KMMessage::setBodyAndGuessCte( const QByteArray& aBuf,
                                    QList<int> & allowedCte,
                                    bool allow8Bit,
                                    bool willBeSigned,
                                    DwEntity *entity )
{
  if ( !entity )
    entity = mMsg;

  CharFreq cf( aBuf ); // it's safe to pass null arrays
  allowedCte = StringUtil::determineAllowedCtes( cf, allow8Bit, willBeSigned );
  setCte( allowedCte[0], entity ); // choose best fitting
  setBodyEncodedBinary( aBuf, entity );
}

//-----------------------------------------------------------------------------
void KMMessage::setBodyEncoded(const QByteArray& aStr)
{
  // Qt4: QCString and QByteArray have been merged; this method can be cleaned up
  setBodyEncodedBinary( aStr );
}

//-----------------------------------------------------------------------------
void KMMessage::setBodyEncodedBinary( const QByteArray& bodyStr, DwEntity *entity )
{
  if ( !entity )
    entity = mMsg;

  DwString dwSrc = Util::dwString( bodyStr );
  DwString dwResult;

  switch ( cte( entity ) )
  {
  case DwMime::kCteBase64:
    DwEncodeBase64( dwSrc, dwResult );
    break;
  case DwMime::kCteQuotedPrintable:
    DwEncodeQuotedPrintable( dwSrc, dwResult );
    break;
  default:
    dwResult = dwSrc;
    break;
  }

  entity->Body().FromString( dwResult );
  entity->Body().Parse();

  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::setBody(const QByteArray& aStr)
{
  mMsg->Body().FromString(aStr.data());
  mNeedsAssembly = true;
}

//-----------------------------------------------------------------------------
void KMMessage::setMultiPartBody( const QByteArray & aStr ) {
  setBody( aStr );
  mMsg->Body().Parse();
  mNeedsAssembly = true;
}


// Patched by Daniel Moisset <dmoisset@grulic.org.ar>
// modified numbodyparts, bodypart to take nested body parts as
// a linear sequence.
// third revision, Sep 26 2000

// this is support structure for traversing tree without recursion

//-----------------------------------------------------------------------------
int KMMessage::numBodyParts() const
{
  int count = 0;
  DwBodyPart* part = getFirstDwBodyPart();
  QList< DwBodyPart* > parts;

  while (part)
  {
    //dive into multipart messages
    while (    part
            && part->hasHeaders()
            && part->Headers().HasContentType()
            && part->Body().FirstBodyPart()
            && (DwMime::kTypeMultipart == part->Headers().ContentType().Type()) )
    {
      parts.append( part );
      part = part->Body().FirstBodyPart();
    }
    // this is where currPart->msgPart contains a leaf message part
    count++;
    // go up in the tree until reaching a node with next
    // (or the last top-level node)
    while (part && !(part->Next()) && !(parts.isEmpty()))
    {
      part = parts.last();
      parts.removeLast();
    }

    if (part && part->Body().Message() &&
        part->Body().Message()->Body().FirstBodyPart())
    {
      part = part->Body().Message()->Body().FirstBodyPart();
    } else if (part) {
      part = part->Next();
    }
  }

  return count;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::getFirstDwBodyPart() const
{
  return mMsg->Body().FirstBodyPart();
}


//-----------------------------------------------------------------------------
int KMMessage::partNumber( DwBodyPart * aDwBodyPart ) const
{
  DwBodyPart *curpart;
  QList< DwBodyPart* > parts;
  int curIdx = 0;
  int idx = 0;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();

  while (curpart && !idx) {
    //dive into multipart messages
    while(    curpart
           && curpart->hasHeaders()
           && curpart->Headers().HasContentType()
           && curpart->Body().FirstBodyPart()
           && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) )
    {
      parts.append( curpart );
      curpart = curpart->Body().FirstBodyPart();
    }
    // this is where currPart->msgPart contains a leaf message part
    if (curpart == aDwBodyPart)
      idx = curIdx;
    curIdx++;
    // go up in the tree until reaching a node with next
    // (or the last top-level node)
    while (curpart && !(curpart->Next()) && !(parts.isEmpty()))
    {
      curpart = parts.last();
      parts.removeLast();
    } ;
    if (curpart)
      curpart = curpart->Next();
  }
  return idx;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::dwBodyPart( int aIdx ) const
{
  DwBodyPart *part, *curpart;
  QList< DwBodyPart* > parts;
  int curIdx = 0;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();
  part = 0;

  while (curpart && !part) {
    //dive into multipart messages
    while(    curpart
           && curpart->hasHeaders()
           && curpart->Headers().HasContentType()
           && curpart->Body().FirstBodyPart()
           && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) )
    {
      parts.append( curpart );
      curpart = curpart->Body().FirstBodyPart();
    }
    // this is where currPart->msgPart contains a leaf message part
    if (curIdx==aIdx)
        part = curpart;
    curIdx++;
    // go up in the tree until reaching a node with next
    // (or the last top-level node)
    while (curpart && !(curpart->Next()) && !(parts.isEmpty()))
    {
      curpart = parts.last();
      parts.removeLast();
    }
    if (curpart)
      curpart = curpart->Next();
  }
  return part;
}


//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::findDwBodyPart( int type, int subtype ) const
{
  DwBodyPart *part, *curpart;
  QList< DwBodyPart* > parts;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();
  part = 0;

  while (curpart && !part) {
    //dive into multipart messages
    while(curpart
          && curpart->hasHeaders()
          && curpart->Headers().HasContentType()
          && curpart->Body().FirstBodyPart()
          && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) ) {
        parts.append( curpart );
        curpart = curpart->Body().FirstBodyPart();
    }
    // this is where curPart->msgPart contains a leaf message part

    // pending(khz): Find out WHY this look does not travel down *into* an
    //               embedded "Message/RfF822" message containing a "Multipart/Mixed"
    if ( curpart && curpart->hasHeaders() && curpart->Headers().HasContentType() ) {
      kDebug() << curpart->Headers().ContentType().TypeStr().c_str()
                   << " " << curpart->Headers().ContentType().SubtypeStr().c_str();
    }

    if (curpart &&
        curpart->hasHeaders() &&
        curpart->Headers().HasContentType() &&
        curpart->Headers().ContentType().Type() == type &&
        curpart->Headers().ContentType().Subtype() == subtype) {
        part = curpart;
    } else {
      // go up in the tree until reaching a node with next
      // (or the last top-level node)
      while (curpart && !(curpart->Next()) && !(parts.isEmpty())) {
        curpart = parts.last();
        parts.removeLast();
      } ;
      if (curpart)
        curpart = curpart->Next();
    }
  }
  return part;
}

//-----------------------------------------------------------------------------
DwBodyPart * KMMessage::findDwBodyPart( const QByteArray& type, const QByteArray&  subtype ) const
{
  DwBodyPart *part, *curpart;
  QList< DwBodyPart* > parts;
  // Get the DwBodyPart for this index

  curpart = getFirstDwBodyPart();
  part = 0;

  while (curpart && !part) {
    //dive into multipart messages
    while(curpart
          && curpart->hasHeaders()
          && curpart->Headers().HasContentType()
          && curpart->Body().FirstBodyPart()
          && (DwMime::kTypeMultipart == curpart->Headers().ContentType().Type()) ) {
      parts.append( curpart );
      curpart = curpart->Body().FirstBodyPart();
    }
    // this is where curPart->msgPart contains a leaf message part

    // pending(khz): Find out WHY this look does not travel down *into* an
    //               embedded "Message/RfF822" message containing a "Multipart/Mixed"
    if (curpart && curpart->hasHeaders() && curpart->Headers().HasContentType() ) {
      kDebug() << curpart->Headers().ContentType().TypeStr().c_str()
                   << " " << curpart->Headers().ContentType().SubtypeStr().c_str();
    }

    if (curpart &&
        curpart->hasHeaders() &&
        curpart->Headers().HasContentType() &&
        curpart->Headers().ContentType().TypeStr().c_str() == type &&
        curpart->Headers().ContentType().SubtypeStr().c_str() == subtype) {
        part = curpart;
    } else {
      // go up in the tree until reaching a node with next
      // (or the last top-level node)
      while (curpart && !(curpart->Next()) && !(parts.isEmpty())) {
        curpart = parts.last();
        parts.removeLast();
      } ;
      if (curpart)
        curpart = curpart->Next();
    }
  }
  return part;
}


void applyHeadersToMessagePart( DwHeaders& headers, KMMessagePart* aPart )
{
  // Content-type
  QByteArray additionalCTypeParams;
  if (headers.HasContentType())
  {
    DwMediaType& ct = headers.ContentType();
    aPart->setOriginalContentTypeStr( ct.AsString().c_str() );
    aPart->setTypeStr(ct.TypeStr().c_str());
    aPart->setSubtypeStr(ct.SubtypeStr().c_str());
    DwParameter *param = ct.FirstParameter();
    while(param)
    {
      if (!qstricmp(param->Attribute().c_str(), "charset"))
        aPart->setCharset(QByteArray(param->Value().c_str()).toLower());
      else if (!qstrnicmp(param->Attribute().c_str(), "name*", 5))
        aPart->setName(KMMsgBase::decodeRFC2231String(
              param->Value().c_str()));
      else {
        additionalCTypeParams += ';';
        additionalCTypeParams += param->AsString().c_str();
      }
      param=param->Next();
    }
  }
  else
  {
    aPart->setTypeStr("text");      // Set to defaults
    aPart->setSubtypeStr("plain");
  }
  aPart->setAdditionalCTypeParamStr( additionalCTypeParams );
  // Modification by Markus
  if (aPart->name().isEmpty())
  {
    if (headers.HasContentType() && !headers.ContentType().Name().empty()) {
      aPart->setName(KMMsgBase::decodeRFC2047String(headers.
            ContentType().Name().c_str()) );
    } else if (headers.HasSubject() && !headers.Subject().AsString().empty()) {
      aPart->setName( KMMsgBase::decodeRFC2047String(headers.
            Subject().AsString().c_str()) );
    }
  }

  // Content-transfer-encoding
  if (headers.HasContentTransferEncoding())
    aPart->setCteStr(headers.ContentTransferEncoding().AsString().c_str());
  else
    aPart->setCteStr("7bit");

  // Content-description
  if (headers.HasContentDescription())
    aPart->setContentDescription(headers.ContentDescription().AsString().c_str());
  else
    aPart->setContentDescription("");

  // Content-disposition
  if (headers.HasContentDisposition())
    aPart->setContentDisposition(headers.ContentDisposition().AsString().c_str());
  else
    aPart->setContentDisposition("");
}

//-----------------------------------------------------------------------------
void KMMessage::bodyPart(DwBodyPart* aDwBodyPart, KMMessagePart* aPart,
                         bool withBody)
{
  if ( !aPart )
    return;

  aPart->clear();

  if( aDwBodyPart && aDwBodyPart->hasHeaders()  ) {
    // This must not be an empty string, because we'll get a
    // spurious empty Subject: line in some of the parts.
    //aPart->setName(" ");
    // partSpecifier
    QString partId( aDwBodyPart->partId() );
    aPart->setPartSpecifier( partId );

    DwHeaders& headers = aDwBodyPart->Headers();
    applyHeadersToMessagePart( headers, aPart );

    // Body
    if (withBody)
      aPart->setBody( aDwBodyPart->Body().AsString().c_str() );
    else
      aPart->setBody( "" );

    // Content-id
    if ( headers.HasContentId() ) {
      const QByteArray contentId = headers.ContentId().AsString().c_str();
      // ignore leading '<' and trailing '>'
      aPart->setContentId( contentId.mid( 1, contentId.length() - 2 ) );
    }
  }
  // If no valid body part was given,
  // set all MultipartBodyPart attributes to empty values.
  else
  {
    aPart->setTypeStr("");
    aPart->setSubtypeStr("");
    aPart->setCteStr("");
    // This must not be an empty string, because we'll get a
    // spurious empty Subject: line in some of the parts.
    //aPart->setName(" ");
    aPart->setContentDescription("");
    aPart->setContentDisposition("");
    aPart->setBody("");
    aPart->setContentId("");
  }
}


//-----------------------------------------------------------------------------
void KMMessage::bodyPart(int aIdx, KMMessagePart* aPart) const
{
  if ( !aPart )
    return;

  // If the DwBodyPart was found get the header fields and body
  if ( DwBodyPart *part = dwBodyPart( aIdx ) ) {
    KMMessage::bodyPart(part, aPart);
    if( aPart->name().isEmpty() )
      aPart->setName( i18n("Attachment: %1", aIdx ) );
  }
}


//-----------------------------------------------------------------------------
void KMMessage::deleteBodyParts()
{
  mMsg->Body().DeleteBodyParts();
}

//-----------------------------------------------------------------------------

bool KMMessage::deleteBodyPart( int partIndex )
{
  KMMessagePart part;
  DwBodyPart *dwpart = findPart( partIndex );
  if ( !dwpart )
    return false;
  KMMessage::bodyPart( dwpart, &part, true );
  if ( !part.isComplete() )
     return false;

  DwBody *parentNode = dynamic_cast<DwBody*>( dwpart->Parent() );
  if ( !parentNode )
    return false;
  parentNode->RemoveBodyPart( dwpart );

  // add dummy part to show that a attachment has been deleted
  KMMessagePart dummyPart;
  dummyPart.duplicate( part );
  QString comment = i18n("This attachment has been deleted.");
  if ( !part.fileName().isEmpty() )
    comment = i18n( "The attachment '%1' has been deleted.", part.fileName() );
  dummyPart.setContentDescription( comment );
  dummyPart.setBodyEncodedBinary( QByteArray() );
  QByteArray cd = dummyPart.contentDisposition();
  if ( cd.toLower().indexOf( "inline" ) == 0 ) {
    cd.replace( 0, 10, "attachment" );
    dummyPart.setContentDisposition( cd );
  } else if ( cd.isEmpty() ) {
    dummyPart.setContentDisposition( "attachment" );
  }
  DwBodyPart* newDwPart = createDWBodyPart( &dummyPart );
  parentNode->AddBodyPart( newDwPart );
  getTopLevelPart()->Assemble();
  return true;
}

//-----------------------------------------------------------------------------
DwBodyPart* KMMessage::createDWBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = DwBodyPart::NewBodyPart(s->emptyString, 0);

  if ( !aPart )
    return part;

  QByteArray charset  = aPart->charset();
  QByteArray type     = aPart->typeStr();
  QByteArray subtype  = aPart->subtypeStr();
  QByteArray cte      = aPart->cteStr();
  QByteArray contDesc = aPart->contentDescriptionEncoded();
  QByteArray contDisp = aPart->contentDisposition();
  QByteArray contID   = aPart->contentId();
  QByteArray name     = KMMsgBase::encodeRFC2231StringAutoDetectCharset( aPart->name(), charset );
  bool RFC2231encoded = aPart->name() != QString(name);
  QByteArray paramAttr  = aPart->parameterAttribute();

  DwHeaders& headers = part->Headers();

  DwMediaType& ct = headers.ContentType();
  if (!type.isEmpty() && !subtype.isEmpty())
  {
    ct.SetTypeStr(type.data());
    ct.SetSubtypeStr(subtype.data());
    if (!charset.isEmpty()){
      DwParameter *param;
      param=new DwParameter;
      param->SetAttribute("charset");
      param->SetValue(charset.data());
      ct.AddParameter(param);
    }
  }

  QByteArray additionalParam = aPart->additionalCTypeParamStr();
  if( !additionalParam.isEmpty() )
  {
    QByteArray parAV;
    DwString parA, parV;
    int iL, i1, i2, iM;
    iL = additionalParam.length();
    i1 = 0;
    i2 = additionalParam.indexOf(';', i1);
    while ( i1 < iL )
    {
      if( -1 == i2 )
        i2 = iL;
      if( i1+1 < i2 ) {
        parAV = additionalParam.mid( i1, (i2-i1) );
        iM = parAV.indexOf('=');
        if( -1 < iM )
        {
          parA = parAV.left( iM ).data();
          parV = parAV.right( parAV.length() - iM - 1 ).data();
          if( ('"' == parV.at(0)) && ('"' == parV.at(parV.length()-1)) )
          {
            parV.erase( 0,  1);
            parV.erase( parV.length()-1 );
          }
        }
        else
        {
          parA = parAV.data();
          parV = "";
        }
        DwParameter *param;
        param = new DwParameter;
        param->SetAttribute( parA );
        param->SetValue(     parV );
        ct.AddParameter( param );
      }
      i1 = i2+1;
      i2 = additionalParam.indexOf( ';', i1 );
    }
  }

  if ( !name.isEmpty() ) {
    if (RFC2231encoded)
    {
      DwParameter *nameParam;
      nameParam = new DwParameter;
      nameParam->SetAttribute("name*");
      nameParam->SetValue(name.data(),true);
      ct.AddParameter(nameParam);
    } else {
      ct.SetName(name.data());
    }
  }

  if ( !paramAttr.isEmpty() ) {
    QByteArray paramValue;
    paramValue = KMMsgBase::encodeRFC2231StringAutoDetectCharset( aPart->parameterValue(), charset );
    DwParameter *param = new DwParameter;
    if ( aPart->parameterValue() != QString( paramValue ) ) {
      param->SetAttribute( ( paramAttr + '*' ).data() );
      param->SetValue( paramValue.data(), true );
    } else {
      param->SetAttribute( paramAttr.data() );
      param->SetValue( paramValue.data() );
    }
    ct.AddParameter( param );
  }

  if (!cte.isEmpty())
    headers.Cte().FromString(cte);

  if (!contDesc.isEmpty())
    headers.ContentDescription().FromString( contDesc );

  if (!contDisp.isEmpty())
    headers.ContentDisposition().FromString( contDisp );

  if ( !contID.isEmpty() )
    headers.ContentId().FromString( contID );

  if (!aPart->body().isNull())
    part->Body().FromString(aPart->body());
  else
    part->Body().FromString("");

  if (!aPart->partSpecifier().isNull())
    part->SetPartId( DwString(aPart->partSpecifier().toLatin1()) );

  if (aPart->decodedSize() > 0)
    part->SetBodySize( aPart->decodedSize() );

  return part;
}


//-----------------------------------------------------------------------------
void KMMessage::addDwBodyPart(DwBodyPart * aDwPart)
{
  mMsg->Body().AddBodyPart( aDwPart );
  mNeedsAssembly = true;
}


//-----------------------------------------------------------------------------
void KMMessage::addBodyPart(const KMMessagePart* aPart)
{
  DwBodyPart* part = createDWBodyPart( aPart );
  addDwBodyPart( part );
}

//-----------------------------------------------------------------------------
void KMMessage::readConfig()
{
  KMMsgBase::readConfig();

  KConfigGroup config( KMKernel::config(), "General" );


  { // area for config group "Composer"
    KConfigGroup config( KMKernel::config(), "Composer" );
    s->smartQuote = GlobalSettings::self()->smartQuote();
    s->wordWrap = GlobalSettings::self()->wordWrap();
    s->wrapCol = GlobalSettings::self()->lineWrapWidth();
    if ((s->wrapCol == 0) || (s->wrapCol > 78))
      s->wrapCol = 78;
    if (s->wrapCol < 30)
      s->wrapCol = 30;

    s->prefCharsets = config.readEntry("pref-charsets", QStringList() );
  }

  { // area for config group "Reader"
    KConfigGroup config( KMKernel::config(), "Reader" );
    s->headerStrategy = HeaderStrategy::create( config.readEntry( "header-set-displayed", "rich" ) );
  }
}

//-----------------------------------------------------------------------------
QByteArray KMMessage::charset() const
{
  if ( mMsg->Headers().HasContentType() ) {
    DwMediaType &mType=mMsg->Headers().ContentType();
    mType.Parse();
    DwParameter *param=mType.FirstParameter();
    while(param){
      if (!kasciistricmp(param->Attribute().c_str(), "charset"))
        return param->Value().c_str();
      else param=param->Next();
    }
  }
  return ""; // us-ascii, but we don't have to specify it
}

//-----------------------------------------------------------------------------
void KMMessage::setCharset( const QByteArray &charset, DwEntity *entity )
{
  kWarning( type( entity ) != DwMime::kTypeText )
    << "Trying to set a charset for a non-textual mimetype." << endl
    << "Fix this caller:" << endl
    << "====================================================================" << endl
    << kBacktrace( 5 ) << endl
    << "====================================================================";

  if ( !entity )
    entity = mMsg;

  DwMediaType &mType = entity->Headers().ContentType();
  mType.Parse();
  DwParameter *param = mType.FirstParameter();
  while( param ) {

    // FIXME use the mimelib functions here for comparison.
    if ( !kasciistricmp( param->Attribute().c_str(), "charset" ) )
      break;

    param = param->Next();
  }
  if ( !param ) {
    param = new DwParameter;
    param->SetAttribute( "charset" );
    mType.AddParameter( param );
  }
  else
    mType.SetModified();

  QByteArray lowerCharset = charset;
  kAsciiToLower( lowerCharset.data() );
  param->SetValue( DwString( lowerCharset ) );
  mType.Assemble();
}


//-----------------------------------------------------------------------------
void KMMessage::setStatus(const MessageStatus& aStatus, int idx)
{
  if ( mStatus == aStatus )
    return;
  KMMsgBase::setStatus( aStatus, idx );
}

void KMMessage::setEncryptionState(const KMMsgEncryptionState s, int idx)
{
    if( mEncryptionState == s )
        return;
    mEncryptionState = s;
    mDirty = true;
    KMMsgBase::setEncryptionState(s, idx);
}

void KMMessage::setSignatureState(KMMsgSignatureState s, int idx)
{
    if( mSignatureState == s )
        return;
    mSignatureState = s;
    mDirty = true;
    KMMsgBase::setSignatureState(s, idx);
}

void KMMessage::setMDNSentState( KMMsgMDNSentState status, int idx ) {
  if ( mMDNSentState == status )
    return;
  if ( status == 0 )
    status = KMMsgMDNStateUnknown;
  mMDNSentState = status;
  mDirty = true;
  KMMsgBase::setMDNSentState( status, idx );
}

//-----------------------------------------------------------------------------
void KMMessage::link( const KMMessage *aMsg, const MessageStatus& aStatus )
{
  Q_ASSERT( aStatus.isReplied() || aStatus.isForwarded() || aStatus.isDeleted() );

  QString message = headerField( "X-KMail-Link-Message" );
  if ( !message.isEmpty() )
    message += ',';
  QString type = headerField( "X-KMail-Link-Type" );
  if ( !type.isEmpty() )
    type += ',';

  message += QString::number( aMsg->getMsgSerNum() );
  if ( aStatus.isReplied() )
    type += "reply";
  else if ( aStatus.isForwarded() )
    type += "forward";
  else if ( aStatus.isDeleted() )
    type += "deleted";

  setHeaderField( "X-KMail-Link-Message", message );
  setHeaderField( "X-KMail-Link-Type", type );
}

//-----------------------------------------------------------------------------
void KMMessage::getLink(int n, ulong *retMsgSerNum, MessageStatus& retStatus) const
{
  *retMsgSerNum = 0;
  retStatus.clear();

  QString message = headerField("X-KMail-Link-Message");
  QString type = headerField("X-KMail-Link-Type");
  message = message.section(',', n, n);
  type = type.section(',', n, n);

  if ( !message.isEmpty() && !type.isEmpty() ) {
    *retMsgSerNum = message.toULong();
    if ( type == "reply" )
      retStatus.setReplied();
    else if ( type == "forward" )
      retStatus.setForwarded();
    else if ( type == "deleted" )
      retStatus.setDeleted();
  }
}

//-----------------------------------------------------------------------------
DwBodyPart* KMMessage::findDwBodyPart( DwBodyPart* part, const QString & partSpecifier )
{
  if ( !part ) return 0;
  DwBodyPart* current;

  if ( part->partId() == partSpecifier )
    return part;

  // multipart
  if ( part->hasHeaders() &&
       part->Headers().HasContentType() &&
       part->Body().FirstBodyPart() &&
       (DwMime::kTypeMultipart == part->Headers().ContentType().Type() ) &&
       (current = findDwBodyPart( part->Body().FirstBodyPart(), partSpecifier )) )
  {
    return current;
  }

  // encapsulated message
  if ( part->Body().Message() &&
       part->Body().Message()->Body().FirstBodyPart() &&
       (current = findDwBodyPart( part->Body().Message()->Body().FirstBodyPart(),
                                  partSpecifier )) )
  {
    return current;
  }

  // next part
  return findDwBodyPart( part->Next(), partSpecifier );
}

//-----------------------------------------------------------------------------
void KMMessage::updateBodyPart(const QString partSpecifier, const QByteArray & data)
{
  DwString content( data.data(), data.size() );
  if ( numBodyParts() > 0 &&
       partSpecifier != "0" &&
       partSpecifier != "TEXT" )
  {
    QString specifier = partSpecifier;
    if ( partSpecifier.endsWith( QLatin1String(".HEADER") ) ||
         partSpecifier.endsWith( QLatin1String(".MIME") ) ) {
      // get the parent bodypart
      specifier = partSpecifier.section( '.', 0, -2 );
    }

    // search for the bodypart
    mLastUpdated = findDwBodyPart( getFirstDwBodyPart(), specifier );
    if (!mLastUpdated)
    {
      kWarning() << "Can not find part" << specifier;
      return;
    }
    if ( partSpecifier.endsWith( QLatin1String(".MIME") ) )
    {
      // update headers
      // get rid of EOL
      content.resize( content.length()-2 );
      // we have to delete the fields first as they might have been created by
      // an earlier call to DwHeaders::FieldBody
      mLastUpdated->Headers().DeleteAllFields();
      mLastUpdated->Headers().FromString( content );
      mLastUpdated->Headers().Parse();
    } else if ( partSpecifier.endsWith( QLatin1String(".HEADER") ) )
    {
      // update header of embedded message
      mLastUpdated->Body().Message()->Headers().FromString( content );
      mLastUpdated->Body().Message()->Headers().Parse();
    } else {
      // update body
      mLastUpdated->Body().FromString( content );
      QString parentSpec = partSpecifier.section( '.', 0, -2 );
      if ( !parentSpec.isEmpty() )
      {
        DwBodyPart* parent = findDwBodyPart( getFirstDwBodyPart(), parentSpec );
        if ( parent && parent->hasHeaders() && parent->Headers().HasContentType() )
        {
          const DwMediaType& contentType = parent->Headers().ContentType();
          if ( contentType.Type() == DwMime::kTypeMessage &&
               contentType.Subtype() == DwMime::kSubtypeRfc822 )
          {
            // an embedded message that is not multipart
            // update this directly
            parent->Body().Message()->Body().FromString( content );
          }
        }
      }
    }

  } else
  {
    // update text-only messages
    if ( partSpecifier == "TEXT" )
      deleteBodyParts(); // delete empty parts first
    mMsg->Body().FromString( content );
    mMsg->Body().Parse();
  }
  mNeedsAssembly = true;
  if (! partSpecifier.endsWith( QLatin1String(".HEADER") ) )
  {
    // notify observers
    notify();
  }
}

//-----------------------------------------------------------------------------
void KMMessage::updateAttachmentState( DwBodyPart *partGiven )
{
  DwEntity *part = partGiven;
  DwBodyPart *firstPart = partGiven;

  if ( !part ) {
    part = firstPart = getFirstDwBodyPart();
  }

  if ( !part ) {
    part = mMsg;  // no part, use message itself
  }

  bool filenameEmpty = true;
  if ( part->hasHeaders() ) {
    if ( part->Headers().HasContentDisposition() ) {
      DwDispositionType cd = part->Headers().ContentDisposition();
      filenameEmpty = cd.Filename().empty();
      if ( filenameEmpty ) {
        // let's try if it is rfc 2231 encoded which mimelib can't handle
        filenameEmpty =
          KMMsgBase::decodeRFC2231String( KMMsgBase::extractRFC2231HeaderField( cd.AsString().c_str(), "filename" ) ).isEmpty();
      }
    }

    // Filename still empty? Check if the content-type has a "name" parameter and try to use that as
    // the attachment name
    if ( filenameEmpty && part->Headers().HasContentType() ) {
      DwMediaType contentType = part->Headers().ContentType();
      filenameEmpty = contentType.Name().empty();
      if ( filenameEmpty ) {
        // let's try if it is rfc 2231 encoded which mimelib can't handle
        filenameEmpty = KMMsgBase::decodeRFC2231String( KMMsgBase::extractRFC2231HeaderField(
            contentType.AsString().c_str(), "name" ) ).isEmpty();
      }
    }
  }

  if ( part->hasHeaders() &&
       ( ( part->Headers().HasContentDisposition() &&
           !part->Headers().ContentDisposition().Filename().empty() ) ||
         ( part->Headers().HasContentType() &&
           !filenameEmpty ) ) ) {
    // now blacklist certain ContentTypes
    if ( !part->Headers().HasContentType() ||
         ( part->Headers().HasContentType() &&
           !( StringUtil::isCryptoPart( part->Headers().ContentType().TypeStr().c_str(),
                                        part->Headers().ContentType().SubtypeStr().c_str(),
                                        part->Headers().HasContentDisposition() ?
                                          part->Headers().ContentDisposition().Filename().c_str() :
                                          QString() ) ) ) ) {
      setStatus( MessageStatus::statusHasAttachment() );
    }
    return;
  }

  // multipart
  if ( part->hasHeaders() &&
       part->Headers().HasContentType() &&
       part->Body().FirstBodyPart() &&
       (DwMime::kTypeMultipart == part->Headers().ContentType().Type() ) ) {
    updateAttachmentState( part->Body().FirstBodyPart() );
  }

  // encapsulated message
  if ( part->Body().Message() &&
       part->Body().Message()->Body().FirstBodyPart() ) {
    updateAttachmentState( part->Body().Message()->Body().FirstBodyPart() );
  }

  // next part
  if ( firstPart && firstPart->Next() ) {
    updateAttachmentState( firstPart->Next() );
  } else if ( attachmentState() == KMMsgAttachmentUnknown &&
              mStatus.hasAttachment() ) {
    toggleStatus( MessageStatus::statusHasAttachment() );
  }
}

void KMMessage::setBodyFromUnicode( const QString &str, DwEntity *entity )
{
  QByteArray encoding =
    KMMsgBase::autoDetectCharset( charset(),
                                  preferredCharsets(), str );
  if ( encoding.isEmpty() ) {
    encoding = "utf-8";
  }
  const QTextCodec * codec = KMMsgBase::codecForName( encoding );
  assert( codec );
  QList<int> dummy;
  setCharset( encoding, entity );
  setBodyAndGuessCte( codec->fromUnicode( str ), dummy, false /* no 8bit */,
                      false, entity );
}

const QTextCodec * KMMessage::codec() const
{
  const QTextCodec *c = mOverrideCodec;
  if ( !c ) {
    // no override-codec set for this message, try the CT charset parameter:
    c = KMMsgBase::codecForName( charset() );
  }
  if ( !c ) {
    // Ok, no override and nothing in the message, let's use the fallback
    // the user configured
    c = KMMsgBase::codecForName( GlobalSettings::self()->fallbackCharacterEncoding().toLatin1() );
  }
  if ( !c ) {
    // no charset means us-ascii (RFC 2045), so using local encoding should
    // be okay
    c = kmkernel->networkCodec();
  }
  assert( c );
  return c;
}

QString KMMessage::bodyToUnicode(const QTextCodec *codec) const
{
  if ( !codec ) {
    // No codec was given, so try the charset in the mail
    codec = this->codec();
  }
  assert( codec );

  return codec->toUnicode( bodyDecoded() );
}

//-----------------------------------------------------------------------------
QByteArray KMMessage::mboxMessageSeparator()
{
  QByteArray str( KPIMUtils::firstEmailAddress( rawHeaderField("From") ) );
  if ( str.isEmpty() )
    str = "unknown@unknown.invalid";
  QByteArray dateStr( dateShortStr() );
  if ( dateStr.isEmpty() ) {
    time_t t = ::time( 0 );
    dateStr = ctime( &t );
    const int len = dateStr.length();
    if ( dateStr[len-1] == '\n' )
      dateStr.truncate( len - 1 );
  }
  return "From " + str + ' ' + dateStr + '\n';
}

void KMMessage::deleteWhenUnused()
{
  s->pendingDeletes << this;
}

QByteArray KMMessage::defaultCharset()
{
  QByteArray retval;

  if (!s->prefCharsets.isEmpty())
    retval = s->prefCharsets[0].toLatin1();

  if (retval.isEmpty()  || (retval == "locale")) {
    retval = QByteArray(kmkernel->networkCodec()->name());
    kAsciiToLower( retval.data() );
  }

  if (retval == "jisx0208.1983-0") retval = "iso-2022-jp";
  else if (retval == "ksc5601.1987-0") retval = "euc-kr";
  return retval;
}

const QStringList &KMMessage::preferredCharsets()
{
  return s->prefCharsets;
}

#ifndef NDEBUG
void KMMessage::dump( DwEntity *entity, int level )
{
  if ( !entity ) {
    entity = mMsg;
    entity->Assemble();
  }

  QString spaces;
  for ( int i = 1; i <= level; i++ )
    spaces += "  ";

  kDebug() << QString( spaces + "Headers of entity " + entity->partId() + ':' );
  kDebug() << QString( spaces + entity->Headers().AsString().c_str() );
  kDebug() << QString( spaces + "Body of entity " + entity->partId() + ':' );
  kDebug() << QString( spaces + entity->Body().AsString().c_str() );

  DwBodyPart *current = entity->Body().FirstBodyPart();
  while ( current ) {
    dump( current, level + 1 );
    current = current->Next();
  }
}
#endif

DwBodyPart* KMMessage::findPart( int index )
{
  int accu = 0;
  return findPartInternal( getTopLevelPart(), index, accu );
}

DwBodyPart* KMMessage::findPartInternal(DwEntity * root, int index, int & accu)
{
  accu++;
  if ( index < accu ) // should not happen
    return 0;
  DwBodyPart *current = dynamic_cast<DwBodyPart*>( root );
  if ( index == accu )
    return current;
  DwBodyPart *rv = 0;
  if ( root->Body().FirstBodyPart() )
    rv = findPartInternal( root->Body().FirstBodyPart(), index, accu );
  if ( !rv && current && current->Next() )
    rv = findPartInternal( current->Next(), index, accu );
  if ( !rv && root->Body().Message() )
    rv = findPartInternal( root->Body().Message(), index, accu );
  return rv;
}
