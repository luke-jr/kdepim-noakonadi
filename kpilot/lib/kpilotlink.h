#ifndef _KPILOT_KPILOTLINK_H
#define _KPILOT_KPILOTLINK_H
/* kpilotlink.h			KPilot
**
** Copyright (C) 1998-2001 by Dan Pilone
**
** This is a horrible class that implements three different
** functionalities and that should be split up as soon as 2.1 
** is released. See the .cc file for more.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, 
** MA 02139, USA.
*/

/*
** Bug reports and questions can be sent to kde-pim@kde.org
*/

#include <time.h>
#include <pi-dlp.h>

#ifndef QOBJECT_H
#include <qobject.h>
#endif


class QTimer;
class QSocketNotifier;
class KPilotUser;
struct DBInfo;

/*
** The KPilotLink class was originally a kind of C++ wrapper
** for the pilot-link library. It grew and grew and mutated
** until it was finally cleaned up again in 2001. In the meantime
** it had become something that wrapped a lot more than just
** pilot-link. This class currently does:
** 
** * Client (ie. conduit) handling of kpilotlink protocol connections
** * Pilot-link handling
**
** Which is exactly what is needed: something that conduits can
** plug onto to talk to the pilot.
*/

/*
** The struct db is a description class for Pilot databases
** by Kenneth Albanowski. It's not really clear why it's *here*.
** The macro pi_mktag is meant to be given four char (8-bit)
** quantities, which are arranged into an unsigned long; for example
** pi_mktag('l','n','c','h'). This is called the creator tag
** of a database, and db.creator can be compared with such a
** tag. The tag lnch is used by the Pilot's launcher app. Some
** parts of KPilot require such a tag.
*/
struct db 
{
	char name[256];
	int flags;
	unsigned long creator;
	unsigned long type;
	int maxblock;
};

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))


class KPilotDeviceLink : public QObject
{
friend class SyncAction;
Q_OBJECT

/*
** Constructors and destructors.
*/
protected:
	/**
	* Creates a pilot link that can sync to the pilot.
	*
	* Call reset() on it to start looking for a device.
	*/
	KPilotDeviceLink(QObject *parent, const char *name);
private:
	static KPilotDeviceLink *fDeviceLink;

public:
	virtual ~KPilotDeviceLink();
	static KPilotDeviceLink *link() { return fDeviceLink; } ;
	static KPilotDeviceLink *init(QObject *parent=0L,const char *n=0L);

/*
** Status information
*/

public:
	/**
	* The link behaves like a state machine most of the time:
	* it waits for the actual device to become available, and
	* then becomes ready to handle syncing.
	*/
	typedef enum {
		Init,
		WaitingForDevice,
		FoundDevice,
		CreatedSocket,
		DeviceOpen,
		AcceptedDevice,
		SyncDone,
		PilotLinkError
		} LinkStatus;

	LinkStatus status() const { return fStatus; } ;
	virtual QString statusString() const;

	/**
	* True if HotSync has been started but not finished yet
	* (ie. the physical Pilot is waiting for sync commands)
	*/
	bool getConnected() const { return fStatus == AcceptedDevice; }

private:
	LinkStatus fStatus;


/*
** Used for initially attaching to the device.
** deviceReady() is emitted when the device has been opened
** and a Sync can start.
*/
public:
	/**
	* Information on what kind of device we're dealing with.
	*/
	typedef enum { None,
		Serial,
		OldStyleUSB,
		DevFSUSB
		} DeviceType;

	DeviceType deviceType() const { return fDeviceType; } ;
	QString deviceTypeString(int i) const;
	bool isTransient() const 
	{ 
		return (fDeviceType==OldStyleUSB) ||
			(fDeviceType==DevFSUSB);
	}

	QString pilotPath() const { return fPilotPath; } ;

	/**
	* Return the device link to the Init state and try connecting
	* to the given device path (if it's non-empty).
	*/
	void reset(DeviceType t,const QString &pilotPath = QString::null);


public slots:
	/**
	* Release all resources, including the master pilot socket,
	* timers, notifiers, etc.
	*/
	void close();

	/**
	* Assuming things have been set up at least once already by
	* a call to reset() with parameters, use this slot to re-start
	* with the same settings.
	*/
	void reset();

protected slots:
	/**
	* Attempt to open the device. Called regularly to check
	* if the device exists (to handle USB-style devices).
	*/
	void openDevice();

