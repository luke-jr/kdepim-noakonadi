/* 	$Id$	 */

#ifndef _CALENDARLOCAL_H
#define _CALENDARLOCAL_H

#include <qintdict.h>

#include "calobject.h"

#define BIGPRIME 1031 /* should allow for at least 4 appointments 365 days/yr
			 to be almost instantly fast. */

/**
  * This is the main "calendar" object class for KOrganizer.  It holds
  * information like all appointments/events, user information, etc. etc.
  * one calendar is associated with each TopWidget (@see topwidget.h).
  *
  * @short class providing in interface to a calendar
  * @author Preston Brown
  * @version $Revision$
  */
class CalendarLocal : public CalObject {
    Q_OBJECT
  public:
    /** constructs a new calendar, with variables initialized to sane values. */
    CalendarLocal();
    virtual ~CalendarLocal();
  
    /** loads a calendar on disk in vCalendar format into the current calendar.
     * any information already present is lost. Returns TRUE if successful,
     * else returns FALSE.
     * @param fileName the name of the calendar on disk.
     */
    bool load(const QString &fileName);
    /** writes out the calendar to disk in vCalendar format. Returns true if
     * successful and false on error.
     * @param fileName the name of the file
     */
    bool save(const QString &fileName,CalFormat *format=0);
    /** clears out the current calendar, freeing all used memory etc. etc. */
    void close();
  
    void addEvent(KOEvent *anEvent);
    /** deletes an event from this calendar. */
    void deleteEvent(KOEvent *);

    /** retrieves an event on the basis of the unique string ID. */
    KOEvent *getEvent(const QString &UniqueStr);
    /** builds and then returns a list of all events that match for the
     * date specified. useful for dayView, etc. etc. */
    QList<KOEvent> eventsForDate(const QDate &date, bool sorted = FALSE);
    QList<KOEvent> eventsForDate(const QDateTime &qdt);
    /** Get events in a range of dates. If inclusive is set to true, only events
     * are returned, which are completely included in the range. */
    QList<KOEvent> events(const QDate &start,const QDate &end,
                             bool inclusive=false);
    /** Return all events in calendar */
    QList<KOEvent> getAllEvents();
  
    /*
     * returns a QString with the text of the holiday (if any) that falls
     * on the specified date.
     */
    QString getHolidayForDate(const QDate &qd);
    
    /** returns the number of events that are present on the specified date. */
    int numEvents(const QDate &qd);
  
    /** add a todo to the todolist. */
    void addTodo(Todo *todo);
    /** remove a todo from the todolist. */
    void deleteTodo(Todo *);
    const QList<Todo> &getTodoList() const;
    /** searches todolist for an event with this unique string identifier,
      returns a pointer or null. */
    Todo *getTodo(const QString &UniqueStr);
    /** Returns list of todos due on the specified date */
    QList<Todo> getTodosForDate(const QDate & date);
  
  signals:
    /** emitted at regular intervals to indicate that the events in the
      list have triggered an alarm. */
    void alarmSignal(QList<KOEvent> &);
    /** emitted whenever an event in the calendar changes.  Emits a pointer
      to the changed event. */
    void calUpdated(Incidence *);
  
  public slots:
    /** checks to see if any alarms are pending, and if so, returns a list
     * of those events that have alarms. */
    void checkAlarms();
   
  protected slots:
    /** this method should be called whenever a KOEvent is modified directly
     * via it's pointer.  It makes sure that the calObject is internally
     * consistent. */
    void updateEvent(Incidence *incidence);
  
  protected:
    /** inserts an event into its "proper place" in the calendar. */
    void insertEvent(const KOEvent *anEvent);
  
    /** on the basis of a QDateTime, forms a hash key for the dictionary. */
    long int makeKey(const QDateTime &dt);
    /** on the basis of a QDate, forms a hash key for the dictionary */
    long int makeKey(const QDate &d);
    /** Return the date for which the specified key was made. */
    QDate keyToDate(long int key);
  
  private:
    QIntDict<QList<KOEvent> > *mCalDict;    // dictionary of lists of events.
    QList<KOEvent> mRecursList;             // list of repeating events.
  
    QList<Todo> mTodoList;               // list of "todo" items.
  
    QDate *mOldestDate;
    QDate *mNewestDate;
};  

#endif
