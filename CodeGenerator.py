variableNameList = {}
clockNameList = {}
functionNameList = {}

def generateCode(system, configData) :
	global variableNameList
	global clockNameList
	global functionNameList

	headerCode = """#ifndef _MODEL_H
#define _MODEL_H

#include "uppaal_types.h"

typedef struct MappingInfo
{
	void* variable;
	int size;
	char* address;
	int direction;
} MappingInfo;

int userPeriodicFunc();

extern Program program;
extern Channel* data_exchanged;
extern MappingInfo mapping_list[];

#define PERIOD %(period)sLL

#endif
"""

	gcodeHeader = """#include "model.h"

#define GET_CLOCK(clock) (program.program_clock - (clock))
#define TEST_CLOCK(clock) (program.program_clock - (clock) + 1LL)
#define SET_CLOCK(clock, value) do { clock = program.program_clock - value; } while(0)

"""

	gcodeContextStruct = ""
	gcodeFunctionPrototype = ""

	gcodeTransitionPrototype = ""
	gcodeLocationPrototype = ""

	gcodeChannelDeclaration = ""

	gcodeTransitionDeclaration = ""
	gcodeLocationDeclaration = ""

	gcodeContextDeclaration = ""
	gcodeMappingDeclaration = ""

	gcodeFunctionDeclaration = ""

	# setup name list
	variableNameList[system["name"]] = [var["name"] for var in system["variables"] if var["type"] != "clock" and var["type"] != "chan"]
	clockNameList[system["name"]] = [var["name"] for var in system["variables"] if var["type"] == "clock"]
	functionNameList[system["name"]] = [func["name"] for func in system["functions"]]
	for template in system["templates"] :
		if template["name"] in [task["type"] for task in configData["taskList"]] :
			if template["variables"] :
				variableNameList[template["name"]] = [var["name"] for var in template["variables"] if var["type"] != "clock" and var["type"] != "chan"]
				clockNameList[template["name"]] = [var["name"] for var in template["variables"] if var["type"] == "clock"]
			if template["functions"] :
				functionNameList[template["name"]] = [func["name"] for func in template["functions"]]

	# generate global variale, function code
	gcodeContextStruct += generateContextStruct(system)
	gcodeFunctionPrototype += generateFunctionPrototype(system)
	gcodeChannelDeclaration += generateChannelDeclaration(system)
	gcodeFunctionDeclaration += generateFunctionDeclaration(system)

	# generate template variable, function, elements code
	# for defined in "config file" as task
	for template in system["templates"] :
		if template["name"] in [task["type"] for task in configData["taskList"]] :
			gcodeContextStruct += generateContextStruct(template)
			gcodeFunctionPrototype += generateFunctionPrototype(template)
			gcodeFunctionDeclaration += generateFunctionDeclaration(template)
			gcodeFunctionPrototype += generateLabelFunctionPrototype(template)
			gcodeFunctionDeclaration += generateLabelFunctionDeclaration(template)
			gcodeTransitionPrototype += generateTransitionPrototype(template)
			gcodeLocationPrototype += generateLocationPrototype(template)
			gcodeTransitionDeclaration += generateTransitionDeclaration(template)
			gcodeLocationDeclaration += generateLocationDeclaration(template)

	# generate task instance code
	taskId = 0
	for task in configData["taskList"] :
		for template in system["templates"] :
			if task["type"] == template["name"] :
				task["id"] = taskId
				gcodeContextDeclaration += generateContextDeclaration(template, task)
				taskId = taskId + 1
				break

	# generate program code
	gcodeContextDeclaration += generateProgramCode(system, configData["taskList"])
	gcodeMappingDeclaration += generateMappingCode(configData["ioList"])
	gcodeFunctionDeclaration += generateProgramFunction(system, configData["ioList"])
	headerCode = headerCode%generateHeaderCode(configData)

	modelCode = gcodeHeader + \
				gcodeContextStruct + \
				gcodeFunctionPrototype + \
				gcodeTransitionPrototype + \
				gcodeLocationPrototype + \
				gcodeChannelDeclaration + \
				gcodeTransitionDeclaration + \
				gcodeLocationDeclaration + \
				gcodeContextDeclaration + \
				gcodeMappingDeclaration + \
				gcodeFunctionDeclaration

	return (modelCode, headerCode)

