#include "DurationWidgetCommon.h"

#include "Global.h"

#include "DatabaseManager.h"
#include "EventManager.h"
#include "Timecode.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>

DurationWidgetCommon::DurationWidgetCommon(QWidget* parent)
    : QWidget(parent)
{
    setupUi(this);

    this->useDropFrameNotation = (DatabaseManager::getInstance().getConfigurationByName("UseDropFrameNotation").getValue() == "true") ? true : false;

    this->updateTimer.setInterval(100);
    this->lcdNumber_2->display(QString("00:00:00").append((this->useDropFrameNotation == true) ? ".00" : ":00"));
    this->lcdNumber_3->display(QString("00:00:00").append((this->useDropFrameNotation == true) ? ".00" : ":00"));
    this->lcdNumber_4->display(QString("00:00:00").append((this->useDropFrameNotation == true) ? ".00" : ":00"));

    QObject::connect(&this->updateTimer, SIGNAL(timeout()), this, SLOT(updateTime()));
    QObject::connect(&EventManager::getInstance(), SIGNAL(durationSelected(const DurationSelectedEvent&)), this, SLOT(durationSelected(const DurationSelectedEvent&)));
    QObject::connect(&EventManager::getInstance(), SIGNAL(durationPlayed(const DurationPlayedEvent&)), this, SLOT(durationPlayed(const DurationPlayedEvent&)));

    this->label_4->setText((DatabaseManager::getInstance().getConfigurationByName("ReverseOscTime").getValue() == "true") ? "REMAINING" : "ELAPSED");

    this->updateTimer.start();
}

void DurationWidgetCommon::durationSelected(const DurationSelectedEvent& event)
{
    double selected = event.getSelected();
    double current = event.getCurrent();
    double fps = event.getFPS();
    if (current == 0 && selected !=0)
    {
        this->label_3->setStyleSheet(QStringLiteral("QLabel{color: rgba(35, 180, 35, 255);}"));
        this->label_3->setText("SELECTED");
        this->lcdNumber_3 ->display(Timecode::fromTime(selected ,fps, this->useDropFrameNotation));
    }
    else
    {
        this->label_3->setStyleSheet(QStringLiteral("QLabel{color: rgba(190, 190, 190, 255);}"));
        this->label_3->setText("REMAINING");
        this->lcdNumber_3 ->display(Timecode::fromTime(current ,fps, this->useDropFrameNotation));
    }

}

void DurationWidgetCommon::durationPlayed(const DurationPlayedEvent& event)
{
    int channel = 1; //show timecode of this channel
    int layer = 10;  //show timecode of this layer
    double current = event.getCurrent();
    double fps = event.getFPS();
    if (channel == event.getChannel() && layer == event.getLayer())
        this->lcdNumber_4 ->display(Timecode::fromTime(current ,fps, this->useDropFrameNotation));
}

void DurationWidgetCommon::updateTime()
{
    this->lcdNumber_2->display(QTime::currentTime().toString());
}
