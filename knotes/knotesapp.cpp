

#include "knotesapp.h"
#include "knoteconfigdlg.h"

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdir.h>
#include <iostream.h>


KNotesApp::	KNotesApp()
	: KDockWindow()
{
	first_instance = true;
	
	//create the dock widget....
	setPixmap( KGlobal::iconLoader()->loadIcon( "knotes", KIcon::Desktop ) );
	KPopupMenu* menu = contextMenu();
	menu->insertItem( i18n("New Note"), this, SLOT(slotNewNote(int)) );
	menu->insertItem( i18n("Preferences..."), this, SLOT(slotPreferences(int)) );		
	
 	//initialize saved notes, if none create a note...
	QString str_notedir = KGlobal::dirs()->saveLocation( "appdata", "notes/" );
 	QDir notedir( str_notedir );
 	QStringList notes = notedir.entryList( "*" );

 	int count = 0;
 	for( QStringList::Iterator i = notes.begin(); i != notes.end(); ++i )
 	{
		if( *i != "." && *i != ".." ) //ignore these
 		{
   			KConfig* tmp = new KConfig( notedir.absFilePath( *i ) );
   			KNote* tmpnote = new KNote( tmp );
   			m_NoteList[*i] = tmpnote;
   			tmpnote->show();
			++count;
		}
	}
	
	if( count == 0 )
		slotNewNote();
}


KNotesApp::~KNotesApp()
{
}

void KNotesApp::slotNewNote( int id )
{
	QString globalConfigFile = KGlobal::dirs()->findResource( "config", "knotesrc" );
	QString datadir = KGlobal::dirs()->saveLocation( "appdata", "notes/" );
		
    //find a new appropriate id for the new note...
    bool exists;
    QString thename;
    QDir appdir( datadir );
    for( int i = 1; i < 51; i++ )   //set the unjust limit to 50 notes...
    {
        thename = QString( "KNote %1" ).arg(i);
        exists = false;

        if( !appdir.exists( thename ) )
		{
			exists = false;
			break;
		}
    }

    if( exists )
    {
        QString msg = i18n(""
            "You have exeeded the arbitrary and unjustly set limit of 50 knotes.\n"
            "Please complain to the author.");
        KMessageBox::sorry( NULL, msg );
        return;
    }

    //copy the default config file to $KDEHOME/share/apps/knotes2/notes/filename
    QFile gconfigfile( globalConfigFile );
    gconfigfile.open( IO_ReadOnly );

    QFile nconfigfile( datadir + thename );
    nconfigfile.open( IO_WriteOnly );

    QTextStream input( &gconfigfile );
    QTextStream output( &nconfigfile );

    for( QString curr = input.readLine(); curr != QString::null; curr = input.readLine() )
		output << curr << endl;

    gconfigfile.close();
    nconfigfile.close();


    //then create a new KConfig object for the new note, so it can save it's own data
    KConfig* newconfig = new KConfig( datadir + thename );
    newconfig->setGroup( "Data" );
    newconfig->writeEntry("name", thename );
    newconfig->sync();

	KNote* newnote = new KNote( newconfig );
	
	connect( newnote, SIGNAL( sigRenamed(QString&, QString&) ),
	         this,    SLOT( slotNoteRenamed(QString&, QString&) ) );
	connect( newnote, SIGNAL( sigNewNote(int) ),
	         this,    SLOT( slotNewNote(int) ) );	
	connect( newnote, SIGNAL( sigClosed(QString&) ),
	         this,    SLOT( slotNoteClosed(QString&) ) );
	
	m_NoteList[thename] = newnote;
	newnote->show();	
}

void KNotesApp::slotNoteRenamed( QString& oldname, QString& newname )
{
	KNote* tmp = m_NoteList[oldname];
	m_NoteList[newname] = tmp;
	m_NoteList.remove( oldname );
}

void KNotesApp::slotNoteClosed( QString& name )
{
	m_NoteList.remove( name );
}

void KNotesApp::slotPreferences( int id )
{
	//launch preferences dialog...
	QString globalConfigFile = KGlobal::dirs()->findResource( "config", "knotesrc" );
	KConfig* gconfig = new KConfig( globalConfigFile );
	KNoteConfigDlg* tmpconfig = new KNoteConfigDlg( gconfig );
	tmpconfig->show();
}
