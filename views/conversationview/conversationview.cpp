/*
 * conversationview.h
 *
 * copyright (c) Aron Bostrom <Aron.Bostrom at gmail.com>, 2006 
 *
 * this library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "conversationview.h"
#include "conversationdelegate.h"
#include "foldermodel.h"

ConversationView::ConversationView(FolderModel *folderModel, QSortFilterProxyModel *proxyModel, QWidget *parent)
 : QTreeView(parent)
{
  setRootIsDecorated(false);
  setSortingEnabled(true);
  sortByColumn(1);
  cDelegate = new ConversationDelegate(folderModel, proxyModel);
  setItemDelegate(cDelegate);
  header()->setDefaultAlignment(Qt::AlignLeft);
  header()->setSortIndicator(1, Qt::DescendingOrder);
  header()->setSortIndicatorShown(true);
  header()->setClickable(true);
  header()->resizeSection(0, 185);
//  QObject::connect(header(), SIGNAL(sectionClicked(int)), this, SLOT(swapSort(int)));
}


ConversationView::~ConversationView()
{
}

void ConversationView::updateWidth(int width, int /*nouse*/)
{
  cDelegate->setWidth(width);
}

// void ConversationView::swapSort(int column)
// {
//   cDelegate->pmodel->sort(column, Qt::DescendingOrder);
// }

#include "conversationview.moc"
