/*
    This file is part of Akregator.

    Copyright (C) 2004 Teemu Rytilahti <tpr@d5k.net>
                  2005 Frank Osterfeld <frank.osterfeld at kdemail.net>

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

#include <kaction.h>
#include <kapplication.h>
#include <kfiledialog.h>
#include <khtmlview.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <krun.h>
#include <kshell.h>
#include <ktoolinvocation.h>
#include <kurl.h>
#include <kparts/browserextension.h>
#include <kparts/browserrun.h>

#include <QClipboard>
#include <QPaintDevice>

#include "viewer.h"
#include "akregatorconfig.h"

namespace Akregator {

Viewer::Viewer(QWidget *parent, const char *name)
    : KHTMLPart(parent, name), m_url(0)
{
    setZoomFactor(100);
    setJScriptEnabled(true);
    setJavaEnabled(true);
    setMetaRefreshEnabled(true);
    setPluginsEnabled(true);
    setDNDEnabled(true);
    setAutoloadImages(true);
    setStatusMessagesEnabled(true);

    // change the cursor when loading stuff...
    connect( this, SIGNAL(started(KIO::Job *)),
             this, SLOT(slotStarted(KIO::Job *)));
    connect( this, SIGNAL(completed()),
             this, SLOT(slotCompleted()));

    connect( browserExtension(), SIGNAL(popupMenu (KXMLGUIClient*, const QPoint&, const KUrl&, const KParts::URLArgs&, KParts::BrowserExtension::PopupFlags, mode_t)), this, SLOT(slotPopupMenu(KXMLGUIClient*, const QPoint&, const KUrl&, const KParts::URLArgs&, KParts::BrowserExtension::PopupFlags, mode_t)));

    KStdAction::print(this, SLOT(slotPrint()), actionCollection(), "viewer_print");
    KStdAction::copy(this, SLOT(slotCopy()), actionCollection(), "viewer_copy");

    KAction *action = new KAction(KIcon("viewmag+"),  i18n("&Increase Font Sizes"), actionCollection(), "incFontSizes" );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotZoomIn()));
    action->setShortcut(KShortcut( "Ctrl+Plus" ));
    action = new KAction(KIcon("viewmag-"),  i18n("&Decrease Font Sizes"), actionCollection(), "decFontSizes" );
    connect(action, SIGNAL(triggered(bool)), SLOT(slotZoomOut()));
    action->setShortcut(KShortcut( "Ctrl+Minus" ));

    connect(this, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()));

    connect( browserExtension(), SIGNAL(openURLRequestDelayed(const KUrl&, const KParts::URLArgs&)), this, SLOT(slotOpenURLRequest(const KUrl&, const KParts::URLArgs& )) );

    new KAction(i18n("Copy &Link Address"), "", 0,
                                 this, SLOT(slotCopyLinkAddress()),
                                 actionCollection(), "copylinkaddress");
    new KAction(i18n("&Save Link As..."), "", 0,
                                 this, SLOT(slotSaveLinkAs()),
                                 actionCollection(), "savelinkas");
}

Viewer::~Viewer()
{}

bool Viewer::closeURL()
{
    emit browserExtension()->loadingProgress(-1);
    emit canceled(QString::null);
    return KHTMLPart::closeURL();
}

int Viewer::pointsToPixel(int pointSize) const
{
    return ( pointSize * view()->logicalDpiY() + 36 ) / 72 ;
}

void Viewer::displayInExternalBrowser(const KUrl &url, const QString &mimetype)
{
   if (!url.isValid()) return;
   if (Settings::externalBrowserUseKdeDefault())
   {
       if (mimetype.isEmpty())
           KToolInvocation::invokeBrowser(url.url(), "0");
       else
           KRun::runURL(url, mimetype, false, false);
   }
   else
   {
       QString cmd = Settings::externalBrowserCustomCommand();
       QString urlStr = url.url();
       cmd.replace(QRegExp("%u"), urlStr);
       KProcess *proc = new KProcess;
       QStringList cmdAndArgs = KShell::splitArgs(cmd);
       *proc << cmdAndArgs;
       proc->start(KProcess::DontCare);
       delete proc;
   }
}

void Viewer::slotOpenURLRequest(const KUrl& /*url*/, const KParts::URLArgs& /*args*/)
{

}

void Viewer::urlSelected(const QString &url, int button, int state, const QString &_target, KParts::URLArgs args)
{
    m_url = completeURL(url);
    browserExtension()->setURLArgs(args);
    if (button == Qt::LeftButton)
    {
        switch (Settings::lMBBehaviour())
        {
            case Settings::EnumLMBBehaviour::OpenInExternalBrowser:
                slotOpenLinkInBrowser();
                break;
            case Settings::EnumLMBBehaviour::OpenInBackground:
                slotOpenLinkInBackgroundTab();
                break;
            default:
                slotOpenLinkInForegroundTab();
                break;
        }
        return;
    }
    else if (button == Qt::MidButton)
    {
        switch (Settings::mMBBehaviour())
        {
            case Settings::EnumMMBBehaviour::OpenInExternalBrowser:
                slotOpenLinkInBrowser();
                break;
            case Settings::EnumMMBBehaviour::OpenInBackground:
                slotOpenLinkInBackgroundTab();
                break;
            default:
                slotOpenLinkInForegroundTab();
                break;
        }
        return;
    }
    KHTMLPart::urlSelected(url,button,state,_target,args);
}

