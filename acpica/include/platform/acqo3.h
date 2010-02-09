
#include "acgcc.h"

#ifdef __x86_64__
#define ACPI_MACHINE_WIDTH 64
#else
#define ACPI_MACHINE_WIDTH 32
#endif
typedef unsigned long long COMPILER_DEPENDENT_UINT64;
typedef signed long long COMPILER_DEPENDENT_INT64;
#define ACPI_SYSTEM_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE
#define ACPI_SYSTEM_XFACE

#define ACPI_USE_LOCAL_CACHE
#define ACPI_USE_NATIVE_DIVIDE

#define ACPI_DEBUG_OUTPUT
#define ACPI_DISASSEMBLER
