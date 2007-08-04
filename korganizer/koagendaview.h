/*
  This file is part of KOrganizer.

  Copyright (c) 2000,2001,2003 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef KOAGENDAVIEW_H
#define KOAGENDAVIEW_H

#include "calprinter.h"

#include <kvbox.h>

#include <q3scrollview.h>
#include <QBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QMenu>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QSplitter>
#include <QVector>

#include "agendaview.h"

class KHBox;
class QPushButton;

class KOAgenda;
class KOAgendaItem;
class KConfig;

namespace KCal {
  class ResourceCalendar;
}

namespace KOrg {
class IncidenceChangerBase;
}

class TimeLabels : public Q3ScrollView
{
  Q_OBJECT
  public:
    explicit TimeLabels( int rows, QWidget *parent = 0, Qt::WFlags f = 0 );

    /** Calculates the minimum width */
    virtual int minimumWidth() const;

    /** updates widget's internal state */
    void updateConfig();

    /**  */
    void setAgenda( KOAgenda *agenda );

    /**  */
    virtual void paintEvent( QPaintEvent *e );

  public slots:
    /** update time label positions */
    void positionChanged();

  protected:
    void drawContents( QPainter *p, int cx, int cy, int cw, int ch );

  private slots:
    /** update the position of the marker showing the mouse position */
    void mousePosChanged(const QPoint &pos);

    void showMousePos();
    void hideMousePos();

    void setCellHeight( double height );

  private:
    int mRows;
    double mCellHeight;
    int mMiniWidth;
    KOAgenda* mAgenda;

    QFrame *mMousePos;  // shows a marker for the current mouse position in y direction
};

class EventIndicator : public QFrame
{
  Q_OBJECT
  public:
    enum Location { Top, Bottom };
    explicit EventIndicator( Location loc = Top, QWidget *parent = 0 );
    virtual ~EventIndicator();

    void changeColumns( int columns );

    void enableColumn( int column, bool enable );

  protected:
    void drawContents( QPainter * );

  private:
    int mColumns;
    KHBox *mTopBox;
    QBoxLayout *mTopLayout;
    Location mLocation;
    QPixmap mPixmap;
    QVector<bool> mEnabled;
};

/**
  KOAgendaView is the agenda-like view used to display events in a single one or
  multi-day view.
*/
class KOAgendaView : public KOrg::AgendaView
{
  Q_OBJECT
  public:
    explicit KOAgendaView( Calendar *cal, QWidget *parent = 0 );
    virtual ~KOAgendaView();

    /** Returns maximum number of days supported by the koagendaview */
    virtual int maxDatesHint();

    /** Returns number of currently shown dates. */
    virtual int currentDateCount();

    /** returns the currently selected events */
    virtual Incidence::List selectedIncidences();

    /** returns the currently selected events */
    virtual DateList selectedDates();

