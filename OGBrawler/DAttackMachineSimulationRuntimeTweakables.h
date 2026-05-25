// SPDX-License-Identifier: BUSL-1.1
#include "OGSimulation/OGExport.h"
#include <atomic>
#include <string>

namespace dAttackMachineSimulation 
{

	//$pipe = new - object System.IO.Pipes.NamedPipeClientStream(".", "DAttackPipe", [System.IO.Pipes.PipeDirection]::Out)
	// $pipe.Connect()
	// $writer = new-object System.IO.StreamWriter($pipe)
	// //$writer.WriteLine("MovementAndAimModeTest=4")
	// // $writer.Flush()
	// // $writer.Dispose()
	// // $pipe.Dispose()
OGBRAWLER_API extern std::atomic<unsigned int> MovementAndAimModeTest;

OGBRAWLER_API bool SetVariable(const std::string& name, const std::string& value);

}
