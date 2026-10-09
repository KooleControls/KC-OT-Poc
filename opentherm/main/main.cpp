#include "esp_log.h"
#include "Platform.h"
#include "BoardContext.h"
#include "StruxContext.h"
#include "AppContext.h"

static const char* TAG = "main";

// Three layers, bottom to top, and the dependencies run one way only:
//
//   BoardContext   the hardware. Knows nothing above it.
//   StruxContext   the framework. Knows the board? No — see StruxProvider.h.
//   AppContext     this product. Knows both.
//
// Each layer is the same pair: a CONTEXT that owns the layer's instances, and a PROVIDER
// that says what the layer above may reach for — BoardProvider, StruxProvider,
// AppProvider. A manager therefore takes exactly one reference, its own layer's provider,
// and finds everything through it.
//
// The ORDER WITHIN each layer lives in that layer's context, so this file only orders
// the layers.
//
// No RTOS: one thread of execution. Interrupts only set flags and fill buffers; the
// loop below does the work.
BoardContext board;
StruxContext strux;
AppContext application{ board, strux };

int main()
{
    PlatformInit();       // the tick and the clock, before anything can ask for them
    ESP_LOGI(TAG, "Starting up...");

    board.Init();         // hardware first: it depends on nothing
    strux.Init();         // then the framework the application registers into
    application.Init();   // then the product

    ESP_LOGI(TAG, "All layers initialized");

    for (;;)
    {
        board.GetWatchdog().Feed();
        application.Poll();

        // Sleep until the next interrupt; the tick wakes it every millisecond at most.
        __WFI();
    }
}