    /** return the default start/end date/time for new events   */
    virtual bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay);

    /** Remove all events from view */
    void clearView();

    CalPrinter::PrintType printType();

    /** start-datetime of selection */
    QDateTime selectionStart() { return mTimeSpanBegin; }
    /** end-datetime of selection */
    QDateTime selectionEnd() { return mTimeSpanEnd; }
    /** returns true if selection is for whole day */
    bool selectedIsAllDay() { return mTimeSpanInAllDay; }
    /** make selected start/end invalid */
    void deleteSelectedDateTime();
    /** returns if only a single cell is selected, or a range of cells */
    bool selectedIsSingleCell();

    void setTypeAheadReceiver( QObject * );

    /** Show only incidences from the given resource. */
    void setResource( KCal::ResourceCalendar *res, const QString &subResource = QString() );

  public slots:
    virtual void updateView();
    virtual void updateConfig();
    virtual void showDates( const QDate &start, const QDate &end );
    virtual void showIncidences( const Incidence::List &incidenceList );

    void insertIncidence( Incidence *incidence, const QDate &curDate, int curCol = -1 );
    void changeIncidenceDisplayAdded( Incidence *incidence );
    void changeIncidenceDisplay( Incidence *incidence, int mode );

    void clearSelection();

    void startDrag( Incidence * );

    void readSettings();
    void readSettings( KConfig * );
    void writeSettings( KConfig * );

    void setContentsPos( int y );

    void setExpandedButton( bool expanded );

    void finishTypeAhead();

    /** reschedule the todo  to the given x- and y- coordinates. Third parameter determines all-day (no time specified) */
    void slotTodoDropped( Todo *, const QPoint &, bool );

    void enableAgendaUpdate( bool enable );
    void setIncidenceChanger( KOrg::IncidenceChangerBase *changer );

    void zoomInHorizontally( const QDate& date=QDate() );
    void zoomOutHorizontally( const QDate& date=QDate() );

    void zoomInVertically( );
    void zoomOutVertically( );

    void zoomView( const int delta, const QPoint &pos,
      const Qt::Orientation orient=Qt::Horizontal );

    void clearTimeSpanSelection();
  signals:
    void toggleExpand();
    void zoomViewHorizontally(const QDate &, int count );

    void timeSpanSelectionChanged();

  protected:
    /** Fill agenda beginning with date startDate */
    void fillAgenda( const QDate &startDate );

    /** Fill agenda using the current set value for the start date */
    void fillAgenda();

    void connectAgenda( KOAgenda *agenda, QMenu *popup, KOAgenda *otherAgenda );

    /** Create labels for the selected dates. */
    void createDayLabels();

    /**
      Set the masks on the agenda widgets indicating, which days are holidays.
    */
    void setHolidayMasks();

    void removeIncidence( Incidence * );
    /**
      Updates the event indicators after a certain incidence was modified or
      removed.
    */
    void updateEventIndicators();

    void updateTimeBarWidth();

  protected slots:
    /** Update event belonging to agenda item */
    void updateEventDates( KOAgendaItem *item );
    /** update just the display of the given incidence, called by a single-shot timer */
    void doUpdateItem();

    void updateEventIndicatorTop( int newY );
    void updateEventIndicatorBottom( int newY );

    /** Updates data for selected timespan */
    void newTimeSpanSelected( const QPoint &start, const QPoint &end );
    /** Updates data for selected timespan for all day event*/
    void newTimeSpanSelectedAllDay( const QPoint &start, const QPoint &end );

  private:
    bool filterByResource( Incidence *incidence );

  private:
    // view widgets
    QGridLayout *mTopLayout;
    QFrame *mDayLabels;
    KHBox *mDayLabelsFrame;
    QBoxLayout *mLayoutDayLabels;
    QFrame *mBottomDayLabels;
    KHBox *mBottomDayLabelsFrame;
    QBoxLayout *mLayoutBottomDayLabels;
    KHBox *mAllDayFrame;
    QWidget *mDummyAllDayLeft;
    QSplitter *mSplitterAgenda;
    QPushButton *mExpandButton;

    KOAgenda *mAllDayAgenda;
    KOAgenda *mAgenda;
    TimeLabels *mTimeLabels;

    DateList mSelectedDates;  // List of dates to be displayed
    int mViewType;

    KOEventPopupMenu *mAgendaPopup;
    KOEventPopupMenu *mAllDayAgendaPopup;

    EventIndicator *mEventIndicatorTop;
    EventIndicator *mEventIndicatorBottom;

    QVector<int> mMinY;
    QVector<int> mMaxY;

    QVector<bool> mHolidayMask;

    QPixmap mExpandedPixmap;
    QPixmap mNotExpandedPixmap;

    QDateTime mTimeSpanBegin;
    QDateTime mTimeSpanEnd;
    bool mTimeSpanInAllDay;
    bool mAllowAgendaUpdate;

    Incidence *mUpdateItem;

    KCal::ResourceCalendar *mResource;
    QString mSubResource;
};

#endif
