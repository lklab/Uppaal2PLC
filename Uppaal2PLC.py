#!/usr/bin/env python

import sys, os, shutil

from UppaalProjectParser import *
from ConfigParser import *
from CodeGenerator import *

SUPPORTED_OS = ["linux"]
SUPPORTED_PROTO = {"linux" : ["stdio", "soem"]}

if len(sys.argv) != 3 :
	print("usage : %s [UPPAAL project file] [Config file]")
projectFile = sys.argv[1]
configFile = sys.argv[2]

# parse input files
print("Parsing start.")
system = parseUppaalProject(projectFile)
configData = parseConfig(configFile)
print("Parsing is completed.")

# check configed target platform
if not configData["os"].lower() in SUPPORTED_OS :
	print("%s platform is not supported."%configData["os"])
	sys.exit(-1)
if not configData["protocol"].lower() in SUPPORTED_PROTO[configData["os"].lower()] :
	print("%s protocol is not supported."%configData["protocol"])
	sys.exit(-1)

# code generation
print("Code generation start.")
(source, header) = generateCode(system, configData)

# setup path
rootDirectory = os.path.abspath(os.path.curdir)
resourceDirectory = os.path.join(rootDirectory, "resources")
modelSourceFilePath = os.path.join(resourceDirectory, "model.c")
modelHeaderFilePath = os.path.join(resourceDirectory, "model.h")
appName = "PlcApp"

# write to file
modelSourceFile = open(modelSourceFilePath, 'w')
modelHeaderFile = open(modelHeaderFilePath, 'w')
modelSourceFile.write(source)
modelHeaderFile.write(header)
modelSourceFile.close()
modelHeaderFile.close()
print("Model code(model.[ch]) is generated in resources directory.")

# automatic build
print("Automatic build start.")
if os.uname()[0] == "Linux" :
	os.chdir(resourceDirectory)
	success = os.system("make OS=" + configData["os"].upper() + " PROTO=" + configData["protocol"].upper())
	if success is not 0 :
		print("Automatic build is failed.")
		print("See resources directory for manual build.")
	else :
		shutil.move(os.path.join(resourceDirectory, appName), os.path.join(rootDirectory, appName))
		os.remove(modelSourceFilePath)
		os.remove(modelHeaderFilePath)
#		os.system("make clean")
		os.chdir(rootDirectory)
		print("Automatic build is completed.")
else :
	print("This program does not support automatic builds in the current OS.")
	print("See resources directory for manual build.")