void Viewer::slotPopupMenu(KXMLGUIClient*, const QPoint& p, const KUrl& kurl, const KParts::URLArgs&, KParts::BrowserExtension::PopupFlags kpf, mode_t)
{
   const bool isLink = (kpf & KParts::BrowserExtension::ShowNavigationItems) == 0;
   const bool isSelection = (kpf & KParts::BrowserExtension::ShowTextSelectionItems) != 0;

   QString url = kurl.url();

   m_url = url;
   KMenu popup;

   if (isLink && !isSelection)
   {
        popup.insertItem(SmallIcon("tab_new"), i18n("Open Link in New &Tab"), this, SLOT(slotOpenLinkInForegroundTab()));
        popup.insertItem(SmallIcon("window_new"), i18n("Open Link in External &Browser"), this, SLOT(slotOpenLinkInBrowser()));
        popup.addSeparator();
        popup.addAction( action("savelinkas") );
        popup.addAction( action("copylinkaddress") );
   }
   else
   {
       if (isSelection)
       {
            popup.addAction( action("viewer_copy") );
            popup.addSeparator();
       }
       popup.addAction( action("viewer_print") );
       //KAction *ac = action("setEncoding");
       //if (ac)
       //     ac->plug(&popup);
   }
   popup.exec(p);
}

// taken from KDevelop
void Viewer::slotCopy()
{
    QString text = selectedText();
    text.replace( QChar( 0xa0 ), ' ' );
    QClipboard *cb = QApplication::clipboard();
    disconnect( cb, SIGNAL( selectionChanged() ), this, SLOT( slotClearSelection() ) );
    cb->setText(text);
    connect( cb, SIGNAL( selectionChanged() ), this, SLOT( slotClearSelection() ) );
}

void Viewer::slotCopyLinkAddress()
{
   if(m_url.isEmpty()) return;
   QClipboard *cb = QApplication::clipboard();
   cb->setText(m_url.prettyURL(), QClipboard::Clipboard);
   cb->setText(m_url.prettyURL(), QClipboard::Selection);
}

void Viewer::slotSelectionChanged()
{
    action("viewer_copy")->setEnabled(!selectedText().isEmpty());
}

void Viewer::slotOpenLinkInternal()
{
   openURL(m_url);
}

void Viewer::slotOpenLinkInForegroundTab()
{
    emit urlClicked(m_url, false);
}

void Viewer::slotOpenLinkInBackgroundTab()
{
    emit urlClicked(m_url, true);
}

void Viewer::slotOpenLinkInBrowser()
{
    displayInExternalBrowser(m_url, QString::null);
}

void Viewer::slotSaveLinkAs()
{
    KUrl tmp( m_url );

    if ( tmp.fileName(false).isEmpty() )
        tmp.setFileName( "index.html" );
    KParts::BrowserRun::simpleSave(tmp, tmp.fileName());
}

void Viewer::slotStarted(KIO::Job *)
{
   widget()->setCursor( Qt::WaitCursor );
}

void Viewer::slotCompleted()
{
   widget()->unsetCursor();
}

void Viewer::slotScrollUp()
{
    view()->scrollBy(0,-10);
}

void Viewer::slotScrollDown()
{
    view()->scrollBy(0,10);
}

void Viewer::slotZoomIn()
{
    int zf = zoomFactor();
    if (zf < 100)
    {
        zf = zf - (zf % 20) + 20;
        setZoomFactor(zf);
    }
    else
    {
        zf = zf - (zf % 50) + 50;
        setZoomFactor(zf < 300 ? zf : 300);
    }
}

void Viewer::slotZoomOut()
{
    int zf = zoomFactor();
    if (zf <= 100)
    {
        zf = zf - (zf % 20) - 20;
        setZoomFactor(zf > 20 ? zf : 20);
    }
    else
    {
        zf = zf - (zf % 50) - 50;
        setZoomFactor(zf);
    }
}

void Viewer::slotSetZoomFactor(int percent)
{
    setZoomFactor(percent);
}

// some code taken from KDevelop (lib/widgets/kdevhtmlpart.cpp)
void Viewer::slotPrint( )
{
    view()->print();
}


void Viewer::setSafeMode()
{
    setJScriptEnabled(false);
    setJavaEnabled(false);
    setMetaRefreshEnabled(false);
    setPluginsEnabled(false);
    setDNDEnabled(true);
    setAutoloadImages(true);
    setStatusMessagesEnabled(false);
}

} // namespace Akregator

#include "viewer.moc"

// vim: set et ts=4 sts=4 sw=4:
