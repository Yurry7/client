#pragma once

#include "../Shared.h"

#include <QtCore/QString>

class CORE_EXPORT DurationPlayedEvent
{
    public:
        explicit DurationPlayedEvent(double current, double fps, int channel, int layer);

        double getCurrent() const;
        double getFPS() const;
        int getChannel() const;
        int getLayer() const;
    private:
        double duration;
        double current;
        double fps;
        int channel;
        int layer;
};
