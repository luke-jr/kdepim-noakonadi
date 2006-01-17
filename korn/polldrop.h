/*
* polldrop.h -- Declaration of class KPollableDrop.
* Generated by newclass on Sun Nov 30 22:41:49 EST 1997.
*/
#ifndef SSK_POLLDROP_H
#define SSK_POLLDROP_H

/**
 * @file
 *
 * This file defines the class KPollableDrop
 */

#include"maildrop.h"

class QTimerEvent;

/**
* Superclass for all pollable maildrop monitors.
* 
* To implement a polling maildrop, reimplement recheck and emit
* changed(int) in recheck if new messages have been received.
*
* @author Sirtaj Singh Kang (taj@kde.org)
* @version $Id$
*/
class KPollableDrop : public KMailDrop
{
	Q_OBJECT
public:
	/**
	 * This is the key such as it is used in the configuration file.
	 */
	static const char *PollConfigKey;
	/**
	 * The default interval between two checks
	 */
	static const int DefaultPoll;

private:
	int _freq;
	int _timerId;
	bool _timerRunning;

public:
	/**
	* KPollableDrop Constructor
	*/
	KPollableDrop();

	/**
	 * Starts the monitor.
	 *
	 * @return in this class, it always returns false
	 */
	virtual bool startMonitor();
	/**
	 * Stops the monitor.
	 *
	 * @return in this class, it always returns false
	 */
	virtual bool stopMonitor();
	
	/**
	 * Returns true if the box is running; false otherwise.
	 *
	 * @return true if the monitor is running; false otherwise
	 */
	virtual bool running() { return _timerRunning; };

	/**
	 * Returns the frequency of this monitor.
	 * The frequency is the interval between two checks.
	 *
	 * @return the frequency of this monitor
	 */
	int freq() const { return _freq; }
	/**
	 * Sets the frequency of this monitor.
	 * The frequency is the interval between two checks.
	 *
	 * @param freq the new frequency
	 */
	void setFreq( int freq );

	/**
	 * This function reads the configuration cfg.
	 * Childs classes should reimplement this method and call this one
	 * if they also have configuration.
	 *
	 * @param cfg the configuration data
	 * @return true if succesfull, false otherwise
	 */
	virtual bool readConfigGroup ( const KConfigBase& cfg );
	/**
	 * This function writes the configuration cfg.
	 * Childs classes should reimplement this method and call this one
	 * if they also have configuration.
	 *
	 * @param cfg the configuration data
	 * @return true if succesfull, false otherwise
	 */
	virtual bool writeConfigGroup ( KConfigBase& cfg ) const;

	//virtual void addConfigPage( KDropCfgDialog * );

protected:
	/**
	 * This function is used to determine when it is time to recheck again.
	 *
	 * @param ev the timerevent which is used to find out if this timerevent belongs to this class
	 */
	void timerEvent( QTimerEvent *ev );
};

inline void KPollableDrop::setFreq( int freq ) 
{  
	bool r = running();

	if( r ) stopMonitor();

	_freq = freq; 
 
	if( r ) startMonitor(); 
}

#endif // SSK_POLLDROP_H
