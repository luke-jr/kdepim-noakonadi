// kmsettings.cpp

#include <qdir.h>

#include "kmacctlocal.h"
#include "kmacctmgr.h"
#include "kmacctpop.h"
#include "kmacctseldlg.h"
#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmglobal.h"
#include "kmidentity.h"
#include "kmmainwin.h"
#include "kmsender.h"
#include "kmmessage.h"
#include "kpgp.h"
#include "kfontdialog.h"
#include "kfontutils.h"
#include "kmtopwidget.h"

#include <kapp.h>
#include <kfiledialog.h>
#include <ktablistbox.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qvalidator.h>
#include <qmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcolorbtn.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#include <errno.h>
#include <fcntl.h>

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL  "/usr/sbin/sendmail"
#endif

//------
#include "kmsettings.moc"

static QIntValidator intValidator(1, 0xFFFF, NULL);

//-----------------------------------------------------------------------------
KMSettings::KMSettings(QWidget *parent, const char *name) :
  QTabDialog(parent, name, TRUE)
{
  initMetaObject();

  setCaption(i18n("Settings"));
  //resize(500,600);
  setOKButton(i18n("OK"));
  setCancelButton(i18n("Cancel"));

  connect(this, SIGNAL(applyButtonPressed()), this, SLOT(doApply()));
  connect(this, SIGNAL(cancelButtonPressed()), this, SLOT(doCancel()));

  createTabIdentity(this);
  createTabNetwork(this);
  createTabAppearance(this);
  createTabComposer(this);
  createTabMime(this);
  createTabMisc(this);
  createTabPgp(this);
}


//-----------------------------------------------------------------------------
KMSettings::~KMSettings()
{
  debug("~KMSettings");
  accountList->clear();
}


//-----------------------------------------------------------------------------
// Create an input field with a label and optional detail button ("...")
// The detail button is not created if detail_return is NULL.
// The argument 'label' is the label that will be left of the entry field.
// The argument 'text' is the text that will show up in the entry field.
// The whole thing is placed in the grid from row/col to the right.
static QLineEdit* createLabeledEntry(QWidget* parent, QGridLayout* grid,
				     const QString& aLabel,
				     const QString& aText,
				     int gridy, int gridx,
				     QPushButton** detail_return=NULL)
{
  QLabel* label = new QLabel(parent);
  QLineEdit* edit = new QLineEdit(parent);
  QPushButton* sel;

  label->setText(aLabel);
  label->adjustSize();
  label->resize((int)label->sizeHint().width(),label->sizeHint().height() + 6);
  label->setMinimumSize(label->size());
  grid->addWidget(label, gridy, gridx++);

  if (!aText.isEmpty()) edit->setText(aText);
  edit->setMinimumSize(100, label->height()+2);
  edit->setMaximumSize(1000, label->height()+2);
  grid->addWidget(edit, gridy, gridx++);

  if (detail_return)
  {
    sel = new QPushButton("...", parent);
    sel->setFocusPolicy(QWidget::NoFocus);
    sel->setFixedSize(sel->sizeHint().width(), label->height());
    grid->addWidget(sel, gridy, gridx++);
    *detail_return = sel;
  }

  return edit;
}


//-----------------------------------------------------------------------------
// Add a widget with a label and optional detail button ("...")
// The detail button is not created if detail_return is NULL.
// The argument 'label' is the label that will be left of the entry field.
// The whole thing is placed in the grid from row/col to the right.
static void addLabeledWidget(QWidget* parent, QGridLayout* grid,
			     const QString& aLabel, QWidget* widg,
			     int gridy, int gridx,
			     QPushButton** detail_return,
                             QLabel** label_return)
{
  QLabel* label = new QLabel(parent);
  QPushButton* sel;

  label->setText(aLabel);
  label->adjustSize();
  label->resize((int)label->sizeHint().width(),label->sizeHint().height() + 6);
  label->setMinimumSize(label->size());
  grid->addWidget(label, gridy, gridx++);

  widg->setMinimumSize(100, label->height()+2);
  //widg->setMaximumSize(1000, label->height()+2);
  grid->addWidget(widg, gridy, gridx++);

  if (detail_return)
  {
    sel = new QPushButton("...", parent);
    sel->setFocusPolicy(QWidget::NoFocus);
    sel->setFixedSize(sel->sizeHint().width(), label->height());
    label->setBuddy(sel);
    grid->addWidget(sel, gridy, gridx++);
    *detail_return = sel;
  }
  if (label_return)
    *label_return = label;
}


//-----------------------------------------------------------------------------
QPushButton* KMSettings::createPushButton(QWidget* parent, QGridLayout* grid,
					  const QString& label,
					  int gridy, int gridx)
{
  QPushButton* button = new QPushButton(parent, label);
  button->setText(label);
  button->adjustSize();
  button->setMinimumSize(button->size());
  grid->addWidget(button, gridy, gridx);

  return button;
}


