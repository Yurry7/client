#pragma once

#include "../Shared.h"

#include <QtCore/QString>

class CORE_EXPORT DurationSelectedEvent
{
    public:
        explicit DurationSelectedEvent(double selected, double current, double fps);

        double getSelected() const;
        double getCurrent() const;
        double getFPS() const;
    private:
        double selected;
        double current;
        double fps;
};