def generateContextStruct(context) :
	code = "typedef struct " + context["name"] + "Context\n{\n"
	tab = "\t"
	if context["variables"] :
		for var in context["variables"] :
			if var["type"] == "chan" :
				pass
			elif var["type"] == "clock" :
				code += tab + "Clock var_" + var["name"] + ";\n"
			else :
				code += tab + var["type"] + " var_" + var["name"] + ";\n"
	code += "} " + context["name"] + "Context;\n\n"
	return code

def generateFunctionPrototype(context) :
	code = ""
	if context["name"] == "Program" :
		for func in context["functions"] :
			codeName = func["return"] + " " + func["name"] + "("
			comma = False
			for param in func["parameters"] :
				if comma :
					codeName += ", "
				codeName += param["type"] + " " + param["name"]
				comma = True
			codeName += ")"
			func["codeName"] = codeName
			code += codeName + ";\n"
	else :
		if context["functions"] :
			for func in context["functions"] :
				codeName = func["return"] + " " + context["name"] + "_" + func["name"] + "(" + context["name"] + "Context* tc"
				for param in func["parameters"] :
					codeName += ", " + param["type"] + " " + param["name"]
				codeName += ")"
				func["codeName"] = codeName
				code += codeName + ";\n"
	return code

def generateChannelDeclaration(system) :
	defCode = "Channel chan_dataExchanged = {\n\tCHANNEL_DATAEXCHANGED\n};\n"
	dataExRefCode = "Channel* data_exchanged = &chan_dataExchanged;\n\n"

	for var in system["variables"] :
		if var["type"] == "chan" :
			if var["name"] == "dataExchanged" :
				continue
				
			defCode += "Channel chan_" + var["name"] + " = {\n\t"
			if var["attribute"] == "urgent" :
				defCode += "CHANNEL_URGENT"
			elif var["attribute"] == "broadcast" :
				defCode += "CHANNEL_BROADCAST"
			else :
				defCode += "CHANNEL_NORMAL"
			defCode += "\n};\n"

	return defCode + dataExRefCode

def generateFunctionDeclaration(context) :
	code = ""
	if context["functions"] :
		for func in context["functions"] :
			code += func["codeName"] + "\n{\n"
			for c in func["code"] :
				code += generateCodeLine(context, c) + "\n"
			code += "}\n"
	return code

def generateLabelFunctionPrototype(template) :
	code = ""

	for location in template["locations"].values() :
		if location["invariant"] :
			codeName = template["name"] + "_invariant_" + location["id"]
			location["invariant"]["codeName"] = codeName
			code += "int " + codeName + "(void* context, int test);\n"

	for transition in template["transitions"] :
		if transition["guard"] :
			codeName = template["name"] + "_guard_" + str(transition["id"])
			transition["guard"]["codeName"] = codeName
			code += "int " + codeName + "(void* context);\n"
		if transition["update"] :
			codeName = template["name"] + "_update_" + str(transition["id"])
			transition["update"]["codeName"] = codeName
			code += "void " + codeName + "(void* context);\n"

	return code

