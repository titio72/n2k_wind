#if defined(PIO_UNIT_TESTING) || defined(NATIVE)
#include <MockEEPROM.h>
#define EEE mockEEPROM
#else
#include <EEPROM.h>
#define EEE EEPROM
#endif
#include <Log.h>
#include "CorrectionPersistence.h"

bool CorrectionPersistence::write(const correction_table &t)
{
    if (!EEE.begin(EEPROM_TOTAL_SIZE))
    {
        Log::trace("[CORR] Error writing correction table (EEPROM not initialized)\n");
        return false;
    }
    EEE.write(CORRECTION_EEPROM_OFFSET, CORRECTION_EEPROM_MAGIC);
    EEE.write(CORRECTION_EEPROM_OFFSET + 1, CORRECTION_EEPROM_VERSION);
    EEE.put(CORRECTION_EEPROM_OFFSET + 2, t);
    bool res = EEE.commit();
    EEE.end();
    Log::trace("[CORR] Correction table written {%s}\n", res ? "OK" : "KO");
    return res;
}

bool CorrectionPersistence::read(correction_table &t)
{
    correction_clear(t);
    if (!EEE.begin(EEPROM_TOTAL_SIZE))
        return false;
    bool ok = false;
    if (EEE.read(CORRECTION_EEPROM_OFFSET) == CORRECTION_EEPROM_MAGIC &&
        EEE.read(CORRECTION_EEPROM_OFFSET + 1) == CORRECTION_EEPROM_VERSION)
    {
        correction_table tmp;
        EEE.get(CORRECTION_EEPROM_OFFSET + 2, tmp);
        if (correction_table_valid(tmp))
        {
            t = tmp;
            ok = true;
        }
    }
    EEE.end();
    Log::trace("[CORR] Read correction table {%s}\n", ok ? "OK" : "none/invalid");
    return ok;
}
