#include <qradiobutton.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qlabel.h>

#include <kdebug.h>
#include <kprinter.h>
#include <klocale.h>
#include <kdialog.h>

#include "printingwizard.h"
#include "printstyle.h"
#include "detailledstyle.h"
#include "mikesstyle.h"

namespace KABPrinting {

    PrintingWizardImpl::PrintingWizardImpl(KPrinter *printer,
                                           KABC::AddressBook* doc,
                                           const QStringList& selection,
                                           QWidget *parent,
                                           const char* name)
        : PrintingWizard(printer, doc, selection, parent, name),
          style(0)
    {
        mBasicPage=new BasicPage(this);
        mBasicPage->rbSelection->setEnabled(!selection.isEmpty());
        connect(mBasicPage->cbStyle, SIGNAL(activated(int)),
                SLOT(slotStyleSelected(int)));
        addPage(mBasicPage, i18n("General"));
        setAppropriate(mBasicPage, true);
        registerStyles();
        if(mBasicPage->cbStyle->count()>0)
        {
            slotStyleSelected(0);
        }
    }

    PrintingWizardImpl::~PrintingWizardImpl()
    {
    }

    void PrintingWizardImpl::registerStyles()
    {
        styleFactories.append(new DetailledPrintStyleFactory(this));
        styleFactories.append(new MikesStyleFactory(this));

        mBasicPage->cbStyle->clear();
        for(unsigned int i=0; i<styleFactories.count(); ++i)
        {
            mBasicPage->cbStyle->insertItem(styleFactories.at(i)->description());
        }
    }

    void PrintingWizardImpl::slotStyleSelected(int index)
    {
        if(index>=0 && index<mBasicPage->cbStyle->count())
        {
            if(style!=0)
            {
                delete style;
                style=0;
            }
            PrintStyleFactory *factory=styleFactories.at(index);
            kdDebug() << "PrintingWizardImpl::slotStyleSelected: "
                      << "creating print style "
                      << factory->description() << endl;
            style=factory->create();
        }
        const QPixmap& preview=style->preview();
        mBasicPage->plPreview->setPixmap(preview); // reset it if it is Null
        if(preview.isNull())
        {
            mBasicPage->plPreview->setText(i18n("(No Preview available.)"));
        }
        setFinishEnabled(mBasicPage, style!=0);
    }

    KABC::AddressBook* PrintingWizardImpl::document()
    {
        return mDocument;
    }

    KPrinter* PrintingWizardImpl::printer()
    {
        return mPrinter;
    }

    // WORK_TO_DO: select contacts for printing
    void PrintingWizardImpl::print()
    {
        QStringList contacts;
        if(style!=0)
        {
            if(mBasicPage->rbSelection->isChecked())
            {
                contacts=mSelection;
            } else {
                // create a string list of all entries:
                KABC::AddressBook::Iterator iter;
                for(iter=document()->begin(); iter!=document()->end(); ++iter)
                {
                    contacts << (*iter).uid();
                }
            }
        }
        kdDebug() << "MikesStyle::print: printing "
                  << contacts.count() << " contacts." << endl;
        // ... print:
        style->print(contacts);
    }

}

#include "printingwizard.moc"
