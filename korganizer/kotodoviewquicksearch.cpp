/*
    This file is part of KOrganizer.

    Copyright (c) 2004 Till Adam <adam@kde.org>
    Copyright (c) 2005 Rafal Rzepecki <divide@users.sourceforge.net>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/


#include "kotodoviewquicksearch.h"

#include "kotodoviewitem.h"
#include "kotodoview.h"

#include <kcal/calendar.h>
#include <kcal/calfilter.h>
#include <libkdepim/categoryhierarchyreader.h>

#include <korganizer/mainwindow.h>

#include "koprefs.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <k3listviewsearchline.h>
#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QApplication>
#include <QRegExp>
#include <QSizePolicy>
#include <QTimer>
#include <kicon.h>
using namespace KPIM;
using namespace KCal;

QAction *KOTodoListViewQuickSearch::action = 0;

KOTodoListViewQuickSearch::KOTodoListViewQuickSearch( QWidget *parent,
                                            QList<K3ListView*> listViews,
                                            KActionCollection *actionCollection,
                                            Calendar *calendar )
  : QToolBar( parent ), mCategoryCombo( 0 ), mCalendar( calendar ),
    mQuickSearchLine( 0 )
{
  /*if ( !action ) {
  action  = new KAction(KIcon(QApplication::isRightToLeft() ? "clear-left" : "locationbar-erase"), i18n("Reset"), this);
  actionCollection->addAction("reset_quicksearch", action );
    connect(action, SIGNAL(triggered(bool) ), SLOT( reset() ));
    action->setWhatsThis( i18n( "Reset Quick Search\n"
                                  "Resets the quick search so that "
                                  "all to-dos are shown again." ) );
  }

  addAction( action );*/

  setMovable( false );
  layout()->setSpacing( KDialog::spacingHint() );

  mQuickSearchLine = new KOTodoListViewQuickSearchLine( this, listViews );
  mQuickSearchLine->setClickMessage( i18n("Search") );
  addWidget( mQuickSearchLine );

  mCategoryLabel = new QLabel( i18n("&Category:"), this );
  mCategoryLabel->setObjectName( "kde toolbar widget" );
  addWidget( mCategoryLabel );

  mCategoryCombo = new QComboBox( this );
  mCategoryCombo->setObjectName( "quick search category combo box" );
  fillCategories();
  addWidget( mCategoryCombo );

  mCategoryCombo->setCurrentIndex( 0 );
  connect( mCategoryCombo, SIGNAL ( activated( int ) ),
           this, SLOT( slotCategoryChanged( int ) ) );

  mCategoryLabel->setBuddy( mCategoryCombo );
}

KOTodoListViewQuickSearch::~KOTodoListViewQuickSearch()
{
}

bool KOTodoListViewQuickSearchLine::itemMatches(const Q3ListViewItem *item,
                                                const QString &s)
const
{
  while ( item ) {
    const Todo *todo = static_cast<const KOTodoViewItem *>( item )->todo();
    if ( ( mCategory.isNull() ||
           todo->categories().indexOf( QRegExp( QString( "^" ) +
           QRegExp::escape( mCategory ) ) ) >= 0 ) &&
           K3ListViewSearchLine::itemMatches(item, s) )
      return true;
    else
      item = item->parent(); // children of passed items also pass
  }
  return false;
}

//-----------------------------------------------------------------------------
void KOTodoListViewQuickSearch::reset()
{
  mQuickSearchLine->clear();
  mCategoryCombo->setCurrentIndex( 0 );
  slotCategoryChanged( 0 );
}

void KOTodoListViewQuickSearch::slotCategoryChanged( int index )
{
  if ( index == 0 )
    mQuickSearchLine->setCategory( QString() );
  else
    mQuickSearchLine->setCategory( categoryList[index - 1] );
  mQuickSearchLine->updateSearch();
}