//-----------------------------------------------------------------------------
void KMSettings::createTabMime(QWidget* parent)
{
  QWidget *tab = new QWidget(parent);
  QBoxLayout *box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 2);
  QPushButton *bNew, *bDel;
  QGroupBox *grp = new QGroupBox(i18n("Custom Tags"), tab);
  QGridLayout *grid = new QGridLayout(grp, 8, 4, 20, 8);
  QLabel *mhAbout = new QLabel(grp);  
  QLabel *mhName = new QLabel(grp);  
  QLabel *mhValue = new QLabel(grp);
  KConfig* config = app->config();

  curMHItem = NULL;    // not pointing to anything yet

  box->addWidget(grp);

  mhAbout->setText(i18n("Define custom mime header tags for outgoing emails:"));
  mhAbout->setMinimumSize(mhAbout->size());
  grid->addMultiCellWidget(mhAbout, 0, 0, 0, 3);

  tagList = new QListView(grp, "LstTags");
  tagList->addColumn(i18n("Name"), tagList->width()/2);
  tagList->addColumn(i18n("Value"), tagList->width()/2);
  tagList->setColumnWidthMode(0, QListView::Maximum);
  tagList->setColumnWidthMode(1, QListView::Maximum);
  tagList->setMultiSelection(false);
  grid->addMultiCellWidget(tagList, 1, 4, 0, 3);
  tagList->setMultiSelection(false);

  connect(tagList,SIGNAL(selectionChanged()),this,SLOT(slotMHSelectionChanged()));

  mhName->setText(i18n("Name:"));
  mhName->setMinimumSize(mhName->size());
  grid->addWidget(mhName, 5, 0);
  tagNameEdit = new QLineEdit(grp);
  tagNameEdit->setMinimumSize(100, mhName->height()+2);
  tagNameEdit->setMaximumSize(1000, mhName->height()+2);
  grid->addMultiCellWidget(tagNameEdit, 5, 5, 1, 3);
  connect(tagNameEdit,SIGNAL(textChanged(const QString&)),this,SLOT(slotMHNameChanged(const QString&)));

  mhValue->setText(i18n("Value:"));
  mhValue->setMinimumSize(mhValue->size());
  grid->addWidget(mhValue, 6, 0);
  tagValueEdit = new QLineEdit(grp);
  tagValueEdit->setMinimumSize(100, mhValue->height()+2);
  tagValueEdit->setMaximumSize(1000, mhValue->height()+2);
  grid->addMultiCellWidget(tagValueEdit, 6, 6, 1, 3);
  connect(tagValueEdit,SIGNAL(textChanged(const QString&)),this,SLOT(slotMHValueChanged(const QString&)));

  tagNameEdit->setEnabled(false);
  tagValueEdit->setEnabled(false);

  bNew = new QPushButton(i18n("&New"), grp);
  grid->addWidget(bNew, 7, 2);
  bDel = new QPushButton(i18n("D&elete"), grp);
  grid->addWidget(bDel, 7, 3);

  connect(bNew,SIGNAL(clicked()),this,SLOT(slotMHNew()));
  connect(bDel,SIGNAL(clicked()),this,SLOT(slotMHDelete()));

  // read in all the values
  config->setGroup("General");
  int count = config->readNumEntry("mime-header-count", 0);
  for (int i = 0; i < count; i++) {
    QString group, thisName, thisValue;
    group.sprintf("Mime #%d", i);
    config->setGroup(group);
    thisName = config->readEntry("name", "");
    thisValue = config->readEntry("value", "");
    if (thisName.length() > 0) {
      new QListViewItem(tagList, thisName, thisValue);
    }
  }

  addTab(tab, i18n("Mime Headers"));
  grid->activate();
  grp->adjustSize();
  box->addStretch(100);
  box->activate();
  tab->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabIdentity(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QGridLayout* grid = new QGridLayout(tab, 6, 3, 20, 6);
  QPushButton* button;

  nameEdit = createLabeledEntry(tab, grid, i18n("Name:"), 
				identity->fullName(), 0, 0);
  orgEdit = createLabeledEntry(tab, grid, i18n("Organization:"), 
			       identity->organization(), 1, 0);
  emailEdit = createLabeledEntry(tab, grid, i18n("Email Address:"),
				 identity->emailAddr(), 2, 0);
  replytoEdit = createLabeledEntry(tab, grid, 
				   i18n("Reply-To Address:"),
				   identity->replyToAddr(), 3, 0);
  sigEdit = createLabeledEntry(tab, grid, i18n("Signature File:"),
			       identity->signatureFile(), 4, 0, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSigFile()));
  sigModify = createPushButton(tab, grid, i18n("&Edit Signature File..."),
                               5, 0);
  connect(sigModify, SIGNAL(clicked()), this, SLOT(slotSigModify()));

  grid->setColStretch(0,0);
  grid->setColStretch(1,1);
  grid->setColStretch(2,0);
  grid->setRowStretch(5, 100);

  addTab(tab, i18n("Identity"));
  grid->activate();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabNetwork(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox*   grp;
  QButtonGroup*   bgrp;
  QPushButton* button;
  QLabel* label;
  KMAccount* act;
  QString str;

  //---- group: sending mail
  bgrp = new QButtonGroup(i18n("Sending Mail"), tab);
  box->addWidget(bgrp);
  grid = new QGridLayout(bgrp, 6, 4, 20, 4);

  sendmailRadio = new QRadioButton(bgrp);
  sendmailRadio->setMinimumSize(sendmailRadio->size());
  sendmailRadio->setText(i18n("Sendmail"));
  bgrp->insert(sendmailRadio);
  grid->addMultiCellWidget(sendmailRadio, 0, 0, 0, 3);

  sendmailLocationEdit = createLabeledEntry(bgrp, grid, 
					    i18n("Location:"),
					    NULL, 1, 1, &button);
  connect(button,SIGNAL(clicked()),this,SLOT(chooseSendmailLocation()));

  smtpRadio = new QRadioButton(bgrp);
  smtpRadio->setText(i18n("SMTP"));
  smtpRadio->setMinimumSize(smtpRadio->size());
  bgrp->insert(smtpRadio);
  grid->addMultiCellWidget(smtpRadio, 2, 2, 0, 3);

  smtpServerEdit = createLabeledEntry(bgrp, grid, i18n("Server:"),
				      NULL, 3, 1);
  smtpPortEdit = createLabeledEntry(bgrp, grid, i18n("Port:"),
				    NULL, 4, 1);
  grid->setColStretch(0,4);
  grid->setColStretch(1,1);
  grid->setColStretch(2,10);
  grid->setColStretch(3,0);
  grid->setRowStretch(5, 100);
  grid->activate();
  bgrp->adjustSize();

  if (msgSender->method()==KMSender::smMail) 
    sendmailRadio->setChecked(TRUE);
  else if (msgSender->method()==KMSender::smSMTP)
    smtpRadio->setChecked(TRUE);

  sendmailLocationEdit->setText(msgSender->mailer());
  smtpServerEdit->setText(msgSender->smtpHost());
  QString tmp;
  smtpPortEdit->setText(tmp.setNum(msgSender->smtpPort()));


  //---- group: incoming mail
  grp = new QGroupBox(i18n("Incoming Mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 5, 2, 20, 8);

  label = new QLabel(grp);
  label->setText(i18n("Accounts:   (add at least one account!)"));
  label->setMinimumSize(label->size());
  grid->addMultiCellWidget(label, 0, 0, 0, 1);

  accountList = new KTabListBox(grp, "LstAccounts", 3);
  accountList->setColumn(0, i18n("Name"), 150);
  accountList->setColumn(1, i18n("Type"), 60);
  accountList->setColumn(2, i18n("Folder"), 80);
  accountList->setMinimumSize(50, 50);
  connect(accountList,SIGNAL(highlighted(int,int)),
	  this,SLOT(accountSelected(int,int)));
  connect(accountList,SIGNAL(selected(int,int)),
	  this,SLOT(modifyAccount(int,int)));
  grid->addMultiCellWidget(accountList, 1, 4, 0, 0);

  addButton = createPushButton(grp, grid, i18n("Add..."), 1, 1);
  connect(addButton,SIGNAL(clicked()),this,SLOT(addAccount()));

  modifyButton = createPushButton(grp, grid, i18n("Modify..."),2,1);
  connect(modifyButton,SIGNAL(clicked()),this,SLOT(modifyAccount2()));
  modifyButton->setEnabled(FALSE);

  removeButton = createPushButton(grp, grid, i18n("Delete"), 3, 1);
  connect(removeButton,SIGNAL(clicked()),this,SLOT(removeAccount()));
  removeButton->setEnabled(FALSE);

  grid->setColStretch(0, 10);
  grid->setColStretch(1, 0);

  grid->setRowStretch(1, 2);
  grid->setRowStretch(4, 2);
  grid->activate();
  grp->adjustSize();

  accountList->clear();
  for (act=acctMgr->first(); act; act=acctMgr->next())
    accountList->insertItem(tabNetworkAcctStr(act), -1);

  addTab(tab, i18n("Network"));
  box->addStretch(100);
  box->activate();
  tab->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::createTabAppearance(QWidget* parent)
{
  QWidget* tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox* grp;
  KConfig* config = app->config();
  QFont fnt;

  //----- group: fonts
  grp = new QGroupBox(i18n("Fonts"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 4, 20, 4);

  defaultFonts = new QCheckBox( i18n( "&Default Fonts" ), grp );
  grid->addMultiCellWidget(defaultFonts, 2, 2, 0, 2);
  connect(defaultFonts, SIGNAL(clicked()), this, SLOT(slotDefaultFontSelect()));

  bodyFontLabel = new QLabel(grp);
  addLabeledWidget(grp, grid, i18n("Message &Body Font:"), bodyFontLabel, 
		   3, 0, &bodyFontButton, &bodyFontLabel2);
  connect(bodyFontButton,SIGNAL(clicked()),this,SLOT(slotBodyFontSelect()));

  listFontLabel = new QLabel(grp);
  addLabeledWidget(grp, grid, i18n("Message &List Font:"), listFontLabel,
		   4, 0, &listFontButton, &listFontLabel2);
  connect(listFontButton,SIGNAL(clicked()),this,SLOT(slotListFontSelect()));

  folderListFontLabel = new QLabel(grp);
  addLabeledWidget(grp, grid, i18n("&Folder List Font:"), folderListFontLabel,
		   5, 0, &folderListFontButton, &folderListFontLabel2);
  connect(folderListFontButton,SIGNAL(clicked()),this,SLOT(slotFolderlistFontSelect()));

  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->setColStretch(2,0);
  grid->activate();

  // set values
  config->setGroup("Fonts");

  defaultFonts->setChecked( config->readBoolEntry("defaultFonts",TRUE) );
  slotDefaultFontSelect();

  fnt = kstrToFont(config->readEntry("body-font", "helvetica-medium-r-12"));
  bodyFontLabel->setAutoResize(TRUE);
  bodyFontLabel->setText(kfontToStr(fnt));
  bodyFontLabel->setFont(fnt);

  fnt = kstrToFont(config->readEntry("list-font", "helvetica-medium-r-12"));
  listFontLabel->setAutoResize(TRUE);
  listFontLabel->setText(kfontToStr(fnt));
  listFontLabel->setFont(fnt);

  fnt = kstrToFont(config->readEntry("folder-font", "helvetica-medium-r-12"));
  folderListFontLabel->setAutoResize(TRUE);
  folderListFontLabel->setText(kfontToStr(fnt));
  folderListFontLabel->setFont(fnt);

  //----- group: colors
  config->setGroup("Reader");

  QColor c1=QColor(app->palette().normal().text());
  QColor c2=QColor("blue");
  QColor c3=QColor("red");
  QColor c4=QColor(app->palette().normal().base());

  QColor cFore = config->readColorEntry("ForegroundColor",&c1);
  QColor cBack = config->readColorEntry("BackgroundColor",&c4);
  QColor cNew = config->readColorEntry("LinkColor",&c3);
  QColor cUnread = config->readColorEntry("FollowedColor",&c2);

  grp = new QGroupBox(i18n("Colors"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 2, 20, 10);

  defaultColors = new QCheckBox( i18n( "Default &Colors" ), grp );
  defaultColors->setChecked( config->readBoolEntry("defaultColors",TRUE) );
  grid->addMultiCellWidget(defaultColors, 2, 2, 0, 1);
  connect(defaultColors, SIGNAL(clicked()), this, SLOT(slotDefaultColorSelect()));

  backgroundColorBtn = new KColorButton( cBack, grp );
  backgroundColorBtn->setMinimumSize( backgroundColorBtn->sizeHint() );
  foregroundColorBtn = new KColorButton( cFore, grp );
  newColorBtn = new KColorButton( cNew, grp );
  unreadColorBtn = new KColorButton( cUnread, grp );

  backgroundColorLbl = new QLabel( backgroundColorBtn, 
                                   i18n( "B&ackground Color" ), grp );
  grid->addWidget(backgroundColorLbl, 3, 0);
  grid->addWidget(backgroundColorBtn, 3, 1);

  foregroundColorLbl = new QLabel( foregroundColorBtn,
                                   i18n( "&Normal Text Color" ), grp );
  grid->addWidget(foregroundColorLbl, 4, 0);
  grid->addWidget(foregroundColorBtn, 4, 1);

  newColorLbl = new QLabel( newColorBtn, 
                            i18n( "&URL Link/New Color" ), grp );
  grid->addWidget(newColorLbl, 5, 0);
  grid->addWidget(newColorBtn, 5, 1);

  unreadColorLbl = new QLabel( unreadColorBtn,
                               i18n( "F&ollowed Link/Unread Color" ), grp );
  grid->addWidget(unreadColorLbl, 6, 0);
  grid->addWidget(unreadColorBtn, 6, 1);
  grid->setColStretch( 0, 0 );
  grid->setColStretch( 1, 1 );

  slotDefaultColorSelect();

  //----- group: layout
  grp = new QGroupBox(i18n("Layout"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 2, 2, 20, 4);

  longFolderList = new QCheckBox(
	      i18n("Long folder list"), grp);
  longFolderList->adjustSize();
  longFolderList->setMinimumSize(longFolderList->sizeHint());
  grid->addMultiCellWidget(longFolderList, 0, 0, 0, 1);
  grid->activate();

  // set values
  config->setGroup("Geometry");
  longFolderList->setChecked(config->readBoolEntry("longFolderList", false));

  //----- activation
  box->addStretch(100);
  addTab(tab, i18n("Appearance"));
}


//-----------------------------------------------------------------------------
void KMSettings::createTabComposer(QWidget *parent)
{
  QWidget *tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid;
  QGroupBox* grp;
  QLabel* lbl;
  KConfig* config = app->config();
  QString str;
  int i;

  //---------- group: phrases
  grp = new QGroupBox(i18n("Phrases"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 7, 3, 20, 4);

  QString t = i18n(
     "The following placeholders are supported in the reply phrases:\n"
     "%D=date, %S=subject, %F=sender, %%=percent sign");

  t.append (i18n(", %_=space"));
  lbl = new QLabel(t, grp);
  lbl->adjustSize();
  lbl->setMinimumSize(100,lbl->size().height());
  grid->setRowStretch(0,10);
  grid->addMultiCellWidget(lbl, 0, 0, 0, 2);

  phraseReplyEdit = createLabeledEntry(grp, grid, 
				       i18n("Reply to sender:"),
				       NULL, 2, 0);
  phraseReplyAllEdit = createLabeledEntry(grp, grid, 
					  i18n("Reply to all:"),
					  NULL, 3, 0);
  phraseForwardEdit = createLabeledEntry(grp, grid, 
					 i18n("Forward:"),
					 NULL, 4, 0);
  indentPrefixEdit = createLabeledEntry(grp, grid, 
					i18n("Indentation:"),
					NULL, 5, 0);
  grid->setColStretch(0,1);
  grid->setColStretch(1,10);
  grid->setColStretch(2,0);
  grid->activate();
  //grp->adjustSize();


  // set the values
  config->setGroup("KMMessage");
  str = config->readEntry("phrase-reply");
  if (str.isEmpty()) str = i18n("On %D, you wrote:");
  phraseReplyEdit->setText(str);

  str = config->readEntry("phrase-reply-all");
  if (str.isEmpty()) str = i18n("On %D, %F wrote:");
  phraseReplyAllEdit->setText(str);

  str = config->readEntry("phrase-forward");
  if (str.isEmpty()) str = i18n("Forwarded Message");
  phraseForwardEdit->setText(str);

  indentPrefixEdit->setText(config->readEntry("indent-prefix", ">%_"));

  //---------- group appearance
  grp = new QGroupBox(i18n("Appearance"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 8, 5, 20, 4);

  autoAppSignFile=new QCheckBox(
	      i18n("Automatically append signature"), grp);
  autoAppSignFile->adjustSize();
  autoAppSignFile->setMinimumSize(autoAppSignFile->sizeHint());
  grid->addMultiCellWidget(autoAppSignFile, 0, 0, 0, 1);

  pgpAutoSign=new QCheckBox(
	      i18n("Automatically sign messages using PGP"), grp);
  pgpAutoSign->adjustSize();
  pgpAutoSign->setMinimumSize(pgpAutoSign->sizeHint());
  grid->addMultiCellWidget(pgpAutoSign, 1, 1, 0, 1);

  wordWrap = new QCheckBox(i18n("Word wrap at column:"), grp);
  wordWrap->adjustSize();
  wordWrap->setMinimumSize(autoAppSignFile->sizeHint());
  grid->addWidget(wordWrap, 2, 0);

  wrapColumnEdit = new QLineEdit(grp);
  wrapColumnEdit->adjustSize();
  wrapColumnEdit->setMinimumSize(50, wrapColumnEdit->sizeHint().height());
  grid->addWidget(wrapColumnEdit, 2, 1);

  monospFont = new QCheckBox(i18n("Use monospaced font") +
			     QString(" (still broken)"), grp);
  monospFont->adjustSize();
  monospFont->setMinimumSize(monospFont->sizeHint());
  grid->addMultiCellWidget(monospFont, 3, 3, 0, 1);

  smartQuote = new QCheckBox(i18n("Smart quoting"), grp);
  smartQuote->adjustSize();
  smartQuote->setMinimumSize(smartQuote->sizeHint());
  grid->addWidget(smartQuote, 0, 3);

  grid->setColStretch(0,1);
  grid->setColStretch(1,1);
  grid->setColStretch(2,1);
  grid->addColSpacing(2,20);
  grid->setColStretch(3,1);
  grid->setColStretch(4,10);
  grid->activate();

  //---------- group sending
  grp = new QGroupBox(i18n("When sending mail"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 4, 3, 20, 4);
  lbl = new QLabel(i18n("Default sending:"), grp);
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 0, 0);
  sendNow = new QRadioButton(i18n("send now"), grp);
  sendNow->setMinimumSize(sendNow->sizeHint());
  connect(sendNow,SIGNAL(clicked()),SLOT(slotSendNow()));
  grid->addWidget(sendNow, 0, 1);
  sendLater = new QRadioButton(i18n("send later"), grp);
  sendLater->setMinimumSize(sendLater->sizeHint());
  connect(sendLater,SIGNAL(clicked()),SLOT(slotSendLater()));
  grid->addWidget(sendLater, 0, 2);

  lbl = new QLabel(i18n("Send messages:"), grp);
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 1, 0);
  allow8Bit = new QRadioButton(i18n("Allow 8-bit"), grp);
  allow8Bit->setMinimumSize(allow8Bit->sizeHint());
  connect(allow8Bit,SIGNAL(clicked()),SLOT(slotAllow8Bit()));
  grid->addWidget(allow8Bit, 1, 1);
  quotedPrintable = new QRadioButton(i18n(
			  "MIME Compilant (Quoted Printable)"), grp);
  quotedPrintable->setMinimumSize(quotedPrintable->sizeHint());
  connect(quotedPrintable,SIGNAL(clicked()),SLOT(slotQuotedPrintable()));
  grid->addWidget(quotedPrintable, 1, 2);
  confirmSend = new QCheckBox(i18n("Confirm before send"), grp);
  grid->addWidget(confirmSend, 2, 1);
  grid->activate();

  // set values
  config->setGroup("Composer");
  autoAppSignFile->setChecked(stricmp(config->readEntry("signature"),"auto")==0);
  wordWrap->setChecked(config->readBoolEntry("word-wrap",true));
  wrapColumnEdit->setText(config->readEntry("break-at","78"));
#if 0
  // Limit break between 60 and 78
  if ((mLineBreak == 0) || (mLineBreak > 78))
    mLineBreak = 78;
  if (mLineBreak < 60)
    mLineBreak = 60;
#endif
  monospFont->setChecked(stricmp(config->readEntry("font","variable"),"fixed")==0);
  pgpAutoSign->setChecked(config->readBoolEntry("pgp-auto-sign",false));
  smartQuote->setChecked(config->readBoolEntry("smart-quote",true));
  confirmSend->setChecked(config->readBoolEntry("confirm-before-send",false));

  i = msgSender->sendImmediate();
  sendNow->setChecked(i);
  sendLater->setChecked(!i);

  i = msgSender->sendQuotedPrintable();
  allow8Bit->setChecked(!i);
  quotedPrintable->setChecked(i);

  //---------- �re we g�
  box->addStretch(10);
  box->activate();
 
  addTab(tab, i18n("Composer"));
}



//-----------------------------------------------------------------------------
void KMSettings::createTabMisc(QWidget *parent)
{
  QWidget *tab = new QWidget(parent);
  QBoxLayout* box = new QBoxLayout(tab, QBoxLayout::TopToBottom, 4);
  QGridLayout* grid, *grid2, *grid3;
  QGroupBox* grp, *grp2, *grp3;

  KConfig* config = app->config();
  QString str;

  //---------- group: folders
  grp = new QGroupBox(i18n("Folders"), tab);
  box->addWidget(grp);
  grid = new QGridLayout(grp, 4, 3, 20, 4);

  emptyTrashOnExit=new QCheckBox(i18n("Empty trash on exit"),grp);
  emptyTrashOnExit->setMinimumSize(emptyTrashOnExit->sizeHint());
  grid->addMultiCellWidget(emptyTrashOnExit, 0, 0, 0, 2);

  sendOnCheck = new QCheckBox(i18n("Send Mail in outbox Folder on Check"),grp);
  sendOnCheck->setMinimumSize(sendOnCheck->sizeHint());
  grid->addMultiCellWidget(sendOnCheck,1,1,0,2);

  sendReceipts = new QCheckBox(i18n("Automatically send receive- and read confirmations"),grp);
  sendReceipts->setMinimumSize(sendReceipts->sizeHint());
  grid->addMultiCellWidget(sendReceipts, 2, 2, 0, 2);

  compactOnExit = new QCheckBox(i18n("Compact all folders on exit"),grp);
  compactOnExit->setMinimumSize(compactOnExit->sizeHint());
  grid->addMultiCellWidget(compactOnExit, 3, 3, 0, 2);

  grid->activate();

  //---------- group: External editor
  grp2 = new QGroupBox(i18n("External Editor"), tab);
  box->addWidget(grp2);
  grid2 = new QGridLayout(grp2, 3, 2, 20, 6);

  // Checkbox: use external editor instead of composer?
  useExternalEditor = 
          new QCheckBox(i18n("Use external editor instead of composer"), grp2);
  useExternalEditor->adjustSize();
  useExternalEditor->setMinimumSize(useExternalEditor->sizeHint());
  grid2->addMultiCellWidget(useExternalEditor, 1, 1, 0, 0);

  QLabel *editorHowto = new QLabel(grp2);
  editorHowto->setText(i18n("%f will be replaced with the filename to edit."));
  editorHowto->setIndent(10);
  editorHowto->adjustSize();
  editorHowto->setMinimumSize(editorHowto->size());
  editorHowto->resize(editorHowto->sizeHint().width(),
                      editorHowto->sizeHint().height());

  grid2->addWidget(editorHowto, 2, 0);
  extEditorEdit = 
              createLabeledEntry(grp2, grid2, i18n("External editor command:"),
 			         QString(""), 3, 0);
  grid2->activate();
  

  //---------- group: New Mail Notification
  grp3 = new QGroupBox(i18n("New Mail Notification"), tab);
  box->addWidget(grp3);
  grid3 = new QGridLayout(grp3, 4, 2, 20, 6);

  beepNotify = new QCheckBox(i18n("Beep on new mail"), grp3);
  beepNotify->adjustSize();
  beepNotify->setMinimumSize(beepNotify->sizeHint());
  grid3->addMultiCellWidget(beepNotify, 0, 0, 0, 0);

  msgboxNotify = new QCheckBox(i18n("Display message box on new mail"), grp3);
  msgboxNotify->adjustSize();
  msgboxNotify->setMinimumSize(msgboxNotify->sizeHint());
  grid3->addMultiCellWidget(msgboxNotify, 1, 1, 0, 0);

  execNotify = new QCheckBox(i18n("Execute command line on new mail"), grp3);
  execNotify->adjustSize();
  execNotify->setMinimumSize(execNotify->sizeHint());
  grid3->addMultiCellWidget(execNotify, 2, 2, 0, 0);
  connect(execNotify,SIGNAL(toggled(bool)),this,SLOT(slotExecOnMail(bool)));

  mailNotifyEdit = new QLineEdit(grp3);
  mailNotifyEdit->adjustSize();
  grid3->addMultiCellWidget(mailNotifyEdit, 2, 2, 1, 1);

  grid3->activate();


  //---------- set values
  config->setGroup("General");
  emptyTrashOnExit->setChecked(config->readBoolEntry("empty-trash-on-exit",false));
  compactOnExit->setChecked(config->readNumEntry("compact-all-on-exit", 0));
  sendOnCheck->setChecked(config->readBoolEntry("sendOnCheck",false));
  sendReceipts->setChecked(config->readBoolEntry("send-receipts", true));
  useExternalEditor->setChecked(config->readBoolEntry("use-external-editor", false));  
  extEditorEdit->setText(config->readEntry("external-editor", ""));
  beepNotify->setChecked(config->readBoolEntry("beep-on-mail", false));
  msgboxNotify->setChecked(config->readBoolEntry("msgbox-on-mail", false));
  execNotify->setChecked(config->readBoolEntry("exec-on-mail", false));
  mailNotifyEdit->setText(config->readEntry("mail-notify-cmd", ""));
  mailNotifyEdit->setEnabled(execNotify->isChecked());

  //---------- here we go
  box->addStretch(10);
  box->activate();
 
  addTab(tab, i18n("Misc"));
}

// ----------------------------------------------------------------------------
void KMSettings::createTabPgp(QWidget *parent)
{
  pgpConfig = new KpgpConfig(parent);
  addTab(pgpConfig, i18n("PGP"));
}

//-----------------------------------------------------------------------------
const QString KMSettings::tabNetworkAcctStr(const KMAccount* act) const
{
  QString str;
  
  str = "";
  str += act->name();
  str += "\n";
  str += act->type();
  str += "\n";
  if (act->folder()) str += act->folder()->name();

  return str;
}

//-----------------------------------------------------------------------------
void KMSettings::slotDefaultColorSelect()
{
   bool enabled = !defaultColors->isChecked();
 
   backgroundColorLbl->setEnabled(enabled);   
   backgroundColorBtn->setEnabled(enabled);   
   foregroundColorLbl->setEnabled(enabled);   
   foregroundColorBtn->setEnabled(enabled);   
   newColorLbl->setEnabled(enabled);   
   newColorBtn->setEnabled(enabled);   
   unreadColorLbl->setEnabled(enabled);   
   unreadColorBtn->setEnabled(enabled);   
}

//-----------------------------------------------------------------------------
void KMSettings::slotDefaultFontSelect()
{
   bool enabled = !defaultFonts->isChecked();

   bodyFontLabel->setEnabled(enabled);   
   bodyFontButton->setEnabled(enabled);   
   bodyFontLabel2->setEnabled(enabled);   
   listFontLabel->setEnabled(enabled);   
   listFontButton->setEnabled(enabled);   
   listFontLabel2->setEnabled(enabled);   
   folderListFontLabel->setEnabled(enabled);   
   folderListFontButton->setEnabled(enabled);   
   folderListFontLabel2->setEnabled(enabled);   
}


//-----------------------------------------------------------------------------
void KMSettings::slotBodyFontSelect()
{
  QFont font;
  KFontDialog dlg(this, i18n("Message Body Font"), TRUE);

  font = kstrToFont(bodyFontLabel->text());
  dlg.getFont(font);
  bodyFontLabel->setFont(font);
  bodyFontLabel->setText(kfontToStr(font));
  bodyFontLabel->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::slotListFontSelect()
{
  QFont font;
  KFontDialog dlg(this, i18n("Message List Font"), TRUE);

  font = kstrToFont(listFontLabel->text());
  dlg.getFont(font);
  listFontLabel->setFont(font);
  listFontLabel->setText(kfontToStr(font));
  listFontLabel->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::slotFolderlistFontSelect()
{
  QFont font;
  KFontDialog dlg(this, i18n("Folder List Font"), TRUE);

  font = kstrToFont(folderListFontLabel->text());
  dlg.getFont(font);
  folderListFontLabel->setFont(font);
  folderListFontLabel->setText(kfontToStr(font));
  folderListFontLabel->adjustSize();
}


//-----------------------------------------------------------------------------
void KMSettings::slotAllow8Bit()
{
  allow8Bit->setChecked(TRUE);
  quotedPrintable->setChecked(FALSE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotQuotedPrintable()
{
  allow8Bit->setChecked(FALSE);
  quotedPrintable->setChecked(TRUE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotSendNow()
{
  sendNow->setChecked(TRUE);
  sendLater->setChecked(FALSE);
}


//-----------------------------------------------------------------------------
void KMSettings::slotSendLater()
{
  sendNow->setChecked(FALSE);
  sendLater->setChecked(TRUE);
}


//-----------------------------------------------------------------------------
//-----------------  Utility class for the proceeding slot --------------------
//-----------------------------------------------------------------------------

//
//  I tried to mimick QThread's interface here.
//

//
//  FIXME:
//  There is a problem here.  If an interactive console program is given,
//  the program is stopped.  How do we work around this?
//  FIXME:
//  Use the KDE launching service instead
//

class ExtEditLaunch {
private:
   QString cmdline;

public:
  ExtEditLaunch(QString cmd) { cmdline=cmd; }
  ~ExtEditLaunch()          {}
  void run();
  inline void setFile(QString cmd) {cmdline = cmd;}
private:
  void doIt();
};

void ExtEditLaunch::doIt() {
   signal(SIGCHLD, SIG_IGN); // This isn't used anywhere else so
                             // it should be safe to do this here.
                             // I dont' see how we can cleanly wait
                             // on all possible childs in this app so
                             // I use this hack instead.  Another
                             // alternative is to fork() twice, recursively,
                             // but that is slower.
   system((const char *)cmdline);
}

void ExtEditLaunch::run() {
  signal(SIGCHLD, SIG_IGN);  // see comment above.

  if (!fork()) {
     doIt();
     exit(0);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
void KMSettings::slotSigModify()
{
  struct stat sb;
  int rc = stat((const char *)sigEdit->text(), &sb);

  // verify: is valid file?
  if (!rc && (S_ISREG(sb.st_mode) || 
              S_ISCHR(sb.st_mode) ||
              S_ISLNK(sb.st_mode)   )) {
    // run external editor
    QString cmdline(extEditorEdit->text());
    if (cmdline.length() == 0)
       cmdline = DEFAULT_EDITOR_STR;
    QString fileName = "\"" + sigEdit->text() + "\"";
    ExtEditLaunch kl(cmdline.replace(QRegExp("\\%f"), fileName));
    kl.run();
    
  } else {
    if (rc == -1 || errno == ENOENT) {
      // Create the file and then edit
      if ((sigEdit->text()).length() > 0 && 
         (rc = creat((const char *)sigEdit->text(), S_IREAD|S_IWRITE)) != -1) {
        close(rc);    // we don't need the fd anymore

        // run external editor
        QString cmdline(extEditorEdit->text());
        if (cmdline.length() == 0)
           cmdline = DEFAULT_EDITOR_STR;
        QString fileName = "\"" + sigEdit->text() + "\"";
        ExtEditLaunch kl(cmdline.replace(QRegExp("\\%f"), fileName));
        kl.run();
      } else {
        // message box: permission denied
        QMessageBox::warning(this, i18n("Error Opening Signature"),
                                   i18n("Error creating new signature."),
                                   i18n("Ok"));
        return;
      }
    } else {
      // message box: file no good
      QMessageBox::warning(this, i18n("Error Opening Signature"),
                                 i18n("File is invalid or permission denied."),
                                 i18n("Ok"));
      return;
    }
  }

  
}


//-----------------------------------------------------------------------------
void KMSettings::slotMHValueChanged(const QString& x)
{

  curMHItem->setText(1, x);

}


//-----------------------------------------------------------------------------
void KMSettings::slotMHNameChanged(const QString& x)
{

  curMHItem->setText(0, x);

}


//-----------------------------------------------------------------------------
void KMSettings::slotMHSelectionChanged()
{

  // get text from the selection
  curMHItem = tagList->selectedItem();

  // put it in the entry fields
  tagNameEdit->setText(curMHItem->text(0));
  tagValueEdit->setText(curMHItem->text(1));

  // enable them
  tagNameEdit->setEnabled(true);
  tagValueEdit->setEnabled(true);

}


//-----------------------------------------------------------------------------
void KMSettings::slotMHNew()
{

  curMHItem = new QListViewItem(tagList, QString(""), QString(""));
  tagNameEdit->setEnabled(true);
  tagValueEdit->setEnabled(true);
  tagList->setCurrentItem(curMHItem);
  tagNameEdit->setFocus();

}


//-----------------------------------------------------------------------------
void KMSettings::slotMHDelete()
{

  // Verify that it is selected
  if (!curMHItem)
    return;

  // clear the entry fields
  tagNameEdit->setText("");
  tagValueEdit->setText("");
  tagNameEdit->setEnabled(false);
  tagValueEdit->setEnabled(false);

  // Remove the item
  tagList->takeItem(curMHItem);
  curMHItem = NULL;

}


//-----------------------------------------------------------------------------
void KMSettings::slotExecOnMail(bool x)
{

  mailNotifyEdit->setEnabled(x);

}


//-----------------------------------------------------------------------------
void KMSettings::accountSelected(int,int)
{
  modifyButton->setEnabled(TRUE);
  removeButton->setEnabled(TRUE);
}

//-----------------------------------------------------------------------------
void KMSettings::addAccount()
{
  KMAcctSelDlg acctSel(this, i18n("Select Account"));
  KMAccount* acct;
  KMAccountSettings* acctSettings;
  const char* acctType = NULL;

  if (!acctSel.exec()) return;

  switch(acctSel.selected())
  {
  case 0:
    acctType = "local";
    break;
  case 1:
    acctType = "pop";
    break;
  default:
    fatal("KMSettings: unsupported account type selected");
  }

  acct = acctMgr->create(acctType, i18n("Unnamed"));
  assert(acct != NULL);

  acct->init(); // fill the account fields with good default values

  acctSettings = new KMAccountSettings(this, NULL, acct);

  if (acctSettings->exec())
    accountList->insertItem(tabNetworkAcctStr(acct), -1);
  else
    acctMgr->remove(acct);

  delete acctSettings;
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSendmailLocation()
{
  KFileDialog dlg("/", "*", this, NULL, TRUE);
  dlg.setCaption(i18n("Choose Sendmail Location"));

  if (dlg.exec()) sendmailLocationEdit->setText(dlg.selectedFile());
}

//-----------------------------------------------------------------------------
void KMSettings::chooseSigFile()
{
  KFileDialog *d=new KFileDialog(QDir::homeDirPath(),"*",this,NULL,TRUE);
  d->setCaption(i18n("Choose Signature File"));
  if (d->exec()) sigEdit->setText(d->selectedFile());
  delete d;
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount(int index,int)
{
  KMAccount* acct;
  KMAccountSettings* d;

  if (index < 0) return;

  acct = acctMgr->find(accountList->text(index,0));
  assert(acct != NULL);

  d = new KMAccountSettings(this, NULL, acct);
  d->exec();
  delete d;

  accountList->changeItem(tabNetworkAcctStr(acct), index);
  accountList->setCurrentItem(index);
}

//-----------------------------------------------------------------------------
void KMSettings::modifyAccount2()
{
  modifyAccount(accountList->currentItem(),-1);
}

//-----------------------------------------------------------------------------
void KMSettings::removeAccount()
{
  QString acctName;
  KMAccount* acct;
  int idx = accountList->currentItem();

  acctName = accountList->text(idx, 0);
  acct = acctMgr->find(acctName);
  if (!acct) return;

  if(!acctMgr->remove(acct))
    return;
  accountList->removeItem(idx);
  if (!accountList->count())
  {
    modifyButton->setEnabled(FALSE);
    removeButton->setEnabled(FALSE);
  }
  if (idx >= (int)accountList->count()) idx--;
  if (idx >= 0) accountList->setCurrentItem(idx);

  accountList->update();
}

//-----------------------------------------------------------------------------
void KMSettings::setDefaults()
{
  sigEdit->setText(QString(QDir::home().path())+"/.signature");
  sendmailRadio->setChecked(TRUE);
  sendmailLocationEdit->setText(_PATH_SENDMAIL);
  smtpRadio->setChecked(FALSE);
  smtpPortEdit->setText("25");
}

//-----------------------------------------------------------------------------
void KMSettings::doCancel()
{
  identity->readConfig();
  msgSender->readConfig();
  acctMgr->readConfig();
}

//-----------------------------------------------------------------------------
void KMSettings::doApply()
{
  KConfig* config = app->config();

  //----- identity
  identity->setFullName(nameEdit->text());
  identity->setOrganization(orgEdit->text());
  identity->setEmailAddr(emailEdit->text());
  identity->setReplyToAddr(replytoEdit->text());
  identity->setSignatureFile(sigEdit->text());
  identity->writeConfig(FALSE);
  
  //----- sending mail
  if (sendmailRadio->isChecked())
    msgSender->setMethod(KMSender::smMail);
  else
    msgSender->setMethod(KMSender::smSMTP);
  
  msgSender->setMailer(sendmailLocationEdit->text());
  msgSender->setSmtpHost(smtpServerEdit->text());
  msgSender->setSmtpPort(atoi(smtpPortEdit->text()));
  msgSender->setSendImmediate(sendNow->isChecked());
  msgSender->setSendQuotedPrintable(quotedPrintable->isChecked());
  msgSender->writeConfig(FALSE);
  
  //----- incoming mail
  acctMgr->writeConfig(FALSE);

  //----- fonts
  config->setGroup("Fonts");
  config->writeEntry("defaultFonts", defaultFonts->isChecked());
  config->writeEntry("body-font", bodyFontLabel->text());
  config->writeEntry("list-font", listFontLabel->text());
  config->writeEntry("folder-font", folderListFontLabel->text());

  //----- colors
  config->setGroup("Reader");
  config->writeEntry("defaultColors", defaultColors->isChecked());
  config->writeEntry("BackgroundColor", backgroundColorBtn->color() );
  config->writeEntry("ForegroundColor", foregroundColorBtn->color() );
  config->writeEntry("LinkColor", newColorBtn->color() );
  config->writeEntry("FollowedColor", unreadColorBtn->color() );

  config->writeEntry("body-font", bodyFontLabel->text());
  config->writeEntry("list-font", listFontLabel->text());
  config->writeEntry("folder-font", folderListFontLabel->text());

  //----- layout
  config->setGroup("Geometry");
  config->writeEntry("longFolderList", longFolderList->isChecked());

  //----- composer phrases
  config->setGroup("KMMessage");
  config->writeEntry("phrase-reply", phraseReplyEdit->text());
  config->writeEntry("phrase-reply-all", phraseReplyAllEdit->text());
  config->writeEntry("phrase-forward", phraseForwardEdit->text());
  config->writeEntry("indent-prefix", indentPrefixEdit->text());

  //----- composer appearance
  config->setGroup("Composer");
  config->writeEntry("signature",autoAppSignFile->isChecked()?"auto":"manual");
  config->writeEntry("word-wrap",wordWrap->isChecked());
  config->writeEntry("break-at", atoi(wrapColumnEdit->text()));
  config->writeEntry("font", monospFont->isChecked()?"fixed":"variable");
  config->writeEntry("pgp-auto-sign", pgpAutoSign->isChecked());
  config->writeEntry("smart-quote", smartQuote->isChecked());
  config->writeEntry("confirm-before-send", confirmSend->isChecked());

  //----- misc
  config->setGroup("General");
  config->writeEntry("empty-trash-on-exit", emptyTrashOnExit->isChecked());
  config->writeEntry("compact-all-on-exit", compactOnExit->isChecked());
  config->writeEntry("first-start", false);
  config->writeEntry("sendOnCheck",sendOnCheck->isChecked());
  config->writeEntry("send-receipts", sendReceipts->isChecked());
  config->writeEntry("external-editor", extEditorEdit->text());
  config->writeEntry("use-external-editor", useExternalEditor->isChecked());
  config->writeEntry("beep-on-mail", beepNotify->isChecked());
  config->writeEntry("msgbox-on-mail", msgboxNotify->isChecked());
  config->writeEntry("exec-on-mail", execNotify->isChecked());
  config->writeEntry("exec-on-mail-cmd", mailNotifyEdit->text());

  //----- mime headers
  int count = tagList->childCount();
  int countDelta = 0;
  for (int i = 0; i < count; i++) {
    QString groupName;
    QListViewItem *thisChild;

    groupName.sprintf("Mime #%d", i); 
    config->setGroup(groupName);
    thisChild = tagList->firstChild();
    if (thisChild->text(0) && strlen(thisChild->text(0)) > 0) {
      config->writeEntry("name", thisChild->text(0));
      config->writeEntry("value", thisChild->text(1));
    } else {
      countDelta++;
    }
    tagList->takeItem(thisChild);
  }
  config->setGroup("General");
  config->writeEntry("mime-header-count", count - countDelta);

  //-----
  config->sync();
  pgpConfig->applySettings();

  folderMgr->contentsChanged();
  KMMessage::readConfig();

  KMTopLevelWidget::forEvery(&KMTopLevelWidget::readConfig);
}


//=============================================================================
//=============================================================================
KMAccountSettings::KMAccountSettings(QWidget *parent, const char *name,
				     KMAccount *aAcct):
  QDialog(parent,name,TRUE)
{
  QGridLayout *grid;
  QPushButton *btnDetail, *ok, *cancel;
  QString acctType;
  QWidget *btnBox;
  QLabel *lbl;
  KMFolder *folder, *acctFolder;
  KMFolderDir* fdir = (KMFolderDir*)&folderMgr->dir();
  int i;

  initMetaObject();

  assert(aAcct != NULL);

  mAcct=aAcct;
  setCaption(name);

  acctType = mAcct->type();

  setCaption("Configure Account");
  grid = new QGridLayout(this, 19, 3, 8, 4);
  grid->setColStretch(1, 5);

  lbl = new QLabel(i18n("Type:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addWidget(lbl, 0, 0);

  lbl = new QLabel(this);
  grid->addWidget(lbl, 0, 1);

  mEdtName = createLabeledEntry(this, grid, i18n("Name:"),
				mAcct->name(), 1, 0);

  if (acctType == "local")
  {
    lbl->setText(i18n("Local Account"));

    mEdtLocation = createLabeledEntry(this, grid, i18n("Location:"),
				      ((KMAcctLocal*)mAcct)->location(),
				      2, 0, &btnDetail);
    connect(btnDetail,SIGNAL(clicked()), SLOT(chooseLocation()));
  }
  else if (acctType == "pop")
  {
    lbl->setText(i18n("Pop Account"));

    mEdtLogin = createLabeledEntry(this, grid, i18n("Login:"),
				   ((KMAcctPop*)mAcct)->login(), 2, 0);

    mEdtPasswd = createLabeledEntry(this, grid, i18n("Password:"),
				    ((KMAcctPop*)mAcct)->passwd(), 3, 0);
    mEdtPasswd->setEchoMode(QLineEdit::Password);

    mEdtHost = createLabeledEntry(this, grid, i18n("Host:"),
				  ((KMAcctPop*)mAcct)->host(), 4, 0);

    QString tmpStr;
    tmpStr.sprintf("%u",((KMAcctPop*)mAcct)->port());
    mEdtPort = createLabeledEntry(this, grid, i18n("Port:"),
				  tmpStr, 5, 0);
    tmpStr.sprintf("%u", ((KMAccount*)mAcct)->checkInterval());
    mChkInt = createLabeledEntry(this, grid, i18n("Check interval(minutes):"),
                                  tmpStr, 6, 0);
    if (((KMAccount*)mAcct)->checkInterval() < 1) {
       mChkInt->setEnabled(false);
    }
    // The range on this validator is 1..0xFFFF which should be fine for
    // both fields.
    mEdtPort->setValidator(&intValidator);
    mChkInt->setValidator(&intValidator);

    mStorePasswd = new QCheckBox(i18n("Store POP password in config file"), this);
    mStorePasswd->setMinimumSize(mStorePasswd->sizeHint());
    mStorePasswd->setChecked(((KMAcctPop*)mAcct)->storePasswd());
    grid->addMultiCellWidget(mStorePasswd, 7, 7, 1, 2);

    mChkDelete = new QCheckBox(i18n("Delete mail from server"), this);
    mChkDelete->setMinimumSize(mChkDelete->sizeHint());
    mChkDelete->setChecked(!((KMAcctPop*)mAcct)->leaveOnServer());
    grid->addMultiCellWidget(mChkDelete, 8, 8, 1, 2);

    mChkRetrieveAll=new QCheckBox(i18n("Retrieve all mail from server"), this);
    mChkRetrieveAll->setMinimumSize(mChkRetrieveAll->sizeHint());
    mChkRetrieveAll->setChecked(((KMAcctPop*)mAcct)->retrieveAll());
    grid->addMultiCellWidget(mChkRetrieveAll, 9, 9, 1, 2);

  }
  else 
  {
    warning("KMAccountSettings: unsupported account type");
    return;
  }

  mChkInterval = new QCheckBox(i18n("Enable interval Mail checking"), this);
  mChkInterval->setMinimumSize(mChkInterval->sizeHint());
  mChkInterval->setChecked(mAcct->checkInterval() > 0);
  connect(mChkInterval,SIGNAL(clicked()),SLOT(slotIntervalChange()));
  grid->addMultiCellWidget(mChkInterval, 10, 10, 1, 2);

  mChkExclude = new QCheckBox(i18n("Exclude from \"Check Mail\""), this);
  mChkExclude->setMinimumSize(mChkExclude->sizeHint());
  mChkExclude->setChecked(mAcct->checkExclude());
  grid->addMultiCellWidget(mChkExclude, 11, 11, 1, 2);

  // label with "Local Account" or "Pop Account" created previously
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());


  lbl = new QLabel(i18n("Store new mail in account:"), this);
  lbl->adjustSize();
  lbl->setMinimumSize(lbl->sizeHint());
  grid->addMultiCellWidget(lbl, 12, 12, 0, 2);

  // combobox of all folders with current account folder selected
  acctFolder = mAcct->folder();
  if (acctFolder == 0)
  {
    acctFolder = (KMFolder*)fdir->first();
  }  
  mFolders = new QComboBox(this);
  if (acctFolder == 0)
  {
    mFolders->insertItem(i18n("<none>"));
  }
  else for (i=0, folder=(KMFolder*)fdir->first(); 
  	    folder; 
            folder=(KMFolder*)fdir->next())
  {
    if (folder->isDir() || folder->isSystemFolder()) continue;
    mFolders->insertItem(folder->name());
    if (folder == acctFolder) mFolders->setCurrentItem(i);
    i++;
  }
  mFolders->adjustSize();
  mFolders->setMinimumSize(100, mEdtName->minimumSize().height());
  mFolders->setMaximumSize(500, mEdtName->minimumSize().height());
  grid->addWidget(mFolders, 13, 1);


  // buttons at bottom
  btnBox = new QWidget(this);
  ok = new QPushButton(i18n("OK"), btnBox);
  ok->adjustSize();
  ok->setMinimumSize(ok->sizeHint());
  ok->resize(100, ok->size().height());
  ok->move(10, 5);
  connect(ok, SIGNAL(clicked()), SLOT(accept()));

  cancel = new QPushButton(i18n("Cancel"), btnBox);
  cancel->adjustSize();
  cancel->setMinimumSize(cancel->sizeHint());
  cancel->resize(100, cancel->size().height());
  cancel->move(120, 5);
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));

  btnBox->setMinimumSize(230, ok->size().height()+10);
  btnBox->setMaximumSize(2048, ok->size().height()+10);
  grid->addMultiCellWidget(btnBox, 16, 16, 0, 2);

  resize(350,350);
  grid->activate();
  adjustSize();
  setMinimumSize(size());
}


//-----------------------------------------------------------------------------
void KMAccountSettings::chooseLocation()
{
  static QString sSelLocation("/");
  KFileDialog fdlg(sSelLocation,"*",this,NULL,TRUE);
  fdlg.setCaption(i18n("Choose Location"));

  if (fdlg.exec()) mEdtLocation->setText(fdlg.selectedFile());
  sSelLocation = fdlg.selectedFile().copy();
}

//-----------------------------------------------------------------------------
void KMAccountSettings::accept()
{
  QString acctType = mAcct->type();
  KMFolder* fld;
  int id;

  if (mEdtName->text() != mAcct->name())
  {
    mAcct->setName(mEdtName->text());
  }

  id = mFolders->currentItem();
  fld = folderMgr->find(mFolders->currentText());
  mAcct->setFolder((KMFolder*)fld);

  int _ChkInt = atoi(mChkInt->text());
  if (_ChkInt < 1)
     _ChkInt = ((KMAccount*)mAcct)->defaultCheckInterval();
  mAcct->setCheckInterval(mChkInterval->isChecked() ? _ChkInt : 0);
  mAcct->setCheckExclude(mChkExclude->isChecked());

  if (acctType == "local")
  {
    ((KMAcctLocal*)mAcct)->setLocation(mEdtLocation->text());
  }

  else if (acctType == "pop")
  {
    ((KMAcctPop*)mAcct)->setHost(mEdtHost->text());
    ((KMAcctPop*)mAcct)->setPort(atoi(mEdtPort->text()));
    ((KMAcctPop*)mAcct)->setLogin(mEdtLogin->text());
    ((KMAcctPop*)mAcct)->setPasswd(mEdtPasswd->text(), true);
    ((KMAcctPop*)mAcct)->setStorePasswd(mStorePasswd->isChecked());
    ((KMAcctPop*)mAcct)->setPasswd(mEdtPasswd->text(),
                 ((KMAcctPop*)mAcct)->storePasswd());
    ((KMAcctPop*)mAcct)->setLeaveOnServer(!mChkDelete->isChecked());
    ((KMAcctPop*)mAcct)->setRetrieveAll(mChkRetrieveAll->isChecked());
  }

  acctMgr->writeConfig(TRUE);

  QDialog::accept();
}



//-----------------------------------------------------------------------------
void KMAccountSettings::slotIntervalChange()
{
  if (mChkInterval->isChecked()) {
      int _ChkInt = atoi(mChkInt->text());
      if (_ChkInt < 1)
         mChkInt->setText(QString("5"));      
      mChkInt->setEnabled(true);
  } else mChkInt->setEnabled(false);
}
