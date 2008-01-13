/*
 *  File : snippet_widget.cpp
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#include <kdebug.h>
#include <klocale.h>
#include <qlayout.h>
#include <kpushbutton.h>
#include <qheaderview.h>
#include <kmessagebox.h>
#include <qsplitter.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kconfig.h>
#include <qtooltip.h>
#include <kmenu.h>
#include <qregexp.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qdrag.h>
#include <QtGui/QContextMenuEvent>
#include <qtimer.h>
#include <kcombobox.h>
#include <kdeversion.h>
#include <kmedit.h>

#include "snippetitem.h"
#include "snippet_widget.h"

SnippetWidget::SnippetWidget(KMeditor* editor, QWidget* parent)
 : QTreeWidget(parent),
   _cfg( 0 ),
   mEditor( editor )
{
    setObjectName("snippet widget");
    // init the QTreeWidget
    //setSorting( -1 );
    //setFullWidth(true);
    header()->hide();
    setAcceptDrops(true);
    setDragEnabled(true);
    //setDropVisualizer(false);
    setRootIsDecorated(true);

    connect( this, SIGNAL( itemActivated (QTreeWidgetItem *) ),
             this, SLOT( slotEdit( QTreeWidgetItem *) ) );

//### in KDE 3 the enter key used to insert the snippet in the editor
//    (slotExecute()) but the enterPressed signal is gone. It is also a problem
//    that pressing enter on an item and double-clicking is supposed have the
//    exact same effect.

    connect( editor, SIGNAL( insertSnippet() ), this, SLOT( slotExecute() ));

    QTimer::singleShot(0, this, SLOT(initConfig()));
}

SnippetWidget::~SnippetWidget()
{
  writeConfig();
  delete _cfg;

  /* We need to delete the child items before the parent items
     otherwise KMail would crash on exiting */
  int itemsLeft = _list.count();
  while (itemsLeft) {
    for (int i = 0; i < _list.count(); i++) {
      if (_list[i] && _list[i]->childCount() == 0) {
        delete _list[i];
        _list[i] = 0;
        itemsLeft--;
      }
    }
  }
}


/*!
    \fn SnippetWidget::slotAdd()
    Opens the dialog to add a snippet
 */
void SnippetWidget::slotAdd()
{
  kDebug(5006) << "Ender slotAdd()" << endl;
  SnippetDlg dlg( this );
  dlg.setObjectName( "SnippetDlg" );

  /*check if the user clicked a SnippetGroup
    If not, we set the group variable to the SnippetGroup
    which the selected item is a child of*/
  SnippetGroup * group = dynamic_cast<SnippetGroup*>(selectedItem());
  if (!group)
    group = dynamic_cast<SnippetGroup*>(selectedItem()->parent());

  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach (SnippetItem *item, _list) {
    if (dynamic_cast<SnippetGroup*>(item)) {
      dlg.cbGroup->insertItem(item->getName());
    }
  }
  dlg.cbGroup->setCurrentText(group->getName());

  if (dlg.exec() == QDialog::Accepted) {
      group = dynamic_cast<SnippetGroup*>(SnippetItem::findItemByName(dlg.cbGroup->currentText(), _list));
      _list.append( new SnippetItem(group, dlg.snippetName->text(), dlg.snippetText->text()) );
  }
}


/*!
    \fn SnippetWidget::slotAddGroup()
    Opens the didalog to add a snippet
 */