void KOTodoListViewQuickSearch::fillCategories()
{
  QString current = mCategoryCombo->currentIndex() > 0 ?
    categoryList[mCategoryCombo->currentIndex() - 1] : QString();
  QStringList categories;

  CalFilter *filter = mCalendar->filter();
  if ( filter->criteria() & CalFilter::ShowCategories ) {
    categories = filter->categoryList();
    categories.sort();
  } else {
    categories = KOPrefs::instance()->mCustomCategories;
    QStringList filterCategories = filter->categoryList();
    categories.sort();
    filterCategories.sort();

    QStringList::Iterator it = categories.begin();
    QStringList::Iterator jt = filterCategories.begin();
    while ( it != categories.end() && jt != filterCategories.end() )
      if ( *it == *jt ) {
        it = categories.erase( it );
        jt++;
      } else if ( *it < *jt )
        it++;
      else if ( *it > *jt )
        jt++;
  }

  CategoryHierarchyReaderQComboBox( mCategoryCombo ).read( categories );
  mCategoryCombo->insertItem( 0, i18n( "Any category" ) );

  categoryList.resize( categories.count() );
  qCopy( categories.begin(), categories.end(), categoryList.begin() );

  if ( current.isNull() ) {
    mCategoryCombo->setCurrentIndex( 0 );
  } else {
    for ( int i = 0; i < categoryList.count(); ++i )
      if ( categoryList[i] == current ) {
        mCategoryCombo->setCurrentIndex( i + 1 );
        break;
      }
  }

}

void KOTodoListViewQuickSearch::setCalendar( Calendar *calendar )
{
  mCalendar = calendar;
  mQuickSearchLine->updateSearch();
}

void KOTodoListViewQuickSearch::resizeEvent( QResizeEvent *e )
{
  int w = width() - mCategoryCombo->sizeHint().width()
                  - mCategoryLabel->sizeHint().width();
  int halfw = width() / 2;

  if ( w < halfw ) {
    w += mCategoryLabel->sizeHint().width();
    mCategoryLabel->hide();
  } else
    mCategoryLabel->show();
  if ( w < halfw ) {
    slotCategoryChanged( 0 );
    mCategoryCombo->hide();
  } else {
    slotCategoryChanged( mCategoryCombo->currentIndex() );
    mCategoryCombo->show();
  }

  QToolBar::resizeEvent( e );
}

void KOTodoListViewQuickSearch::showEvent( QShowEvent *e )
{
  connect( action, SIGNAL( activated() ), this, SLOT( reset() ) );

  QToolBar::showEvent( e );
}

void KOTodoListViewQuickSearch::hideEvent( QHideEvent *e )
{
  disconnect( action, SIGNAL( activated() ), this, SLOT( reset() ) );

  QToolBar::hideEvent( e );
}

KOTodoListViewQuickSearchContainer::KOTodoListViewQuickSearchContainer(
                               QWidget *parent,
                               QList<K3ListView*> listViews,
                               KActionCollection *actionCollection,
                               Calendar *calendar)
     : QWidget( parent ), mQuickSearch( new KOTodoListViewQuickSearch(
         this, listViews, actionCollection, calendar ) )
{
  setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
  mQuickSearch->setObjectName( "search toolbar" );
}

KOTodoListViewQuickSearchContainer::~KOTodoListViewQuickSearchContainer()
{
}

QSize KOTodoListViewQuickSearchContainer::sizeHint() const
{
  int width = KDialog::spacingHint();
  QList<QObject*> list = mQuickSearch->children();
  for ( QList<QObject*>::Iterator it = list.begin(); it != list.end(); ++it ) {
    QWidget *child = dynamic_cast<QWidget *>( *it );
    if ( child ) {
      width += child->sizeHint().width() + KDialog::spacingHint();
    }
  }

  return QSize( width, mQuickSearch->sizeHint().height() );
}

QSize KOTodoListViewQuickSearchContainer::minimumSizeHint() const
{
  return QSize( mQuickSearch->iconSize().width() + 
                mQuickSearch->mQuickSearchLine->minimumSizeHint().width() +
                3 * KDialog::spacingHint(),
                mQuickSearch->minimumSizeHint().height() );
}

KOTodoListViewQuickSearch *KOTodoListViewQuickSearchContainer::quickSearch()
                                                                          const
{
  return mQuickSearch;
}

void KOTodoListViewQuickSearchContainer::resizeEvent ( QResizeEvent */*e*/ )
{
  mQuickSearch->setGeometry( QRect( QPoint( 0, 0 ), size() ) );
}


#include "kotodoviewquicksearch.moc"
