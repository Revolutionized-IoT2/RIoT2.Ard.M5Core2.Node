#pragma once

#include <riot2/Factory.h>

#include "IView.h"

// Project-local (IView itself differs per project - see riot2::Factory's
// own comment) registry mapping a DeviceConfiguration's classFullName to a
// concrete View. Populated by each View's static Registrar (see
// ButtonView.cpp etc.), consulted by ViewManager::rebuild().
using ViewFactory = riot2::Factory<IView>;