void SnippetWidget::slotAddGroup()
{
  kDebug(5006) << "Ender slotAddGroup()" << endl;
  SnippetDlg dlg( this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetText->setEnabled(false);
  dlg.snippetText->setText("GROUP");
  dlg.setCaption(i18n("Add Group"));
  dlg.cbGroup->insertItem(i18n("All"));
  dlg.cbGroup->setCurrentText(i18n("All"));

  if (dlg.exec() == QDialog::Accepted) {
    _list.append( new SnippetGroup(this, dlg.snippetName->text(), SnippetGroup::getMaxId() ) );
  }
}


/*!
    \fn SnippetWidget::slotRemove()
    Removes the selected snippet
 */
void SnippetWidget::slotRemove()
{
  //get current data
  QTreeWidgetItem * item = currentItem();
  SnippetItem *snip = dynamic_cast<SnippetItem*>( item );
  SnippetGroup *group = dynamic_cast<SnippetGroup*>( item );
  if (!snip)
    return;

  if (group) {
    if (group->childCount() > 0 &&
        KMessageBox::warningContinueCancel(this, i18n("Do you really want to remove this group and all its snippets?"),QString::null, KStandardGuiItem::del())
        == KMessageBox::Cancel)
      return;

    for (int i = 0; i < _list.count(); i++) {
      if (_list[i]->getParent() == group->getId()) {
        kDebug(5006) << "remove " << _list[i]->getName() << endl;
        delete _list.takeAt(i);
      }
    }
  }

  kDebug(5006) << "remove " << snip->getName() << endl;
  int snipIndex = _list.indexOf(snip);
  if (snipIndex > -1)
    delete _list.takeAt(snipIndex);
}



/*!
    \fn SnippetWidget::slotEdit()
    Opens the dialog of editing the selected snippet
 */
void SnippetWidget::slotEdit( QTreeWidgetItem* item )
{
    if( item == 0 ) {
	item = currentItem();
    }

  SnippetGroup *pGroup = dynamic_cast<SnippetGroup*>(item);
  SnippetItem *pSnippet = dynamic_cast<SnippetItem*>( item );
  if (!pSnippet || pGroup) /*selected item must be a SnippetItem but MUST not be a SnippetGroup*/
    return;

  //init the dialog
  SnippetDlg dlg( this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetName->setText(pSnippet->getName());
  dlg.snippetText->setText(pSnippet->getText());
  dlg.btnAdd->setText(i18n("&Apply"));

  dlg.setCaption(i18n("Edit Snippet"));
  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach (SnippetItem *item, _list) {
    if (dynamic_cast<SnippetGroup*>(item)) {
      dlg.cbGroup->insertItem(item->getName());
    }
  }
  dlg.cbGroup->setCurrentText(SnippetItem::findGroupById(pSnippet->getParent(), _list)->getName());

  if (dlg.exec() == QDialog::Accepted) {
    //update the QListView and the SnippetItem
    item->setText( 0, dlg.snippetName->text() );
    pSnippet->setName( dlg.snippetName->text() );
    pSnippet->setText( dlg.snippetText->text() );

    /* if the user changed the parent we need to move the snippet */
    if ( SnippetItem::findGroupById(pSnippet->getParent(), _list)->getName() != dlg.cbGroup->currentText() ) {
      SnippetGroup * newGroup = dynamic_cast<SnippetGroup*>(SnippetItem::findItemByName(dlg.cbGroup->currentText(), _list));
      pSnippet->parent()->removeChild(pSnippet);
      newGroup->addChild(pSnippet);
      pSnippet->resetParent();
    }

    item->setSelected( true );
  }
}

/*!
    \fn SnippetWidget::slotEditGroup()
    Opens the dialog of editing the selected snippet-group
 */
void SnippetWidget::slotEditGroup()
{
  //get current data
  QTreeWidgetItem * item = currentItem();

  SnippetGroup *pGroup = dynamic_cast<SnippetGroup*>( item );
  if (!pGroup) /*selected item MUST be a SnippetGroup*/
    return;

  //init the dialog
  SnippetDlg dlg( this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetName->setText(pGroup->getName());
  dlg.snippetText->setText(pGroup->getText());
  dlg.btnAdd->setText(i18n("&Apply"));
  dlg.snippetText->setEnabled(FALSE);
  dlg.setCaption(i18n("Edit Group"));
  dlg.cbGroup->insertItem(i18n("All"));

  if (dlg.exec() == QDialog::Accepted) {
    //update the QListView and the SnippetGroup
    item->setText( 0, dlg.snippetName->text() );
    pGroup->setName( dlg.snippetName->text() );

    item->setSelected( true );
  }
}

void SnippetWidget::slotExecuted(QTreeWidgetItem * item)
{

  if( item == 0 ) {
    item = currentItem();
  }

  SnippetItem *pSnippet = dynamic_cast<SnippetItem*>( item );
  if (!pSnippet || dynamic_cast<SnippetGroup*>(item))
      return;

  //process variables if any, then insert into the active view
  insertIntoActiveView( parseText(pSnippet->getText(), _SnippetConfig.getDelimiter()) );
}


/*!
    \fn SnippetWidget::insertIntoActiveView(QString text)
    Inserts the parameter text into the active view
 */
void SnippetWidget::insertIntoActiveView(QString text)
{
    mEditor->insertPlainText(text);
}


/*!
    \fn SnippetWidget::writeConfig()
    Write the config file
 */
void SnippetWidget::writeConfig()
{
  if( !_cfg )
    return;
  _cfg->deleteGroup("SnippetPart");  //this is neccessary otherwise delete entries will stay in list until
                                     //they get overwritten by a more recent entry
  KConfigGroup kcg = _cfg->group("SnippetPart");

  SnippetItem *item;
  QString strKeyName="";
  QString strKeyText="";
  QString strKeyId="";

  int iSnipCount = 0;
  int iGroupCount = 0;

  foreach ( item, _list ) {  //write the snippet-list
    kDebug(5006) << "SnippetWidget::writeConfig() " << item->getName() << endl;
    SnippetGroup * group = dynamic_cast<SnippetGroup*>(item);
    if (group) {
      kDebug(5006) << "-->GROUP " << item->getName() << group->getId() << " " << iGroupCount<< endl;
      strKeyName=QString("snippetGroupName_%1").arg(iGroupCount);
      strKeyId=QString("snippetGroupId_%1").arg(iGroupCount);

      kcg.writeEntry(strKeyName, group->getName());
      kcg.writeEntry(strKeyId, group->getId());

      iGroupCount++;
    } else if (dynamic_cast<SnippetItem*>(item)) {
      kDebug(5006) << "-->ITEM " << item->getName() << item->getParent() << " " << iSnipCount << endl;
      strKeyName=QString("snippetName_%1").arg(iSnipCount);
      strKeyText=QString("snippetText_%1").arg(iSnipCount);
      strKeyId=QString("snippetParent_%1").arg(iSnipCount);

      kcg.writeEntry(strKeyName, item->getName());
      kcg.writeEntry(strKeyText, item->getText());
      kcg.writeEntry(strKeyId, item->getParent());
      iSnipCount++;
    } else {
      kDebug(5006) << "-->ERROR " << item->getName() << endl;
    }
  }
  kcg.writeEntry("snippetCount", iSnipCount);
  kcg.writeEntry("snippetGroupCount", iGroupCount);

  int iCount = 1;
  QMap<QString, QString>::Iterator it;
  for ( it = _mapSaved.begin(); it != _mapSaved.end(); ++it ) {  //write the saved variable values
    if (it.data().length()<=0) continue;  //is the saved value has no length -> no need to save it

    strKeyName=QString("snippetSavedName_%1").arg(iCount);
    strKeyText=QString("snippetSavedVal_%1").arg(iCount);

    kcg.writeEntry(strKeyName, it.key());
    kcg.writeEntry(strKeyText, it.data());

    iCount++;
  }
  kcg.writeEntry("snippetSavedCount", iCount-1);


  kcg.writeEntry( "snippetDelimiter", _SnippetConfig.getDelimiter() );
  kcg.writeEntry( "snippetVarInput", _SnippetConfig.getInputMethod() );
  kcg.writeEntry( "snippetToolTips", _SnippetConfig.useToolTips() );
  kcg.writeEntry( "snippetGroupAutoOpen", _SnippetConfig.getAutoOpenGroups() );

  kcg.writeEntry("snippetSingleRect", _SnippetConfig.getSingleRect() );
  kcg.writeEntry("snippetMultiRect", _SnippetConfig.getMultiRect() );

  kcg.sync();
}

/*!
    \fn SnippetWidget::initConfig()
    Initial read the config file
 */
void SnippetWidget::initConfig()
{
  if (!_cfg)
    _cfg = new KConfig("kmailsnippetrc", KConfig::NoGlobals);

  KConfigGroup kcg = _cfg->group("SnippetPart");

  QString strKeyName;
  QString strKeyText;
  QString strKeyId;
  SnippetItem *item;
  SnippetGroup *group;

  kDebug(5006) << "SnippetWidget::initConfig() " << endl;

  //if entry doesn't get found, this will return -1 which we will need a bit later
  int iCount = kcg.readEntry("snippetGroupCount", -1);

  for ( int i=0; i<iCount; i++) {  //read the group-list
    strKeyName=QString("snippetGroupName_%1").arg(i);
    strKeyId=QString("snippetGroupId_%1").arg(i);

    QString strNameVal="";
    int iIdVal=-1;

    strNameVal = kcg.readEntry(strKeyName, "");
    iIdVal = kcg.readEntry(strKeyId, -1);
    kDebug(5006) << "Read group "  << " " << iIdVal << endl;

    if (strNameVal != "" && iIdVal != -1) {
      group = new SnippetGroup(this, strNameVal, iIdVal);
      kDebug(5006) << "Created group " << group->getName() << " " << group->getId() << endl;
      _list.append(group);
    }
  }

  /* Check if the snippetGroupCount property has been found
     if iCount is -1 this means, that the user has his snippets
     stored without groups. Therefore we will call the
     initConfigOldVersion() function below */
  // should not happen since this function has been removed

  if (iCount != -1) {
    iCount = kcg.readEntry("snippetCount", 0);
    for ( int i=0; i<iCount; i++) {  //read the snippet-list
        strKeyName=QString("snippetName_%1").arg(i);
        strKeyText=QString("snippetText_%1").arg(i);
        strKeyId=QString("snippetParent_%1").arg(i);

        QString strNameVal="";
        QString strTextVal="";
        int iParentVal = -1;

        strNameVal = kcg.readEntry(strKeyName, "");
        strTextVal = kcg.readEntry(strKeyText, "");
        iParentVal = kcg.readEntry(strKeyId, -1);
        kDebug(5006) << "Read item " << strNameVal << " " << iParentVal << endl;

        if (strNameVal != "" && strTextVal != "" && iParentVal != -1) {
        item = new SnippetItem(SnippetItem::findGroupById(iParentVal, _list), strNameVal, strTextVal);
        kDebug(5006) << "Created item " << item->getName() << " " << item->getParent() << endl;
        _list.append(item);
        }
    }
  } else {
      kDebug() << "iCount is not -1";
  }

  iCount = kcg.readEntry("snippetSavedCount", 0);

  for ( int i=1; i<=iCount; i++) {  //read the saved-values and store in QMap
    strKeyName=QString("snippetSavedName_%1").arg(i);
    strKeyText=QString("snippetSavedVal_%1").arg(i);

    QString strNameVal="";
    QString strTextVal="";

    strNameVal = kcg.readEntry(strKeyName, "");
    strTextVal = kcg.readEntry(strKeyText, "");

    if (strNameVal != "" && strTextVal != "") {
      _mapSaved[strNameVal] = strTextVal;
    }
  }


  _SnippetConfig.setDelimiter( kcg.readEntry("snippetDelimiter", "$") );
  _SnippetConfig.setInputMethod( kcg.readEntry("snippetVarInput", 0) );
  _SnippetConfig.setToolTips( kcg.readEntry("snippetToolTips", true) );
  _SnippetConfig.setAutoOpenGroups( kcg.readEntry("snippetGroupAutoOpen", 1) );

  _SnippetConfig.setSingleRect( kcg.readEntry( "snippetSingleRect", QRect() ) );
  _SnippetConfig.setMultiRect( kcg.readEntry( "snippetMultiRect", QRect() ) );
}


/*!
    \fn SnippetWidget::contextMenuEvent( QContextMenuEvent *e )
    Shows the Popup-Menu if there is an item where the event occurred
*/
void SnippetWidget::contextMenuEvent( QContextMenuEvent *e )
{
    KMenu popup;

    SnippetItem *item = static_cast<SnippetItem *>(itemAt(e->pos()));
    if ( item ) {
        popup.addTitle( item->getName() );
        popup.addAction( i18n("Paste"), this, SLOT( slotExecuted() ) );
        if (dynamic_cast<SnippetGroup*>(item)) {
            popup.addAction( i18n("Edit group..."), this, SLOT( slotEditGroup() ) );
        } else {
            popup.addAction( i18n("Edit..."), this, SLOT( slotEdit() ) );
        }
        popup.addAction( i18n("Remove"), this, SLOT( slotRemove() ) );
        popup.addSeparator();
        popup.addAction( i18n("Add Snippet..."), this, SLOT( slotAdd() ) );
        popup.addAction( i18n("Add Group..."), this, SLOT( slotAddGroup() ) );
    } else {
        popup.addTitle(i18n("Code Snippets"));

        popup.addAction( i18n("Add Group..."), this, SLOT( slotAddGroup() ) );
    }

    popup.exec(e->globalPos());
}


//  fn SnippetWidget::parseText(QString text, QString del)
/*!
    This function is used to parse the given QString for variables. If found the user will be prompted
    for a replacement value. It returns the string text with all replacements made
 */
QString SnippetWidget::parseText(QString text, QString del)
{
  QString str = text;
  QString strName = "";
  QString strNew = "";
  QString strMsg="";
  int iFound = -1;
  int iEnd = -1;
  QMap<QString, QString> mapVar;
  int iInMeth = _SnippetConfig.getInputMethod();
  QRect rSingle = _SnippetConfig.getSingleRect();
  QRect rMulti = _SnippetConfig.getMultiRect();

  do {
    iFound = text.find(QRegExp("\\"+del+"[A-Za-z-_0-9\\s]*\\"+del), iEnd+1);  //find the next variable by this QRegExp
    if (iFound >= 0) {
      iEnd = text.find(del, iFound+1)+1;
      strName = text.mid(iFound, iEnd-iFound);

      if ( strName != del+del  ) {  //if not doubel-delimiter 
        if (iInMeth == 0) { //if input-method "single" is selected
          if ( mapVar[strName].length() <= 0 ) {  // and not already in map
            strMsg=i18n("Please enter the value for <b>%1</b>:").arg(strName);
            strNew = showSingleVarDialog( strName, &_mapSaved, rSingle );
            if (strNew=="")
              return ""; //user clicked Cancle
          } else {
            continue; //we have already handled this variable
          }
        } else {
          strNew = ""; //for inputmode "multi" just reset new valaue
        }
      } else {
        strNew = del; //if double-delimiter -> replace by single character
      }

      if (iInMeth == 0) {  //if input-method "single" is selected
        str.replace(strName, strNew);
      }

      mapVar[strName] = strNew;
    }
  } while (iFound != -1);

  if (iInMeth == 1) {  //check config, if input-method "multi" is selected
    int w, bh, oh;
    w = rMulti.width();
    bh = rMulti.height();
    oh = rMulti.top();
    if (showMultiVarDialog( &mapVar, &_mapSaved, w, bh, oh )) {  //generate and show the dialog
      QMap<QString, QString>::Iterator it;
      for ( it = mapVar.begin(); it != mapVar.end(); ++it ) {  //walk through the map and do the replacement
        str.replace(it.key(), it.data());
      }
    } else {
      return "";
    }

    rMulti.setWidth(w);   //this is a hack to save the dialog's dimensions in only one QRect
    rMulti.setHeight(bh);
    rMulti.setTop(oh);
    rMulti.setLeft(0);
     _SnippetConfig.setMultiRect(rMulti);
  }

  _SnippetConfig.setSingleRect(rSingle);

  return str;
}


//  fn SnippetWidget::showMultiVarDialog()
/*!
    This function constructs a dialog which contains a label and a linedit for every
    variable that is stored in the given map except the double-delimiter entry
    It return true if everything was ok and false if the user hit cancel
 */
bool SnippetWidget::showMultiVarDialog(QMap<QString, QString> * map, QMap<QString, QString> * mapSave,
                                       int & iWidth, int & iBasicHeight, int & iOneHeight)
{
  //if no var -> no need to show
  if (map->count() == 0)
    return true;

  //if only var is the double-delimiter -> no need to show
  QMap<QString, QString>::Iterator it = map->begin();
  if ( map->count() == 1 && it.data()==_SnippetConfig.getDelimiter()+_SnippetConfig.getDelimiter() )
    return true;

  QMap<QString, KTextEdit *> mapVar2Te;  //this map will help keeping track which TEXTEDIT goes with which variable
  QMap<QString, QCheckBox *> mapVar2Cb;  //this map will help keeping track which CHECKBOX goes with which variable

  // --BEGIN-- building a dynamic dialog
  QDialog dlg(this);
  dlg.setCaption(i18n("Enter Values for Variables"));

  QGridLayout * layout = new QGridLayout( &dlg, 1, 1, 11, 6, "layout");
  QGridLayout * layoutTop = new QGridLayout( 0, 1, 1, 0, 6, "layoutTop");
  QGridLayout * layoutVar = new QGridLayout( 0, 1, 1, 0, 6, "layoutVar");
  QGridLayout * layoutBtn = new QGridLayout( 0, 1, 1, 0, 6, "layoutBtn");

  KTextEdit *te = NULL;
  QLabel * labTop = NULL;
  QCheckBox * cb = NULL;

  labTop = new QLabel( &dlg, "label" );
  labTop->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0,
                         labTop->sizePolicy().hasHeightForWidth() ) );
  labTop->setText(i18n("Enter the replacement values for these variables:"));
  layoutTop->addWidget(labTop, 0, 0);
  layout->addMultiCellLayout( layoutTop, 0, 0, 0, 1 );


  int i = 0;                                           //walk through the variable map and add
  for ( it = map->begin(); it != map->end(); ++it ) {  //a checkbox, a lable and a lineedit to the main layout
    if (it.key() == _SnippetConfig.getDelimiter() + _SnippetConfig.getDelimiter())
      continue;

    cb = new QCheckBox( &dlg, "cbVar" );
    cb->setChecked( FALSE );
    cb->setText(it.key());
    layoutVar->addWidget( cb, i ,0, Qt::AlignTop );

    te = new KTextEdit( &dlg );
    te->setObjectName( "teVar" );
    layoutVar->addWidget( te, i, 1, Qt::AlignTop );

    if ((*mapSave)[it.key()].length() > 0) {
      cb->setChecked( TRUE );
      te->setText((*mapSave)[it.key()]);
    }

    mapVar2Te[it.key()] = te;
    mapVar2Cb[it.key()] = cb;

    QToolTip::add( cb, i18n("Enable this to save the value entered to the right as the default value for this variable") );
    QWhatsThis::add( cb, i18n("If you enable this option, the value entered to the right will be saved. "
                              "If you use the same variable later, even in another snippet, the value entered to the right "
			      "will be the default value for that variable.") );

    i++;
  }
  layout->addMultiCellLayout( layoutVar, 1, 1, 0, 1 );

  KPushButton * btn1 = new KPushButton( &dlg );
  btn1->setObjectName( "pushButton1" );
  btn1->setText(i18n("&Cancel"));
  btn1->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0,
                         btn1->sizePolicy().hasHeightForWidth() ) );
  layoutBtn->addWidget( btn1, 0, 0 );

  KPushButton * btn2 = new KPushButton( &dlg );
  btn2->setObjectName( "pushButton2" );
  btn2->setText(i18n("&Apply"));
  btn2->setDefault( TRUE );
  btn2->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)1, (QSizePolicy::SizeType)0, 0, 0,
                         btn2->sizePolicy().hasHeightForWidth() ) );
  layoutBtn->addWidget( btn2, 0, 1 );

  layout->addMultiCellLayout( layoutBtn, 2, 2, 0, 1 );
  // --END-- building a dynamic dialog

  //connect the buttons to the QDialog default slots
  connect(btn1, SIGNAL(clicked()), &dlg, SLOT(reject()) );
  connect(btn2, SIGNAL(clicked()), &dlg, SLOT(accept()) );

  //prepare to execute the dialog
  bool bReturn = false;
  //resize the textedits
  if (iWidth > 1) {
    QRect r = dlg.geometry();
    r.setHeight(iBasicHeight + iOneHeight*mapVar2Te.count());
    r.setWidth(iWidth);
    dlg.setGeometry(r);
  }
  if ( i > 0 && // only if there are any variables
    dlg.exec() == QDialog::Accepted ) {

    QMap<QString, KTextEdit *>::Iterator it2;
    for ( it2 = mapVar2Te.begin(); it2 != mapVar2Te.end(); ++it2 ) {
      if (it2.key() == _SnippetConfig.getDelimiter() + _SnippetConfig.getDelimiter())
        continue;
      (*map)[it2.key()] = it2.data()->text();    //copy the entered values back to the given map

      if (mapVar2Cb[it2.key()]->isChecked())     //if the checkbox is on; save the values for later
        (*mapSave)[it2.key()] = it2.data()->text();
      else
        (*mapSave).erase(it2.key());
    }
    bReturn = true;

    iBasicHeight = dlg.geometry().height() - layoutVar->geometry().height();
    iOneHeight = layoutVar->geometry().height() / mapVar2Te.count();
    iWidth = dlg.geometry().width();
  }

  //do some cleanup
  QMap<QString, KTextEdit *>::Iterator it1;
  for (it1 = mapVar2Te.begin(); it1 != mapVar2Te.end(); ++it1)
    delete it1.data();
  mapVar2Te.clear();
  QMap<QString, QCheckBox *>::Iterator it2;
  for (it2 = mapVar2Cb.begin(); it2 != mapVar2Cb.end(); ++it2)
    delete it2.data();
  mapVar2Cb.clear();
  delete layoutTop;
  delete layoutVar;
  delete layoutBtn;
  delete layout;

  if (i==0) //if nothing happened this means, that there are no variables to translate
    return true; //.. so just return OK

  return bReturn;