	/**
	* Called when the device is opened *and* activity occurs on the
	* device. This indicates the beginning of a hotsync.
	*/
	void acceptDevice();

protected:
	/**
	* Does the low-level opening of the device and handles the
	* pilot-link library initialisation.
	*/
	bool open();

signals:
	/**
	* Emitted once the user information has been read and
	* the HotSync is really ready to go.
	*/
	void deviceReady();

protected:
	int pilotSocket() const { return fCurrentPilotSocket; } ;


private:
	/**
	* Path of the device special file that will be used.
	* Usually /dev/pilot, /dev/ttySx, or /dev/usb/x.
	*/
	QString fPilotPath;

	/**
	* What kind of device is this?
	*/
	DeviceType fDeviceType;

	/**
	* For transient devices: how often have we tried pi_bind()?
	*/
	int fRetries;

	/**
	* Timers and Notifiers for detecting activity on the device.
	*/
	QTimer *fOpenTimer;
	QSocketNotifier *fSocketNotifier;

	/**
	* Pilot-link library handles for the device once it's opened.
	*/
	int fPilotMasterSocket;
	int fCurrentPilotSocket;

signals:
	/**
	* Whenever a conduit adds a Sync log entry (actually,
	* KPilotLink itself adds some log entries itself),
	* this signal is emitted.
	*/
	void logEntry(const char *);

/*
** File installation.
*/
public:
	int installFiles(const QStringList &);
protected:
	bool installFile(const QString &);

 	/**
 	* Write a log entry to the pilot. Note that the library
 	* function takes a char *, not const char * (which is
 	* highly dubious). Causes signal logEntry(const char *)
 	* to be emitted.
 	*/
 	void addSyncLogEntry(const QString &entry,bool suppress=false);

signals:
 	/**
 	* Whenever a conduit adds a Sync log entry (actually,
 	* KPilotLink itself adds some log entries itself),
	* this signal is emitted.
	*/
 	void logMessage(const QString &);
 	void logError(const QString &);
 	void logProgress(const QString &, int);


/*
** Pilot User Identity functions.
*/
public:
	/**
	* Returns the user information as set in the KPilot settings dialog.
	* The user information can also be set by the Pilot, and at the
	* end of a HotSync the two user informations can be synced as well
	* with finishSync -- this writes fPilotUser again, so don't make
	* local copies of the KPilotUser structure and modify them.
	*/
	KPilotUser *getPilotUser() { return fPilotUser; }
	void finishSync();

protected:
	KPilotUser  *fPilotUser;

/*
** Actions intended just to abstract away the pilot-link library interface.
*/
protected:
	/**
	* Notify the Pilot user which conduit is running now.
	*/
	int openConduit();
public:
	int getNextDatabase(int index,struct DBInfo *);
	int findDatabase(char*name, struct DBInfo*);

	/**
	* Retrieve the database indicated by DBInfo *db into the
	* local file @p path.
	*/
	bool retrieveDatabase(const QString &path, struct DBInfo *db);
} ;

bool operator < ( const struct db &, const struct db &) ;


// $Log$
// Revision 1.4  2002/04/07 20:19:48  cschumac
// Compile fixes.
//
// Revision 1.3  2002/02/02 11:46:03  adridg
// Abstracting away pilot-link stuff
//
// Revision 1.2  2002/01/21 23:14:03  adridg
// Old code removed; extra abstractions added; utility extended
//
// Revision 1.1  2001/10/08 21:56:02  adridg
// Start of making a separate KPilot lib
//
// Revision 1.34  2001/09/29 16:26:18  adridg
// The big layout change
//
// Revision 1.33  2001/09/24 19:46:17  adridg
// Made exec() pure virtual for SyncActions, since that makes more sense than having an empty default action.
//
// Revision 1.32  2001/09/23 18:24:59  adridg
// New syncing architecture
//
// Revision 1.31  2001/09/16 12:24:54  adridg
// Added sensible subclasses of KPilotLink, some USB support added.
//
// Revision 1.30  2001/09/07 20:46:40  adridg
// Cleaned up some methods
//
// Revision 1.29  2001/09/06 22:04:27  adridg
// Enforce singleton-ness & retry pi_bind()
//
// Revision 1.28  2001/09/05 21:53:51  adridg
// Major cleanup and architectural changes. New applications kpilotTest
// and kpilotConfig are not installed by default but can be used to test
// the codebase. Note that nothing else will actually compile right now.
//
#endif
