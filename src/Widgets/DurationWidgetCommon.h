#pragma once

#include "Shared.h"
#include "ui_DurationWidgetCommon.h"

#include "Events/DurationSelectedEvent.h"
#include "Events/DurationPlayedEvent.h"
#include "Events/Rundown/ExecutePlayoutCommandEvent.h"

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <QtWidgets/QWidget>

class WIDGETS_EXPORT DurationWidgetCommon : public QWidget, Ui::DurationWidgetCommon
{
    Q_OBJECT

    public:
        explicit DurationWidgetCommon(QWidget* parent = 0);

    private:
        double duration = 0;
        double rundownDuration;
        bool useDropFrameNotation = false;

        QTime time;
        QTime timeSinceStart;
        QTimer updateTimer;

        void resetDuration();
        QString getTimecode();

        Q_SLOT void updateTime();
        Q_SLOT void durationSelected(const DurationSelectedEvent&event);
        Q_SLOT void durationPlayed(const DurationPlayedEvent&event);
//        Q_SLOT void executePlayoutCommand(const ExecutePlayoutCommandEvent&);
};