//  return true;
}


//  fn SnippetWidget::showSingleVarDialog(QString var, QMap<QString, QString> * mapSave)
/*!
    This function constructs a dialog which contains a label and a linedit for the given variable
    It return either the entered value or an empty string if the user hit cancel
 */
QString SnippetWidget::showSingleVarDialog(QString var, QMap<QString, QString> * mapSave, QRect & dlgSize)
{
  // --BEGIN-- building a dynamic dialog
  QDialog dlg(this);
  dlg.setCaption(i18n("Enter Values for Variables"));

  QGridLayout * layout = new QGridLayout( &dlg, 1, 1, 11, 6, "layout");
  QGridLayout * layoutTop = new QGridLayout( 0, 1, 1, 0, 6, "layoutTop");
  QGridLayout * layoutVar = new QGridLayout( 0, 1, 1, 0, 6, "layoutVar");
  QGridLayout * layoutBtn = new QGridLayout( 0, 2, 1, 0, 6, "layoutBtn");

  KTextEdit *te = NULL;
  QLabel * labTop = NULL;
  QCheckBox * cb = NULL;

  labTop = new QLabel( &dlg, "label" );
  layoutTop->addWidget(labTop, 0, 0);
  labTop->setText(i18n("Enter the replacement values for %1:").arg( var ));
  layout->addMultiCellLayout( layoutTop, 0, 0, 0, 1 );


  cb = new QCheckBox( &dlg, "cbVar" );
  cb->setChecked( FALSE );
  cb->setText(i18n( "Make value &default" ));

  te = new KTextEdit( &dlg );
  te->setObjectName( "teVar" );
  layoutVar->addWidget( te, 0, 1, Qt::AlignTop);
  layoutVar->addWidget( cb, 1, 1, Qt::AlignTop);
  if ((*mapSave)[var].length() > 0) {
    cb->setChecked( TRUE );
    te->setText((*mapSave)[var]);
  }

  QToolTip::add( cb, i18n("Enable this to save the value entered to the right as the default value for this variable") );
  QWhatsThis::add( cb, i18n("If you enable this option, the value entered to the right will be saved. "
                            "If you use the same variable later, even in another snippet, the value entered to the right "
                            "will be the default value for that variable.") );

  layout->addMultiCellLayout( layoutVar, 1, 1, 0, 1 );

  KPushButton * btn1 = new KPushButton( &dlg );
  btn1->setObjectName( "pushButton1") ;
  btn1->setText(i18n("&Cancel"));
  layoutBtn->addWidget( btn1, 0, 0 );

  KPushButton * btn2 = new KPushButton( &dlg );
  btn2->setObjectName( "pushButton2") ;
  btn2->setText(i18n("&Apply"));
  btn2->setDefault( TRUE );
  layoutBtn->addWidget( btn2, 0, 1 );

  layout->addMultiCellLayout( layoutBtn, 2, 2, 0, 1 );
  te->setFocus();
  // --END-- building a dynamic dialog

  //connect the buttons to the QDialog default slots
  connect(btn1, SIGNAL(clicked()), &dlg, SLOT(reject()) );
  connect(btn2, SIGNAL(clicked()), &dlg, SLOT(accept()) );

  //execute the dialog
  QString strReturn = "";
  if (dlgSize.isValid())
    dlg.setGeometry(dlgSize);
  if ( dlg.exec() == QDialog::Accepted ) {
    if (cb->isChecked())     //if the checkbox is on; save the values for later
      (*mapSave)[var] = te->text();
    else
      (*mapSave).erase(var);

    strReturn = te->text();    //copy the entered values back the the given map

    dlgSize = dlg.geometry();
  }

  //do some cleanup
  delete cb;
  delete te;
  delete labTop;
  delete btn1;
  delete btn2;
  delete layoutTop;
  delete layoutVar;
  delete layoutBtn;
  delete layout;

  return strReturn;
}


