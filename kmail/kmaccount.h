/* KMail Account: Abstract base class for accounts.
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmaccount_h
#define kmaccount_h

#include <qstring.h>
#include <qlist.h>
#include <kmsgbox.h>

class KMAcctMgr;
class KMFolder;
class KMAcctFolder;
class KConfig;

class KMAccount: public QObject
{
  Q_OBJECT
  friend class KMAcctMgr;

public:
  virtual ~KMAccount();

  /** Returns type of the account */
  virtual const char* type(void) const = 0;

  /** Returns account name */
  const QString& name(void) const { return mName; }
  virtual void setName(const QString&);

  /** Set intelligent default values to the fields of the account. */
  virtual void init(void) = 0;

  /** There can be exactly one folder that is fed by messages from an
    account. */
  KMFolder* folder(void) const { return ((KMFolder*)mFolder); }
  virtual void setFolder(KMFolder*);

  /** Process new mail for this account if one arrived. Returns TRUE if new
    mail has been found. Whether the mail is automatically loaded to
    an associated folder or not depends on the type of the account. */
  virtual bool processNewMail(void) = 0;

  /** Read config file entries. This method is called by the account
    manager when a new account is created. */
  virtual void readConfig(KConfig& config);

  /** Write all account information to given config file. The config group
      is already properly set by the caller. */
  virtual void writeConfig(KConfig& config);


protected:
  KMAccount(KMAcctMgr* owner, const char* accountName);

  QString       mName;
  KMAcctMgr*    mOwner;
  KMAcctFolder* mFolder;
};


class KMAcctList: public QList<KMAccount>
{
public:
  virtual ~KMAcctList() {}
  short _dummy; // some compilers fail otherwise
};

#endif /*kmaccount_h*/
