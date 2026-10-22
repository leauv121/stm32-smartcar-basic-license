#ifndef FINAL_WORK_DIAGNOSTIC_VIEW_H
#define FINAL_WORK_DIAGNOSTIC_VIEW_H

#include <stdint.h>

#include "services/error_manager.h"

#define DIAGNOSTIC_LINE_COUNT  4U
#define DIAGNOSTIC_LINE_LENGTH 16U

typedef struct
{
    char line[DIAGNOSTIC_LINE_COUNT][DIAGNOSTIC_LINE_LENGTH + 1U];
} DiagnosticLines;

void DiagnosticView_Format(const ErrorRecord *record,
                           uint8_t index,
                           uint8_t count,
                           DiagnosticLines *out);

#endif
