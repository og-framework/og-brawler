// SPDX-License-Identifier: BUSL-1.1
#include "ConsoleCommandsNamedPipeServer.h"

#ifdef _WIN32

#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"

#include <windows.h>
#include <thread>
#include <sstream>

namespace DAttackPipeServer
{
    void NamedPipeServer()
    {
        while (true)
        {
            const wchar_t* pipeName = L"\\\\.\\pipe\\DAttackPipe";
            HANDLE hPipe = CreateNamedPipeW(
                pipeName,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1, 4096, 4096, 0, NULL);

            if (hPipe == INVALID_HANDLE_VALUE)
                return;

            if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED)
            {
                char buffer[1024] = {};
                DWORD bytesRead = 0;
                // Read until client closes the pipe
                while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';
                    std::istringstream iss(buffer);
                    std::string line;
                    while (std::getline(iss, line))
                    {
                        if (line.empty())
                            continue;

                        std::istringstream lineStream(line);
                        std::string varName, varValue;
                        if (std::getline(lineStream, varName, '=') && std::getline(lineStream, varValue))
                        {
                            if (dAttackMachineSimulation::SetVariable(varName, varValue))
                                continue;
                        }
                    }
                }
                DisconnectNamedPipe(hPipe);
            }

            CloseHandle(hPipe);

        }
    }
}

#endif // _WIN32