/*!
    Emulate Qt3's selectedItem() because it does exactly what we need here.
*/
QTreeWidgetItem * SnippetWidget::selectedItem() const
{
  if ( selectedItems().isEmpty() )
    return 0;
  return selectedItems().first();
}


//  fn SnippetWidget::acceptDrag (QDropEvent *event) const
/*!
    Reimplementation from QListView.
    Check here if the data the user is about to drop fits our restrictions.
    We only accept dropps of plaintext, because from the dropped text
    we will create a snippet.
 */
bool SnippetWidget::acceptDrag (QDropEvent *event) const
{
  kDebug(5006) << "Format: " << event->format() << "" << event->pos() << endl;

  QTreeWidgetItem * item = itemAt(event->pos());

  if (item &&
      QString(event->format()).startsWith("text/plain") &&
      static_cast<SnippetWidget *>(event->source()) != this) {
    kDebug(5006) << "returning TRUE " << endl;
    return TRUE;
  } else if(item &&
            event->provides("text/x-kmail-textsnippet") &&
            static_cast<SnippetWidget *>(event->source()) != this) {
    kDebug(5006) << "returning TRUE " << endl;
    return TRUE;
  } else {
    kDebug(5006) << "returning FALSE" << endl;
    event->acceptAction(FALSE);
    return FALSE;
  }
}

