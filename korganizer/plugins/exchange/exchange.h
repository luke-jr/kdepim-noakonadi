/*
    This file is part of KOrganizer.
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef KORG_EXCHANGE_H
#define KORG_EXCHANGE_H

#include <qstring.h>
#include <kio/job.h>

#include <calendar/plugin.h>
#include <korganizer/part.h>

// #include <klocale.h>
// #include <klibloader.h>
// #include <kfileitem.h>
// #include <kdirlister.h>
// #include <mimelib/string.h>

// #include <mimelib/entity.h>
#include <libkcal/icalformat.h>
#include <libkcal/incidence.h>

class DwString;
class DwEntity;
class ExchangeProgress;

// using namespace KOrg;

class Exchange : public KOrg::Part {
    Q_OBJECT
  public:
    Exchange( KOrg::MainWindow *, const char * );
    ~Exchange();

    QString info();

  private slots:
    void download();
    void upload();
    void configure();
    void test();
    void slotPatchResult( KIO::Job * );
    void slotPropFindResult( KIO::Job * );
    void slotComplete( ExchangeProgress * );
    // void slotDataReq( KIO::Job *, QByteArray& data );
 
    // void slotNewItems(const KFileItemList&);
    void slotSearchEntries( KIO::Job *, const KIO::UDSEntryList& ); 
    void slotSearchResult( KIO::Job *job );
    void slotMasterResult( KIO::Job* job );
    void slotData( KIO::Job *job, const QByteArray &data );
    void slotTransferResult( KIO::Job *job );
    void slotMasterEntries( KIO::Job *, const KIO::UDSEntryList& );

  signals:
    void startDownload();
    void finishDownload();

  protected:
    KURL getBaseURL();
    KURL getCalendarURL();
    
  private:
    void test2();
    void tryExist();
    void handleEntries( const KIO::UDSEntryList &, bool recurrence );
    void handleAppointments( const QDomDocument &, bool recurrence );
    void handleRecurrence( QString uid );
    void handlePart( DwEntity *part );
    
    KURL baseURL;
    KCal::Calendar *calendar;

    KCal::Incidence* m_currentUpload;
    int m_currentUploadNumber;

    QMap<QString,int> m_uids; // This keeps trakc of uids we already covered. Especially useful for
    	// recurring events.
    QMap<QString,DwString *> m_transferJobs; // keys are URLs
    QString putData;
};

#endif