def generateLabelFunctionDeclaration(template) :
	code = ""

	for location in template["locations"].values() :
		if location["invariant"] :
			code += "int " + location["invariant"]["codeName"] + "(void* context, int test)\n{\n"
			innercode = "if(test)\n{\n"
			usetc = False
			for c in location["invariant"]["code"] :
				(tempcode, tempusetc) = generateCodeLineWhetherUsetc(template, c)
				innercode += "return " + tempcode.replace("GET_CLOCK", "TEST_CLOCK") + "\n"
				usetc = usetc or tempusetc
			innercode += "}\nelse\n{\n"
			for c in location["invariant"]["code"] :
				innercode += "return " + generateCodeLine(template, c) + "\n"
			innercode += "}\n}\n"
			if usetc :
				code += template["name"] + "Context* tc = (" + template["name"] + "Context*)context;\n"
			code += innercode

	for transition in template["transitions"] :
		if transition["guard"] :
			code += "int " + transition["guard"]["codeName"] + "(void* context)\n{\n"
			innercode = ""
			usetc = False
			for c in transition["guard"]["code"] :
				(tempcode, tempusetc) = generateCodeLineWhetherUsetc(template, c)
				innercode += "return " + tempcode + "\n"
				usetc = usetc or tempusetc
			innercode += "}\n"
			if usetc :
				code += template["name"] + "Context* tc = (" + template["name"] + "Context*)context;\n"
			code += innercode
		if transition["update"] :
			code += "void " + transition["update"]["codeName"] + "(void* context)\n{\n"
			innercode = ""
			usetc = False
			for c in transition["update"]["code"] :
				(tempcode, tempusetc) = generateCodeLineWhetherUsetc(template, c)
				innercode += tempcode + "\n"
				usetc = usetc or tempusetc
			innercode += "}\n"
			if usetc :
				code += template["name"] + "Context* tc = (" + template["name"] + "Context*)context;\n"
			code += innercode

	return code

def generateTransitionPrototype(template) :
	code = ""
	for transition in template["transitions"] :
		codeName = template["name"] + "_transition_" + str(transition["id"])
		transition["codeName"] = codeName
		code += "extern Transition " + codeName + ";\n"
	return code

def generateLocationPrototype(template) :
	code = ""
	for location in template["locations"].values() :
		codeName = template["name"] + "_location_" + location["id"]
		location["codeName"] = codeName
		code += "extern Location " + codeName + ";\n"
	return code

def generateTransitionDeclaration(template) :
	code = ""

	for transition in template["transitions"] :
		code += "Transition " + transition["codeName"] + " = {\n\t"
		code += "&" + template["locations"][transition["source"]]["codeName"] + ",\n\t"
		code += "&" + template["locations"][transition["target"]]["codeName"] + ",\n\t"

		if transition["sync"] :
			if transition["sync"]["direction"] == "I" :
				code += "&chan_" + transition["sync"]["channel"] + ",\n\tNULL,\n\t"
			elif transition["sync"]["direction"] == "O" :
				code += "NULL,\n\t&chan_" + transition["sync"]["channel"] + ",\n\t"
		else :
			code += "NULL,\n\tNULL,\n\t"

		if transition["guard"] :
			code += transition["guard"]["codeName"] + ",\n\t"
		else :
			code += "NULL,\n\t"
		if transition["update"] :
			code += transition["update"]["codeName"] + "\n};\n\n"
		else :
			code += "NULL\n};\n\n"

	return code

def generateLocationDeclaration(template) :
	code = ""

	for location in template["locations"].values() :
		transitionListName = template["name"] + "_transitionlist_" + location["id"]
		code += "Transition* " + transitionListName + "[] = {\n\t"
		for transitionId in location["transitions"] :
			code += "&" + template["transitions"][transitionId]["codeName"] + ",\n\t"
		code += "NULL\n};\n"

		code += "Location " + location["codeName"] + " = {\n\t"
		code += "(Transition**)&" + transitionListName + ",\n\t"

		if location["urgent"] :
			code += "LOCATION_URGENT,\n\t"
		elif location["committed"] :
			code += "LOCATION_COMMITTED,\n\t"
		else :
			code += "LOCATION_NORMAL,\n\t"

		if location["invariant"] :
			code += location["invariant"]["codeName"] + "\n};\n\n"
		else :
			code += "NULL\n};\n\n"

	return code

