// $Id$

#include <kconfig.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <kdebug.h>
#include <qcolor.h>

#include "kprefs.h"

class KPrefsItemBool : public KPrefsItem {
  public:
    KPrefsItemBool(const QString &group,const QString &name,bool *,bool defaultValue=true);
    virtual ~KPrefsItemBool() {}
    
    void setDefault();
    void readConfig(KConfig *);
    void writeConfig(KConfig *);

  private:
    bool *mReference;
    bool mDefault;
};

class KPrefsItemInt : public KPrefsItem {
  public:
    KPrefsItemInt(const QString &group,const QString &name,int *,int defaultValue=0);
    virtual ~KPrefsItemInt() {}
    
    void setDefault();
    void readConfig(KConfig *);
    void writeConfig(KConfig *);

  private:
    int *mReference;
    int mDefault;
};


class KPrefsItemColor : public KPrefsItem {
  public:
    KPrefsItemColor(const QString &group,const QString &name,QColor *,
                    const QColor &defaultValue=QColor(128,128,128));
    virtual ~KPrefsItemColor() {}
    
    void setDefault();
    void readConfig(KConfig *);
    void writeConfig(KConfig *);    

  private:
    QColor *mReference;
    QColor mDefault;
};


class KPrefsItemFont : public KPrefsItem {
  public:
    KPrefsItemFont(const QString &group,const QString &name,QFont *,
                   const QFont &defaultValue=QFont("helvetica",12));
    virtual ~KPrefsItemFont() {}
    
    void setDefault();
    void readConfig(KConfig *);
    void writeConfig(KConfig *);    

  private:
    QFont *mReference;
    QFont mDefault;
};


class KPrefsItemString : public KPrefsItem {
  public:
    KPrefsItemString(const QString &group,const QString &name,QString *,
                     const QString &defaultValue="");
    virtual ~KPrefsItemString() {}
    
    void setDefault();
    void readConfig(KConfig *);
    void writeConfig(KConfig *);    

  private:
    QString *mReference;
    QString mDefault;
};


KPrefsItemBool::KPrefsItemBool(const QString &group,const QString &name,
                               bool *reference,bool defaultValue) :
  KPrefsItem(group,name)
{
  mReference = reference;
  mDefault = defaultValue;
}

void KPrefsItemBool::setDefault()
{
  *mReference = mDefault;
}

void KPrefsItemBool::writeConfig(KConfig *config)
{
  config->setGroup(mGroup);
  config->writeEntry(mName,*mReference);
}


void KPrefsItemBool::readConfig(KConfig *config)
{
  config->setGroup(mGroup);
  *mReference = config->readBoolEntry(mName,mDefault);
}


KPrefsItemInt::KPrefsItemInt(const QString &group,const QString &name,
                             int *reference,int defaultValue) :
  KPrefsItem(group,name)
{
  mReference = reference;
  mDefault = defaultValue;
}

void KPrefsItemInt::setDefault()
{
  *mReference = mDefault;
}

void KPrefsItemInt::writeConfig(KConfig *config)
{
  config->setGroup(mGroup);
  config->writeEntry(mName,*mReference);
}

void KPrefsItemInt::readConfig(KConfig *config)
{
  config->setGroup(mGroup);
  *mReference = config->readNumEntry(mName,mDefault);
}


KPrefsItemColor::KPrefsItemColor(const QString &group,const QString &name,
                                 QColor *reference,const QColor &defaultValue) :
  KPrefsItem(group,name)
{
  mReference = reference;
  mDefault = defaultValue;
}

void KPrefsItemColor::setDefault()
{
  *mReference = mDefault;
}

void KPrefsItemColor::writeConfig(KConfig *config)
{
  config->setGroup(mGroup);
  config->writeEntry(mName,*mReference);
}

void KPrefsItemColor::readConfig(KConfig *config)
{
  config->setGroup(mGroup);
  *mReference = config->readColorEntry(mName,&mDefault);
}


KPrefsItemFont::KPrefsItemFont(const QString &group,const QString &name,
                               QFont *reference,const QFont &defaultValue) :
  KPrefsItem(group,name)
{
  mReference = reference;
  mDefault = defaultValue;
}

void KPrefsItemFont::setDefault()
{
  *mReference = mDefault;
}

void KPrefsItemFont::writeConfig(KConfig *config)
{
  config->setGroup(mGroup);
  config->writeEntry(mName,*mReference);
}

void KPrefsItemFont::readConfig(KConfig *config)
{
  config->setGroup(mGroup);
  *mReference = config->readFontEntry(mName,&mDefault);
}


KPrefsItemString::KPrefsItemString(const QString &group,const QString &name,
                                   QString *reference,const QString &defaultValue) :
  KPrefsItem(group,name)
{
  mReference = reference;
  mDefault = defaultValue;
}

void KPrefsItemString::setDefault()
{
  *mReference = mDefault;
}

void KPrefsItemString::writeConfig(KConfig *config)
{
  config->setGroup(mGroup);
  config->writeEntry(mName,*mReference);
}

void KPrefsItemString::readConfig(KConfig *config)
{
  config->setGroup(mGroup);
  *mReference = config->readEntry(mName,mDefault);
}


QString *KPrefs::mCurrentGroup = 0;

KPrefs::KPrefs(const QString &configname)
{
  if (!configname.isEmpty()) {
    mConfig = new KConfig(locateLocal("config",configname));
  } else {
    mConfig = KGlobal::config();
  }

  mItems.setAutoDelete(true);

  // Set default group
  if (mCurrentGroup == 0) mCurrentGroup = new QString("No Group");
}

KPrefs::~KPrefs()
{
  if (mConfig != KGlobal::config()) {
    delete mConfig;
  }
}

void KPrefs::setCurrentGroup(const QString &group)
{
  if (mCurrentGroup) delete mCurrentGroup;
  mCurrentGroup = new QString(group);
}

KConfig *KPrefs::config() const
{
  return mConfig;
}

void KPrefs::setDefaults()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->setDefault();
  }

  usrSetDefaults();
}

void KPrefs::readConfig()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->readConfig(mConfig);
  }

  usrReadConfig();
}

void KPrefs::writeConfig()
{
  KPrefsItem *item;
  for(item = mItems.first();item;item = mItems.next()) {
    item->writeConfig(mConfig);
  }

  usrWriteConfig();

  mConfig->sync();
}


void KPrefs::addItem(KPrefsItem *item)
{
  mItems.append(item);
}

void KPrefs::addItemBool(const QString &key,bool *reference,bool defaultValue)
{
  addItem(new KPrefsItemBool(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemInt(const QString &key,int *reference,int defaultValue)
{
  addItem(new KPrefsItemInt(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemColor(const QString &key,QColor *reference,const QColor &defaultValue)
{
  addItem(new KPrefsItemColor(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemFont(const QString &key,QFont *reference,const QFont &defaultValue)
{
  addItem(new KPrefsItemFont(*mCurrentGroup,key,reference,defaultValue));
}

void KPrefs::addItemString(const QString &key,QString *reference,const QString &defaultValue)
{
  addItem(new KPrefsItemString(*mCurrentGroup,key,reference,defaultValue));
}
