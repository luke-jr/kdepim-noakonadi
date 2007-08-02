/*
    This file is part of KAddressBook.
    Copyright (c) 1996-2002 Mirko Boehm <mirko@kde.org>
                            Tobias Koenig <tokoe@kde.org>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <QComboBox>
#include <q3header.h>
#include <QLabel>
#include <QLayout>
#include <q3listview.h>
#include <QPixmap>
#include <QPushButton>
#include <QRadioButton>

#include <kabc/addresseelist.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kprinter.h>
#include <kglobal.h>

// including the styles
#include "detailledstyle.h"
#include "mikesstyle.h"
#include "ringbinderstyle.h"

#include "kabprefs.h"
#include "printprogress.h"
#include "printstyle.h"
#include "printsortmode.h"

#include "printingwizard.h"

using namespace KABPrinting;

PrintingWizard::PrintingWizard( KPrinter *printer, KABC::AddressBook* ab,
                                const QStringList& selection, QWidget *parent,
                                const char* name )
  : K3Wizard( parent, name ), mPrinter( printer ), mAddressBook( ab ),
    mSelection( selection ), mStyle( 0 )
{
  mSelectionPage = new SelectionPage( this );
  mSelectionPage->setUseSelection( !selection.isEmpty() );
  insertPage( mSelectionPage, i18n("Choose Contacts to Print"), -1 );

  mFilters = Filter::restore( KGlobal::config().data(), "Filter" );
  QStringList filters;
  for ( Filter::List::ConstIterator it = mFilters.begin(); it != mFilters.end(); ++it )
    filters.append( (*it).name() );

  mSelectionPage->setFilters( filters );

  mSelectionPage->setCategories( KABPrefs::instance()->customCategories() );

  setAppropriate( mSelectionPage, true );


  mStylePage = new StylePage( mAddressBook, this );
  connect( mStylePage, SIGNAL( styleChanged(int) ), SLOT( slotStyleSelected(int) ) );
  insertPage( mStylePage, i18n("Choose Printing Style"), -1 );

  registerStyles();

  if ( mStyleFactories.count() > 0 )
    slotStyleSelected( 0 );
}

PrintingWizard::~PrintingWizard()
{
}

void PrintingWizard::accept()
{
  print();
  close();
}

void PrintingWizard::registerStyles()
{
  mStyleFactories.append( new DetailledPrintStyleFactory( this ) );
  mStyleFactories.append( new MikesStyleFactory( this ) );
  mStyleFactories.append( new RingBinderPrintStyleFactory( this ) );

  mStylePage->clearStyleNames();
  for ( int i = 0; i < mStyleFactories.count(); ++i )
    mStylePage->addStyleName( mStyleFactories.at( i )->description() );
}

void PrintingWizard::slotStyleSelected( int index )
{
  if ( index < 0 || index >= mStyleFactories.count() )
    return;

  setFinishEnabled( mStylePage, false );

  if ( mStyle )
    mStyle->hidePages();

  if ( mStyleList.at( index ) != 0 )
    mStyle = mStyleList.at( index );
  else {
    PrintStyleFactory *factory = mStyleFactories.at( index );
    kDebug(5720) <<"PrintingWizardImpl::slotStyleSelected:"
                  << "creating print style"
                  << factory->description();
    mStyle = factory->create();
    mStyleList.insert( index, mStyle );
  }

  mStyle->showPages();

  mStylePage->setPreview( mStyle->preview() );

  setFinishEnabled( page( pageCount() - 1 ), true );

  if ( mStyle->preferredSortField() != 0 ) {
    mStylePage->setSortField( mStyle->preferredSortField() );
    mStylePage->setSortAscending( mStyle->preferredSortType() );
  }
}

KABC::AddressBook* PrintingWizard::addressBook()
{
  return mAddressBook;
}

KPrinter* PrintingWizard::printer()
{
  return mPrinter;
}

void PrintingWizard::print()
{
  // create and show print progress widget:
  PrintProgress *progress = new PrintProgress( this );
  insertPage( progress, i18n( "Print Progress" ), -1 );
  showPage( progress );
  kapp->processEvents();

  // prepare list of contacts to print:

  KABC::AddresseeList list;
  if ( mStyle != 0 ) {
    if ( mSelectionPage->useSelection() ) {
      QStringList::ConstIterator it;
      for ( it = mSelection.begin(); it != mSelection.end(); ++it ) {
        KABC::Addressee addr = addressBook()->findByUid( *it );
        if ( !addr.isEmpty() )
          list.append( addr );
      }
    } else if ( mSelectionPage->useFilters() ) {
      // find contacts that can pass selected filter
      Filter::List::ConstIterator filterIt;
      for ( filterIt = mFilters.begin(); filterIt != mFilters.end(); ++filterIt )
        if ( (*filterIt).name() == mSelectionPage->filter() )
          break;

      KABC::AddressBook::ConstIterator it;
      for ( it = addressBook()->begin(); it != addressBook()->end(); ++it ) {
        if ( (*filterIt).filterAddressee( *it ) )
          list.append( *it );
      }

    } else if ( mSelectionPage->useCategories() ) {
      QStringList categories = mSelectionPage->categories();
      KABC::AddressBook::ConstIterator it;
      for ( it = addressBook()->begin(); it != addressBook()->end(); ++it ) {
        const QStringList tmp( (*it).categories() );
        QStringList::ConstIterator tmpIt;
        for ( tmpIt = tmp.begin(); tmpIt != tmp.end(); ++tmpIt )
          if ( categories.contains( *tmpIt ) ) {
            list.append( *it );
            break;
          }
      }
    } else {
      // create a string list of all entries:
      KABC::AddressBook::ConstIterator it;
      for ( it = addressBook()->begin(); it != addressBook()->end(); ++it )
        list.append( *it );
    }

    list.setReverseSorting( !mStylePage->sortAscending() );

    PrintSortMode sortMode( mStylePage->sortField() );
    list.sortByMode( &sortMode );
  }

  kDebug(5720) <<"PrintingWizardImpl::print: printing"
                << list.count() << "contacts.";

  // ... print:
  setBackEnabled( progress, false );
  cancelButton()->setEnabled( false );
  mStyle->print( list, progress );
}

#include "printingwizard.moc"
