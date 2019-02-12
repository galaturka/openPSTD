#include "runcommand.h"

/**
 * Executes the run command.
 * 
 * @param parser  A reference to an ArgumentParser instance
 *                containing the supplied arguments
 */
void RunCommand::execute(ArgumentParser* parser) {
	// Verify that all required arguments are supplied
	std::string filename;
	if (!parser->hasOption("-f", &filename)) {
		std::cerr << "[RunCommand] Option \"-f\" is required" << std::endl;
		exit(1);
	}
	if (filename == "") {
		std::cerr << "[RunCommand] Option \"-f\" requires a filename" << std::endl;
		exit(1);
	}
	
	// Read the scene file
	Configuration conf = FileIO::readSceneFile(filename);
	
	// Validate the configuration
	SceneValidator::validateScene(&conf);
	
	// Enable debug output if the debug flag is set
	if (parser->hasOption("--debug", nullptr)) conf.debug = true;
	
	// Create a Solver instance
	Solver solver(&conf);
	
	// Compute the number of timesteps to simulate
	unsigned int numFrames = solver.getNumFrames();
	
	// Loop through all frames to compute
	unsigned int saventhframe = static_cast<unsigned int>(conf.saventhframe);
	for (unsigned int frameID = 0; frameID < numFrames; frameID++) {
		// Let the solver compute a single frame
		solver.computeFrame();
		
		// Save the computed frame
		if (frameID % saventhframe == 0) {
			FileIO::saveFrameFile(filename, &conf);
		}
		
		// Output the progress
		std::cout << "Finished frame: " << frameID << std::endl;
	}
	
	// Simulation finished
	std::cout << "Successfully finished simulation" << std::endl;
}