/*!
    Called on drop.
    Return true if we did use the data, false if we did not.
 */
bool SnippetWidget::dropMimeData( QTreeWidgetItem *parent, int index,
                                  const QMimeData *data, Qt::DropAction action )
{
  QTreeWidgetItem *item = parent->child( index );
  SnippetGroup *group = dynamic_cast<SnippetGroup *>( item );
  if ( !group )
    group = dynamic_cast<SnippetGroup *>( parent );

  if ( !group || !data->hasText() || data->text().isEmpty() )
    return false;

  // fill the dialog with the given data
  SnippetDlg dlg( this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetName->clear();
  dlg.snippetText->setText( data->text() );

  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach ( SnippetItem *const si, _list ) {
    if ( dynamic_cast<SnippetGroup*>( si ) ) {
      dlg.cbGroup->insertItem( si->getName() );
    }
  }
  dlg.cbGroup->setCurrentText(group->getName());

  if (dlg.exec() == QDialog::Accepted) {
    /* get the group that the user selected with the combobox */
    group = dynamic_cast<SnippetGroup*>(SnippetItem::findItemByName(dlg.cbGroup->currentText(), _list));
    _list.append( new SnippetItem(group, dlg.snippetName->text(), dlg.snippetText->text()) );
    return true;
  }
  return false;
}

void SnippetWidget::startDrag( Qt::DropActions supportedActions )
{
  QString text = dynamic_cast<SnippetItem*>( currentItem() )->getText();
  QDrag *drag = new QDrag( this );
  QMimeData *mimeData = new QMimeData();
  mimeData->setData( "text/x-kmail-textsnippet", text.toUtf8() );
  drag->setMimeData( mimeData );
  drag->exec();
}

void SnippetWidget::slotExecute()
{
    slotExecuted(currentItem());    
}

SnippetDlg::SnippetDlg( QWidget *parent )
 : QDialog( parent )
{
  setupUi( this );
}

#include "snippet_widget.moc"
