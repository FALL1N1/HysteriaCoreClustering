#ifndef _PROCESSPRIO_H
#define _PROCESSPRIO_H

#include "Define.h"
#include <string>

#define CONFIG_PROCESSOR_AFFINITY "UseProcessors"
#define CONFIG_HIGH_PRIORITY "ProcessPriority"

void SetProcessPriority(std::string const& logChannel, uint32 affinity, bool highPriority);

#endif
