#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/xtypes.h>

using rex::X_RESULT;

REX_STUB_RETURN(__imp__XUsbcamGetState, X_ERROR_DEVICE_NOT_CONNECTED)
REX_STUB_RETURN(__imp__XUsbcamSetConfig, X_ERROR_DEVICE_NOT_CONNECTED)
