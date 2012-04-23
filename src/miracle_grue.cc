/**
   MiracleGrue - Model Generator for toolpathing. <http://www.grue.makerbot.com>
   Copyright (C) 2011 Far McKon <Far@makerbot.com>, Hugo Boyer (hugo@makerbot.com)

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

*/



#include <iostream>
#include <string>

#include <stdlib.h>
#include "mgl/abstractable.h"
#include "mgl/configuration.h"
#include "mgl/miracle.h"

#include "mgl/Vector2.h"
#include "clpp/parser.hpp"

using namespace std;
using namespace mgl;


int intFromCharEqualsStr(const std::string& str)
{
	string nb = str.substr(2, str.length()-2);
	int val = atoi(nb.c_str());
	return val;
}

double doubleFromCharEqualsStr(const std::string& str)
{
	string nb = str.substr(2, str.length()-2);
	double val = atof(nb.c_str());
	return val;
}

class ConfigSetter {
public:
	ConfigSetter(Configuration &c, const string &s, const string &n):
		config(c), section(s), name(n) {};
	void set_s(string &val) {
		config[section.c_str()][name.c_str()] = val;
		cout << section + "." + name + " = " + val << endl;
	};
	void set_d(float val) {
		config[section.c_str()][name.c_str()] = val;
		cout << section + "." + name + " = ";
		cout << val << endl;
	};
	void set_i(int val) {
		config[section.c_str()][name.c_str()] = val;
		cout << section + "." + name + " = ";
		cout << val << endl;
	};
	void set_b() {
		config[section.c_str()][name.c_str()] = true;
		cout << section + "." + name + " = true" << endl;
	};
private:
	Configuration &config;
	const string &section;
	const string &name;
};

void parseArgs(Configuration &config,
				int argc,
				char *argv[],
				string &modelFile,
				string &configFileName,
				int &firstSliceIdx,
				int &lastSliceIdx)
{
	firstSliceIdx = -1;
	lastSliceIdx = -1;

	//first get the config parameter and parse the file so that other params can override the
	//config
	clpp::command_line_parameters_parser configparser;
	configparser.add_parameter("-c", "--config", &config, &Configuration::readFromFile)
			.default_value("miracle.config").necessary();
	configparser.parse(argc, argv);


	//get the rest of the parameters
	clpp::command_line_parameters_parser parser;

	ConfigSetter f(config, "slicer", "firstLayerZ");
	parser.add_parameter("-f", "--firstLayerZ", &f, &ConfigSetter::set_d);

	ConfigSetter l(config, "slicer", "layerH");
	parser.add_parameter("-l", "--layerH", &l, &ConfigSetter::set_d);

	ConfigSetter w(config, "slicer", "layerW");
	parser.add_parameter("-w", "--layerW", &w, &ConfigSetter::set_d);

	ConfigSetter t(config, "slicer", "tubeSpacing");
	parser.add_parameter("-t", "--tubeSpacing", &t, &ConfigSetter::set_d);

	ConfigSetter a(config, "slicer", "angle");
	parser.add_parameter("-a", "--angle", &a, &ConfigSetter::set_d);

	ConfigSetter s(config, "slicer", "nbOfShells");
	parser.add_parameter("-s", "--nbOfShells", &s, &ConfigSetter::set_d);

	ConfigSetter d(config, "slicer", "writeDebugScadFiles");
	parser.add_parameter("-d", "--writeDebugScadFiles", &d, &ConfigSetter::set_b);

	ConfigSetter n(config, "slicer", "firstSliceIdx");
	parser.add_parameter("-n", "--firstSliceIdx", &n, &ConfigSetter::set_i);

	firstSliceIdx = config["slicer"]["firstSliceIdx"].asInt();

	ConfigSetter m(config, "slicer", "lastSliceIdx");
	parser.add_parameter("-m", "--lastSliceIdx", &m, &ConfigSetter::set_i);

	lastSliceIdx = config["slicer"]["lastSliceIdx"].asInt();
}



int preConditionsOrShowUsage(int argc, char *argv[])
{
	cout << endl;
	cout << "Miracle-Grue "<< getMiracleGrueVersionStr() << endl;
	cout << "Makerbot Industries 2012"  << endl;
	cout << endl;

	cout << endl;

	if (argc < 2)
	{
		cout << endl;
		cout << endl;
		cout << "This program translates a 3d model file in STL format to GCODE toolpath for a 3D printer "<< endl;
		cout << "It also generates an OpenScad file for visualization"<< endl;
		cout << endl;
		cout << "usage: miracle-grue [OPTIONS] STL FILE" << endl;
		cout << "options: " << endl;
		cout << "  c=file.config : set the configuration file (default is local miracle.config)" << endl;
		cout << "  f=height : override the first layer height" << endl;
		cout << "  l=height : override the layer height" << endl;
		cout << "  w=height : override layer width" << endl;
		cout << "  t=width : override the infill grid width" << endl;
		cout << "  a=angle : override the infill grid inter slice angle (radians)" << endl;
		cout << "  s=shells : override the number of shells" << endl;
		cout << "  n=first slice nb : slice from a specific slice" << endl;
		cout << "  m=last slice nb : stop slicing at specific slice" << endl;
		cout << "  -d : debug mode (creates scad files for each inset error)" << endl;
		cout << endl;
		cout << "It is pitch black. You are likely to be eaten by a grue." << endl;
		return (-1);
	}
	return 0;
}



int main(int argc, char *argv[], char *envp[])
{

	// design by contract ;-)
	int checks = preConditionsOrShowUsage(argc, argv);
	if(checks != 0)
	{
		return checks;
	}

	string modelFile;
	string configFileName = "miracle.config";

    Configuration config;
    try
    {

		config.readFromFile(configFileName.c_str());

		int firstSliceIdx, lastSliceIdx;
		parseArgs(config, argc, argv, modelFile, configFileName, firstSliceIdx, lastSliceIdx);
		// cout << config.asJson() << endl;

		MyComputer computer;
		cout << endl;
		cout << endl;
		cout << "behold!" << endl;
		cout << "Materialization of \"" << modelFile << "\" has begun at " << computer.clock.now() << endl;

		std::string scadFile = "."; // outDir
		scadFile += computer.fileSystem.getPathSeparatorCharacter();
		scadFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile.c_str()).c_str(), ".scad" );

		std::string gcodeFile = ".";
		gcodeFile += computer.fileSystem.getPathSeparatorCharacter();
		gcodeFile = computer.fileSystem.ChangeExtension(computer.fileSystem.ExtractFilename(modelFile.c_str()).c_str(), ".gcode" );

		cout << endl << endl;
		cout << modelFile << " to \"" << gcodeFile << "\" and \"" << scadFile << "\"" << endl;

		GCoder gcoder;
		loadGCoderData(config, gcoder);

		Slicer slicer;
		loadSlicerData(config, slicer);
		std::vector<mgl::SliceData> slices;
		std::vector<Scalar> zIndicies;
		const char* scad = NULL;

		if (scadFile.size() > 0 )
			scad = scadFile.c_str();

		Meshy mesh(slicer.firstLayerZ, slicer.layerH);  
		mesh.readStlFile(modelFile.c_str());

		miracleGrue(gcoder, slicer, modelFile.c_str(),
					scad, gcodeFile.c_str(),
					firstSliceIdx, lastSliceIdx, slices);

    }
    catch(mgl::Exception &mixup)
    {
    	cout << "ERROR: "<< mixup.error << endl;
    	return -1;
    }
}
