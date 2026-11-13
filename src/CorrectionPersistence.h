#ifndef _CORRECTION_PERSISTENCE_H
#define _CORRECTION_PERSISTENCE_H

#include "DataAndConf.h"
#include "WindCorrection.h"

// EEPROM block after the configuration struct: magic, version, correction_table.
#define CORRECTION_EEPROM_MAGIC 0xC7
#define CORRECTION_EEPROM_VERSION 1
#define CORRECTION_EEPROM_OFFSET (sizeof(configuration))

class CorrectionPersistence
{
public:
    virtual ~CorrectionPersistence() {}
    virtual bool write(const correction_table &t);
    // false + cleared table if magic/version/validation fails
    virtual bool read(correction_table &t);
};

#endif
