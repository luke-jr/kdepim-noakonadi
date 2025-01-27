/* -*- mode: C++; c-file-style: "gnu" -*-
  kmsearchpatternedit.cpp
  Author: Marc Mutz <Marc@Mutz.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "kmsearchpatternedit.h"

#include <QStackedWidget>
#include "kmsearchpattern.h"
#include "rulewidgethandlermanager.h"
using KMail::RuleWidgetHandlerManager;

#include <kcombobox.h>
#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>

#include <QButtonGroup>
#include <QByteArray>
#include <QHBoxLayout>
#include <QRadioButton>

#include <assert.h>

// Definition of special rule field strings
// Note: Also see KMSearchRule::matches() and ruleFieldToEnglish() if
//       you change the following i18n-ized strings!
// Note: The index of the values in the following array has to correspond to
//       the value of the entries in the enum in KMSearchRuleWidget.

#undef I18N_NOOP
#define I18N_NOOP(t) 0, t
#undef I18N_NOOP2
#define I18N_NOOP2(c,t) c, t

static const struct {
  const char *internalName;
  const char *context;
  const char *displayName;

  QString getLocalizedDisplayName() const { return i18nc(context, displayName); };

} SpecialRuleFields[] = {
  { "<message>",     I18N_NOOP( "Complete Message" )       },
  { "<body>",        I18N_NOOP( "Body of Message" )          },
  { "<any header>",  I18N_NOOP( "Anywhere in Headers" )    },
  { "<recipients>",  I18N_NOOP( "All Recipients" )    },
  { "<size>",        I18N_NOOP( "Size in Bytes" ) },
  { "<age in days>", I18N_NOOP( "Age in Days" )   },
  { "<status>",      I18N_NOOP( "Message Status" )        },
  { "<tag>",         I18N_NOOP( "Message Tag" ) },
  { "Subject",       I18N_NOOP2( "Subject of an email.", "Subject" )  },
  { "From",          I18N_NOOP( "From" )  },
  { "To",            I18N_NOOP2( "Receiver of an email.", "To" )  },
  { "CC",            I18N_NOOP( "CC" )  }
};
static const int SpecialRuleFieldsCount =
  sizeof( SpecialRuleFields ) / sizeof( *SpecialRuleFields );

//=============================================================================
//
// class KMSearchRuleWidget
//
//=============================================================================

KMSearchRuleWidget::KMSearchRuleWidget( QWidget *parent, KMSearchRule *aRule,
                                        bool headersOnly,
                                        bool absoluteDates )
  : QWidget( parent ),
    mRuleField( 0 ),
    mFunctionStack( 0 ),
    mValueStack( 0 ),
    mAbsoluteDates( absoluteDates )
{
  initFieldList( headersOnly, absoluteDates );
  initWidget();

  if ( aRule )
    setRule( aRule );
  else
    reset();
}

void KMSearchRuleWidget::setHeadersOnly( bool headersOnly )
{
  KMSearchRule* srule = rule();
  QByteArray currentText = srule->field();
  delete srule;
  initFieldList( headersOnly, mAbsoluteDates );

  mRuleField->clear();
  mRuleField->addItems( mFilterFieldList );
  mRuleField->setMaxCount( mRuleField->count() );
  mRuleField->adjustSize();

  if (( currentText != "<message>") &&
      ( currentText != "<body>"))
    mRuleField->setItemText( 0, QString::fromAscii( currentText ) );
  else
    mRuleField->setItemText( 0, QString() );
}

void KMSearchRuleWidget::initWidget()
{
  QHBoxLayout * hlay = new QHBoxLayout( this );
  hlay->setSpacing( KDialog::spacingHint() );
  hlay->setMargin( 0 );

  // initialize the header field combo box
  mRuleField = new KComboBox( this );
  mRuleField->setObjectName( "mRuleField" );
  mRuleField->setEditable( true );
  mRuleField->addItems( mFilterFieldList );
  // don't show sliders when popping up this menu
  mRuleField->setMaxCount( mRuleField->count() );
  mRuleField->adjustSize();
  hlay->addWidget( mRuleField );

  // initialize the function/value widget stack
  mFunctionStack = new QStackedWidget( this );
  //Don't expand the widget in vertical direction
  mFunctionStack->setSizePolicy( QSizePolicy::Preferred,QSizePolicy::Fixed );

  hlay->addWidget( mFunctionStack );

  mValueStack = new QStackedWidget( this );
  mValueStack->setSizePolicy( QSizePolicy::Preferred,QSizePolicy::Fixed );
  hlay->addWidget( mValueStack );
  hlay->setStretchFactor( mValueStack, 10 );

  RuleWidgetHandlerManager::instance()->createWidgets( mFunctionStack,
                                                       mValueStack,
                                                       this );

  // redirect focus to the header field combo box
  setFocusProxy( mRuleField );

  connect( mRuleField, SIGNAL( activated( const QString & ) ),
           this, SLOT( slotRuleFieldChanged( const QString & ) ) );
  connect( mRuleField, SIGNAL(editTextChanged( const QString & ) ),
           this, SLOT( slotRuleFieldChanged( const QString & ) ) );
  connect( mRuleField, SIGNAL(editTextChanged( const QString & ) ),
           this, SIGNAL( fieldChanged( const QString & ) ) );
}

void KMSearchRuleWidget::setRule( KMSearchRule *aRule )
{
  assert ( aRule );

//  kDebug() << "(" << aRule->asString() << ")";

  //--------------set the field
  int i = indexOfRuleField( aRule->field() );

  mRuleField->blockSignals( true );

  if ( i < 0 ) { // not found -> user defined field
    mRuleField->setItemText( 0, QString::fromLatin1( aRule->field() ) );
    i = 0;
  } else { // found in the list of predefined fields
    mRuleField->setItemText( 0, QString() );
  }

  mRuleField->setCurrentIndex( i );
  mRuleField->blockSignals( false );

  RuleWidgetHandlerManager::instance()->setRule( mFunctionStack, mValueStack,
                                                 aRule );
}

KMSearchRule* KMSearchRuleWidget::rule() const {
  const QByteArray ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  const KMSearchRule::Function function =
    RuleWidgetHandlerManager::instance()->function( ruleField,
                                                    mFunctionStack );
  const QString value =
    RuleWidgetHandlerManager::instance()->value( ruleField, mFunctionStack,
                                                 mValueStack );

  return KMSearchRule::createInstance( ruleField, function, value );
}

void KMSearchRuleWidget::reset()
{
  mRuleField->blockSignals( true );
  mRuleField->setItemText( 0, "" );
  mRuleField->setCurrentIndex( 0 );
  mRuleField->blockSignals( false );

  RuleWidgetHandlerManager::instance()->reset( mFunctionStack, mValueStack );
}

void KMSearchRuleWidget::slotFunctionChanged()
{
  const QByteArray ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  RuleWidgetHandlerManager::instance()->update( ruleField,
                                                mFunctionStack,
                                                mValueStack );
}

void KMSearchRuleWidget::slotValueChanged()
{
  const QByteArray ruleField = ruleFieldToEnglish( mRuleField->currentText() );
  const QString prettyValue =
    RuleWidgetHandlerManager::instance()->prettyValue( ruleField,
                                                       mFunctionStack,
                                                       mValueStack );
  emit contentsChanged( prettyValue );
}

QByteArray KMSearchRuleWidget::ruleFieldToEnglish( const QString & i18nVal )
{
  for ( int i = 0; i < SpecialRuleFieldsCount; ++i ) {
    if ( i18nVal == SpecialRuleFields[i].getLocalizedDisplayName() )
      return SpecialRuleFields[i].internalName;
  }
  return i18nVal.toLatin1();
}

int KMSearchRuleWidget::ruleFieldToId( const QString & i18nVal )
{
  for ( int i = 0; i < SpecialRuleFieldsCount; ++i ) {
    if ( i18nVal == SpecialRuleFields[i].getLocalizedDisplayName() )
      return i;
  }
  return -1; // no pseudo header
}

static QString displayNameFromInternalName( const QString & internal )
{
  for ( int i = 0; i < SpecialRuleFieldsCount; ++i ) {
    if ( internal == SpecialRuleFields[i].internalName )
      return SpecialRuleFields[i].getLocalizedDisplayName();
  }
  return internal.toLatin1();
}



int KMSearchRuleWidget::indexOfRuleField( const QByteArray & aName ) const
{
  if ( aName.isEmpty() )
    return -1;

  QString i18n_aName = displayNameFromInternalName( aName );

  for ( int i = 1; i < mRuleField->count(); ++i ) {
    if ( mRuleField->itemText( i ) == i18n_aName )
      return i;
  }

  return -1;
}

void KMSearchRuleWidget::initFieldList( bool headersOnly, bool absoluteDates )
{
  mFilterFieldList.clear();
  mFilterFieldList.append(""); // empty entry for user input
  if( !headersOnly ) {
    mFilterFieldList.append( SpecialRuleFields[Message].getLocalizedDisplayName() );
    mFilterFieldList.append( SpecialRuleFields[Body].getLocalizedDisplayName()    );
  }
  mFilterFieldList.append( SpecialRuleFields[AnyHeader].getLocalizedDisplayName()  );
  mFilterFieldList.append( SpecialRuleFields[Recipients].getLocalizedDisplayName() );
  mFilterFieldList.append( SpecialRuleFields[Size].getLocalizedDisplayName()       );
  if ( !absoluteDates )
    mFilterFieldList.append( SpecialRuleFields[AgeInDays].getLocalizedDisplayName() );
  mFilterFieldList.append( SpecialRuleFields[Subject].getLocalizedDisplayName() );
  mFilterFieldList.append( SpecialRuleFields[From].getLocalizedDisplayName()    );
  mFilterFieldList.append( SpecialRuleFields[To].getLocalizedDisplayName()      );
  mFilterFieldList.append( SpecialRuleFields[CC].getLocalizedDisplayName()      );
  mFilterFieldList.append( SpecialRuleFields[Status].getLocalizedDisplayName()  );
  mFilterFieldList.append( SpecialRuleFields[Tag].getLocalizedDisplayName()     );

  // these others only represent message headers and you can add to
  // them as you like
  mFilterFieldList.append("Reply-To");
  mFilterFieldList.append("List-Id");
  mFilterFieldList.append("Organization");
  mFilterFieldList.append("Resent-From");
  mFilterFieldList.append("X-Loop");
  mFilterFieldList.append("X-Mailing-List");
  mFilterFieldList.append("X-Spam-Flag");
}

void KMSearchRuleWidget::slotRuleFieldChanged( const QString & field )
{
  RuleWidgetHandlerManager::instance()->update( ruleFieldToEnglish( field ),
                                                mFunctionStack,
                                                mValueStack );
}

//=============================================================================
//
// class KMFilterActionWidgetLister (the filter action editor)
//
//=============================================================================

KMSearchRuleWidgetLister::KMSearchRuleWidgetLister( QWidget *parent, const char* name, bool headersOnly, bool absoluteDates )
  : KWidgetLister( 2, FILTER_MAX_RULES, parent, name )
{
  mRuleList = 0;
  mHeadersOnly = headersOnly;
  mAbsoluteDates = absoluteDates;
}

KMSearchRuleWidgetLister::~KMSearchRuleWidgetLister()
{
}

void KMSearchRuleWidgetLister::setRuleList( QList<KMSearchRule*> *aList )
{
  assert ( aList );

  if ( mRuleList && mRuleList != aList )
    regenerateRuleListFromWidgets();

  mRuleList = aList;

  if ( !mWidgetList.isEmpty() ) // move this below next 'if'?
    mWidgetList.first()->blockSignals(true);

  if ( aList->count() == 0 ) {
    slotClear();
    mWidgetList.first()->blockSignals(false);
    return;
  }

  int superfluousItems = (int)mRuleList->count() - mMaxWidgets ;
  if ( superfluousItems > 0 ) {
    kDebug() << "Clipping rule list to" << mMaxWidgets << "items!";

    for ( ; superfluousItems ; superfluousItems-- )
      mRuleList->removeLast();
  }

  // HACK to workaround regression in Qt 3.1.3 and Qt 3.2.0 (fixes bug #63537)
  setNumberOfShownWidgetsTo( qMax((int)mRuleList->count(),mMinWidgets)+1 );
  // set the right number of widgets
  setNumberOfShownWidgetsTo( qMax((int)mRuleList->count(),mMinWidgets) );

  // load the actions into the widgets
  QList<KMSearchRule*>::const_iterator rIt;
  QList<QWidget*>::const_iterator wIt = mWidgetList.constBegin();
  for ( rIt = mRuleList->constBegin();
        rIt != mRuleList->constEnd() && wIt != mWidgetList.constEnd(); ++rIt, ++wIt ) {
    static_cast<KMSearchRuleWidget*>( *wIt )->setRule( (*rIt) );
  }
  for ( ; wIt != mWidgetList.constEnd() ; ++wIt )
    static_cast<KMSearchRuleWidget*>( *wIt )->reset();

  assert( !mWidgetList.isEmpty() );
  mWidgetList.first()->blockSignals(false);
}

void KMSearchRuleWidgetLister::setHeadersOnly( bool headersOnly )
{
  foreach ( QWidget *w, mWidgetList ) {
    static_cast<KMSearchRuleWidget*>( w )->setHeadersOnly( headersOnly );
  }
}

void KMSearchRuleWidgetLister::reset()
{
  if ( mRuleList )
    regenerateRuleListFromWidgets();

  mRuleList = 0;
  slotClear();
}

QWidget* KMSearchRuleWidgetLister::createWidget( QWidget *parent )
{
  return new KMSearchRuleWidget(parent, 0,  mHeadersOnly, mAbsoluteDates);
}

void KMSearchRuleWidgetLister::clearWidget( QWidget *aWidget )
{
  if ( aWidget )
    ((KMSearchRuleWidget*)aWidget)->reset();
}

void KMSearchRuleWidgetLister::regenerateRuleListFromWidgets()
{
  if ( !mRuleList ) return;

  mRuleList->clear();

  foreach ( const QWidget *w, mWidgetList ) {
    KMSearchRule *r = static_cast<const KMSearchRuleWidget*>( w )->rule();
    if ( r )
      mRuleList->append( r );
  }
}




//=============================================================================
//
// class KMSearchPatternEdit
//
//=============================================================================

KMSearchPatternEdit::KMSearchPatternEdit( QWidget *parent, bool headersOnly,
                                          bool absoluteDates )
  : QGroupBox( parent )
{
  setObjectName( "KMSearchPatternEdit" );
  setTitle( i18n("Search Criteria") );
  initLayout( headersOnly, absoluteDates );
}

KMSearchPatternEdit::KMSearchPatternEdit( const QString &title, QWidget *parent,
                                          bool headersOnly, bool absoluteDates )
  : QGroupBox( title, parent )
{
  setObjectName( "KMSearchPatternEdit" );
  initLayout( headersOnly, absoluteDates );
}

KMSearchPatternEdit::~KMSearchPatternEdit()
{
}

void KMSearchPatternEdit::initLayout(bool headersOnly, bool absoluteDates)
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  //------------the radio buttons
  mAllRBtn = new QRadioButton( i18n("Match a&ll of the following"), this );
  mAnyRBtn = new QRadioButton( i18n("Match an&y of the following"), this );

  mAllRBtn->setObjectName( "mAllRBtn" );
  mAllRBtn->setChecked(true);
  mAnyRBtn->setObjectName( "mAnyRBtn" );
  mAnyRBtn->setChecked(false);

  layout->addWidget( mAllRBtn );
  layout->addWidget( mAnyRBtn );

  QButtonGroup *bg = new QButtonGroup( this );
  bg->addButton( mAllRBtn );
  bg->addButton( mAnyRBtn );

  //------------connect a few signals
  connect( bg, SIGNAL(buttonClicked(QAbstractButton *)),
	   this, SLOT(slotRadioClicked(QAbstractButton *)) );

  //------------the list of KMSearchRuleWidget's
  mRuleLister = new KMSearchRuleWidgetLister( this, "swl", headersOnly, absoluteDates );
  mRuleLister->slotClear();

  if ( !mRuleLister->mWidgetList.isEmpty() ) {
    KMSearchRuleWidget *srw = static_cast<KMSearchRuleWidget*>( mRuleLister->mWidgetList.first() );
    connect( srw, SIGNAL(fieldChanged(const QString &)),
             this, SLOT(slotAutoNameHack()) );
    connect( srw, SIGNAL(contentsChanged(const QString &)),
             this, SLOT(slotAutoNameHack()) );
  } else
    kDebug() << "No first KMSearchRuleWidget, though slotClear() has been called!";

  layout->addWidget( mRuleLister );
}

void KMSearchPatternEdit::setSearchPattern( KMSearchPattern *aPattern )
{
  assert( aPattern );

  mRuleLister->setRuleList( aPattern );

  mPattern = aPattern;

  blockSignals(true);
  if ( mPattern->op() == KMSearchPattern::OpOr )
    mAnyRBtn->setChecked(true);
  else
    mAllRBtn->setChecked(true);
  blockSignals(false);

  setEnabled( true );
}

void KMSearchPatternEdit::setHeadersOnly( bool headersOnly )
{
  mRuleLister->setHeadersOnly( headersOnly );
}

void KMSearchPatternEdit::reset()
{
  mRuleLister->reset();

  blockSignals(true);
  mAllRBtn->setChecked( true );
  blockSignals(false);

  setEnabled( false );
}

void KMSearchPatternEdit::slotRadioClicked(QAbstractButton *aRBtn)
{
  if ( mPattern ) {
    if ( aRBtn == mAllRBtn )
      mPattern->setOp( KMSearchPattern::OpAnd );
    else
      mPattern->setOp( KMSearchPattern::OpOr );
  }
}

void KMSearchPatternEdit::slotAutoNameHack()
{
  mRuleLister->regenerateRuleListFromWidgets();
  emit maybeNameChanged();
}

#include "kmsearchpatternedit.moc"
