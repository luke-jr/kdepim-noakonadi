#include "knconfigmanager.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kwin.h>

#include <qhbox.h>
#include <qframe.h>

#include "utilities.h"
#include "knglobals.h"
#include "knarticlewidget.h"
#include "knarticlefactory.h"
#include "knodeview.h"

KNConfigManager::KNConfigManager(QObject *p, const char *n) : QObject(p, n), d_ialog(0)
{
  i_dentity           = new KNConfig::Identity();
  a_ppearance         = new KNConfig::Appearance();
  r_eadNewsGeneral    = new KNConfig::ReadNewsGeneral();
  d_isplayedHeaders   = new KNConfig::DisplayedHeaders();
  p_ostNewsTechnical  = new KNConfig::PostNewsTechnical();
  p_ostNewsCompose    = new KNConfig::PostNewsComposer();
  c_leanup            = new KNConfig::Cleanup();
}


KNConfigManager::~KNConfigManager()
{
  delete i_dentity;
  delete a_ppearance;
  delete r_eadNewsGeneral;
  delete d_isplayedHeaders;
  delete p_ostNewsTechnical;
  delete p_ostNewsCompose;
  delete c_leanup;
}


void KNConfigManager::configure()
{
  if(!d_ialog) {
    d_ialog=new KNConfigDialog(this, knGlobals.topWidget, "Preferences_Dlg");
    connect(d_ialog, SIGNAL(finished()), this, SLOT(slotDialogDone()));
    d_ialog->show();
  }
  else
    KWin::setActiveWindow(d_ialog->winId());
}


void KNConfigManager::slotDialogDone()
{
  d_ialog->delayedDestruct();
  d_ialog=0;
}


//===================================================================================================


KNConfigDialog::KNConfigDialog(KNConfigManager *m, QWidget *p, const char *n)
  : KDialogBase(TreeList, i18n("Preferences"), Ok|Apply|Cancel|Help, Ok, p, n, false, true)
{
  setShowIconsInTreeList(true);
  //  setRootIsDecorated(false);

  QStringList list;

  // Set up the folder bitmaps
  list << QString(" ")+i18n("Accounts");
  setFolderIcon(list, UserIcon("server"));

  list.clear();
  list << QString(" ")+i18n("Reading News");
  setFolderIcon(list, BarIcon("mail_get"));

  list.clear();
  list << QString(" ")+i18n("Posting News");
  setFolderIcon(list, BarIcon("mail_forward"));

  // Identity
  QFrame *frame = addHBoxPage(i18n(" Identity"),i18n("Personal Information"), UserIcon("smile"));
  w_idgets.append(new KNConfig::IdentityWidget(m->identity(), frame));

  // Accounts / News
  list.clear();
  list << QString(" ")+i18n("Accounts") << i18n(" News");
  frame = addHBoxPage(list, i18n("Newsgroups Servers"), UserIcon("group"));
  w_idgets.append(new  KNConfig::NntpAccountListWidget(frame));

  // Accounts / Mail
  list.clear();
  list << QString(" ")+i18n("Accounts") << i18n(" Mail");
  frame = addHBoxPage(list, i18n("Mail Server"), BarIcon("mail_send"));
  w_idgets.append(new KNConfig::SmtpAccountWidget(frame));

  // Appearance
  frame = addHBoxPage(QString(" ")+i18n("Appearance"), i18n("Customize visual appearance"), BarIcon("blend"));
  w_idgets.append(new KNConfig::AppearanceWidget(m->appearance(), frame));

  // Read News / General
  list.clear();
  list << QString(" ")+i18n("Reading News") << QString(" ")+i18n("General");
  frame = addHBoxPage(list, i18n("General Options"), BarIcon("misc"));
  w_idgets.append(new KNConfig::ReadNewsGeneralWidget(m->readNewsGeneral(), frame));

  // Read News // Headers
  list.clear();
  list << QString(" ")+i18n("Reading News") << QString(" ")+i18n("Headers");
  frame = addHBoxPage(list, i18n("Customize displayed article headers"), BarIcon("text_block"));
  w_idgets.append(new KNConfig::DisplayedHeadersWidget(m->displayedHeaders(), frame));

  // Read News / Filters
  list.clear();
  list << QString(" ")+i18n("Reading News") << i18n(" Filters");
  frame = addHBoxPage(list,i18n("Article Filters"),BarIcon("filter"));
  w_idgets.append(new KNConfig::FilterListWidget(frame));

  // Post News / Technical
  list.clear();
  list << QString(" ")+i18n("Posting News") << QString(" ")+i18n("Technical");
  frame = addHBoxPage(list, i18n("Technical Settings"), BarIcon("configure"));
  w_idgets.append(new KNConfig::PostNewsTechnicalWidget(m->postNewsTechnical(), frame));

  // Post News / Composer
  list.clear();
  list << QString(" ")+i18n("Posting News") << QString(" ")+i18n("Composer");
  frame = addHBoxPage(list, i18n("Customize composer behaviour"), BarIcon("signature"));
  w_idgets.append(new KNConfig::PostNewsComposerWidget(m->postNewsComposer(), frame));

  // Post News / Spelling
  list.clear();
  list << QString(" ")+i18n("Posting News") << QString(" ")+i18n("Spelling");
  frame = addHBoxPage(list, i18n("Spell checker behavior"), BarIcon("spellcheck"));
  w_idgets.append(new KNConfig::PostNewsSpellingWidget(frame));

  // Cleanup
  frame = addHBoxPage(QString(" ")+i18n("Cleanup"),i18n("Preserving disk space"), BarIcon("wizard"));
  w_idgets.append(new KNConfig::CleanupWidget(m->cleanup(), frame));

  restoreWindowSize("settingsDlg", this, QSize(508,424));

  setHelp("anc-setting-your-identity");
}


KNConfigDialog::~KNConfigDialog()
{
  saveWindowSize("settingsDlg", this->size());
}


void KNConfigDialog::slotApply()
{
  for(KNConfig::BaseWidget *w=w_idgets.first(); w; w=w_idgets.next())
    w->apply();

  KNArticleWidget::configChanged();
  knGlobals.view->configChanged();
  knGlobals.artFactory->configChanged();
}


void KNConfigDialog::slotOk()
{
  slotApply();
  KDialogBase::slotOk();
}


//-----------------------------
#include "knconfigmanager.moc"
