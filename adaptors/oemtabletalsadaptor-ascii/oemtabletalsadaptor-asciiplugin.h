#ifndef OEMTABLETALSADAPTOR_ASCIIPLUGIN_H
#define OEMTABLETALSADAPTOR_ASCIIPLUGIN_H

#include "plugin.h"

class OEMTabletALSAdaptorAsciiPlugin : public Plugin
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "com.nokia.SensorService.Plugin/1.0" FILE "plugin.json")
#endif
private:
        void Register(class Loader& l);
};

#endif
