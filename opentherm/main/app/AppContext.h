#pragma once

#include "AppProvider.h"
#include "BoardContext.h"
#include "StruxProvider.h"
#include "SystemManager.h"
#include "DeviceDoc.h"
#include "LinkManager.h"
#include "ModemManager.h"

// The application layer's context: owns this product's managers and answers AppProvider.
//
// This is the file a fork edits. Strux's own managers, their init order and their wiring
// all live one layer down in StruxContext, so pulling an improvement from the template
// does not touch anything here — which is the whole reason the layers were split.
//
// Adding an application manager means: create the class taking AppProvider&, add an
// accessor to AppProvider, add the member here, and call its Init() below. The framework
// does not need to be told it exists; the manager registers its own commands and
// settings into Strux from its Init().
class AppContext : public AppProvider
{
public:
    AppContext(BoardContext& board, StruxProvider& strux)
        : board_(board), strux_(strux) {}

    ~AppContext() = default;
    AppContext(const AppContext&) = delete;
    AppContext& operator=(const AppContext&) = delete;

    /// Bring the application up. Called last: every manager here registers into the
    /// framework, so the framework has to be ready before any of this runs.
    void Init()
    {
        // What this product is, in the product's own words — registered like
        // everything else the application tells the framework about itself, and read
        // back by `system describe`. See DeviceDoc.h.
        strux_.getSystemManager().SetDocumentation(
            DeviceDoc::DESCRIPTION, DeviceDoc::INSTRUCTIONS);

        // The modem registers its commands before the link says hello, so nothing
        // the gateway can ask for is missing by the time it is able to ask.
        modemManager_.Init();
        linkManager_.Init();
    }

    /// One pass of the main loop. The lines first: a frame they hold is overwritten
    /// by the next one, while the UART has a ring buffer behind it.
    void Poll()
    {
        modemManager_.Poll();
        linkManager_.Poll();
    }

    StruxProvider& getStrux() override { return strux_; }
    BoardContext& getBoard() override { return board_; }
    LinkManager& getLinkManager() override { return linkManager_; }
    ModemManager& getModemManager() override { return modemManager_; }

private:
    BoardContext& board_;
    StruxProvider& strux_;

    LinkManager linkManager_{ *this };
    ModemManager modemManager_{ *this };
};
