#ifndef UI_CONFIGURATIONDIALOGADD_H
#define UI_CONFIGURATIONDIALOGADD_H
namespace Ui 
{
    class ConfigurationDialogAdd {};
}
#endif

#ifndef CONFIGURATIONFRAMEADD_H
#define CONFIGURATIONFRAMEADD_H
 
#include <KDialog>


namespace feedsync
{

class ConfigurationDialogAdd : public KDialog
{
    Q_OBJECT
    public:
        ConfigurationDialogAdd( QWidget *parent=0 );
        ~ConfigurationDialogAdd();

    private:
        Ui::ConfigurationDialogAdd ui;

    private slots:
        void slotButtonOkClicked();
};
 
}

#endif
