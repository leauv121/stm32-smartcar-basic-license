#include "services/error_manager.h"

#include <stddef.h>
#include <string.h>

static ErrorRecord error_records[ERROR_MANAGER_CAPACITY];

static ErrorRecord *ErrorManager_Find(ErrorModule module, ErrorCode code)
{
    uint8_t index;

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if ((error_records[index].occurrence_count != 0U) &&
            (error_records[index].module == module) &&
            (error_records[index].code == code))
        {
            return &error_records[index];
        }
    }

    return NULL;
}

static ErrorRecord *ErrorManager_FindFreeSlot(void)
{
    uint8_t index;

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if (error_records[index].occurrence_count == 0U)
        {
            return &error_records[index];
        }
    }

    /* 故障表已满时，优先复用最早被清除的槽位。 */
    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if (error_records[index].active == 0U)
        {
            return &error_records[index];
        }
    }

    return NULL;
}

void ErrorManager_Init(void)
{
    memset(error_records, 0, sizeof(error_records));
}

void ErrorManager_Report(ErrorModule module,
                         ErrorCode code,
                         ErrorLevel level,
                         uint32_t now_ms,
                         uint8_t auto_recover)
{
    ErrorRecord *record;

    if ((module >= ERR_MODULE_COUNT) || (code == ERROR_CODE_NONE))
    {
        return;
    }

    record = ErrorManager_Find(module, code);
    if (record == NULL)
    {
        record = ErrorManager_FindFreeSlot();
        if (record == NULL)
        {
            return;
        }

        memset(record, 0, sizeof(*record));
        record->module = module;
        record->code = code;
        record->first_seen_ms = now_ms;
    }

    record->active = 1U;
    record->last_seen_ms = now_ms;
    record->auto_recover = (auto_recover != 0U) ? 1U : 0U;
    if (level > record->level)
    {
        record->level = level;
    }
    if (record->occurrence_count < UINT16_MAX)
    {
        record->occurrence_count++;
    }
}

void ErrorManager_Clear(ErrorModule module, ErrorCode code, uint32_t now_ms)
{
    ErrorRecord *record = ErrorManager_Find(module, code);

    if (record != NULL)
    {
        record->active = 0U;
        record->cleared_ms = now_ms;
    }
}

uint8_t ErrorManager_GetActiveCount(void)
{
    uint8_t index;
    uint8_t count = 0U;

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if (error_records[index].active != 0U)
        {
            count++;
        }
    }

    return count;
}

uint8_t ErrorManager_HasFatal(void)
{
    uint8_t index;

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if ((error_records[index].active != 0U) &&
            (error_records[index].level == ERROR_LEVEL_FATAL))
        {
            return 1U;
        }
    }

    return 0U;
}

const ErrorRecord *ErrorManager_GetHighest(void)
{
    const ErrorRecord *highest = NULL;
    uint8_t index;

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        const ErrorRecord *candidate = &error_records[index];

        if (candidate->active == 0U)
        {
            continue;
        }

        if ((highest == NULL) ||
            (candidate->level > highest->level) ||
            ((candidate->level == highest->level) &&
             (candidate->first_seen_ms < highest->first_seen_ms)))
        {
            highest = candidate;
        }
    }

    return highest;
}

const ErrorRecord *ErrorManager_GetDisplayRecord(uint32_t now_ms)
{
    const ErrorRecord *highest = ErrorManager_GetHighest();
    const ErrorRecord *same_level[ERROR_MANAGER_CAPACITY];
    uint8_t index;
    uint8_t count = 0U;
    uint8_t selected;

    if (highest == NULL)
    {
        return NULL;
    }

    for (index = 0U; index < ERROR_MANAGER_CAPACITY; index++)
    {
        if ((error_records[index].active != 0U) &&
            (error_records[index].level == highest->level))
        {
            same_level[count] = &error_records[index];
            count++;
        }
    }

    selected = (uint8_t)((now_ms / ERROR_DISPLAY_ROTATE_INTERVAL_MS) % count);
    return same_level[selected];
}
