#include "AboutDialog.h"

#include "Version.h"

#include <QtCore/QDebug>
#include <QtCore/QString>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);

    this->labelApplicationVersion->setText(QString("CasparCG Client CRN %1.%2.%3").arg(MAJOR_VERSION).arg(MINOR_VERSION).arg(REVISION_VERSION));

#if QT_POINTER_SIZE == 4
    this->labelQtVersion->setText(QString("Based on Qt %1 (32 bit)").arg(QT_VERSION_STR));
#else
    this->labelQtVersion->setText(QString("Based on Qt %1 (64 bit)").arg(QT_VERSION_STR));
#endif

    this->labelBuildTime->setText(QString("Built on %1").arg(BUILD_TIMESTAMP));
    this->labelBuildRevision->setText(QString("From revision %1").arg(BUILD_VERSION));
    this->labelDatabaseRevision->setText(QString("Database revision %1").arg(DATABASE_VERSION));
}