def generateContextDeclaration(template, task) :
	code = template["name"] + "Context " + template["name"] + "_context_" + str(task["id"]) + " = { \n\t"
	if template["variables"] :
		for var in template["variables"] :
			if var["type"] != "chan" and var["initial"] :
				code += var["initial"] + ",\n\t"
			else :
				code += "0,\n\t"
	code = code[:-3] + "\n};\n"

	codeName = template["name"] + "_" + str(task["id"])
	task["codeName"] = codeName
	code += "Template " + codeName + " = {\n\t"
	code += "&" + template["name"] + "_context_" + str(task["id"]) + ",\n\t"
	code += "NULL,\n\t"
	code += "sizeof(" + template["name"] + "Context),\n\t"
	code += "&" + template["locations"][template["initial"]]["codeName"] + ",\n\tNULL\n};\n"
	return code

def generateProgramCode(system, taskList) :
	code = "Template* task_list[] = {\n\t"
	for task in taskList :
		code += "&" + task["codeName"] + ",\n\t"
	code += "NULL\n};\n"

	code += "ProgramContext program_context = { \n\t"
	for var in system["variables"] :
		if var["type"] != "chan" :
			if var["initial"] :
				code += var["initial"] + ",\n\t"
			else :
				code += "0,\n\t"
	code = code[:-3] + "\n};\n\n"

	code += "Program program = {\n\t"
	code += "&program_context,\n\t"
	code += "NULL,\n\t"
	code += "sizeof(ProgramContext),\n\t"
	code += "(Template**)&task_list,\n\t"
	code += "0LL\n};\n\n"
	return code

def generateMappingCode(ioList) :
	code = "MappingInfo mapping_list[] = {\n"
	for var in ioList :
		code += "\t{&(program_context.var_" + var["varname"] + "), "
		code += "sizeof(program_context.var_" + var["varname"] + "), "
		code += "\"" + var["address"] + "\", "
		if var["direction"] == "out" :
			code += "0},\n"
		elif var["direction"] == "in" :
			code += "1},\n"
	code += "\t{NULL, 0, NULL, -1}\n};\n"
	return code

def generateProgramFunction(system, ioList) :
	code = "int userPeriodicFunc()\n{\n"
	for template in system["templates"] :
		if template["name"] == "PLCPlatform" :
			for func in template["functions"] :
				if func["name"] == "userPeriodicFunc" :
					for c in func["code"] :
						code += generateCodeLine(template, c) + "\n"
	code += "}\n"

	return code

def generateHeaderCode(configData) :
	code = {}
	code["period"] = configData["period"]
	return code

def generateCodeLine(context, text) :
	return generateCodeLineWhetherUsetc(context, text)[0]

def generateCodeLineWhetherUsetc(context, text) :
	global variableNameList
	global clockNameList
	global functionNameList

	code = ""
	backup = ""
	semicolon = True
	paramReady = False
	commaReady = False
	setclockReady = False
	setclock = False
	usetc = False

	for t in text :
		if t in variableNameList["Program"] :
			if commaReady :
				code += ", "
				commaReady = False
			code += "program_context.var_" + t + " "
		elif variableNameList.get(context["name"]) and t in variableNameList[context["name"]] :
			if commaReady :
				code += ", "
				commaReady = False
			code += "tc -> var_" + t + " "
			usetc = True

		elif t in clockNameList["Program"] :
			backup = code + "program_context.var_" + t
			setclockReady = True
			code += "GET_CLOCK( program_context.var_" + t + " ) "
		elif clockNameList.get(context["name"]) and t in clockNameList[context["name"]] :
			backup = code + "tc -> var_" + t
			setclockReady = True
			code += "GET_CLOCK( tc -> var_" + t + " ) "
			usetc = True

		elif t in functionNameList["Program"] :
			code += t + " "
		elif functionNameList.get(context["name"]) and t in functionNameList[context["name"]] :
			code += context["name"] + "_" + t
			paramReady = True

		elif t in ["if", "for", "while", "switch"] :
			code += t + " "
			semicolon = False
		elif (t is "(") and paramReady :
			code += "( tc "
			usetc = True
			paramReady = False
			commaReady = True
		elif (t is "=") and setclockReady :
			backup = "SET_CLOCK( " + backup + " , "
			code = ""
			setclockReady = False
			setclock = True

		else :
			code += t + " "

	if setclock :
		code = backup + code + ")"
	if semicolon :
		code += ";"

	return (code, usetc)
