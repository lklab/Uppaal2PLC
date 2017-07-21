from xml.etree.ElementTree import Element, parse

def parseConfig(configFile) :
	root = parse(configFile).getroot()
	configData = {}
	configData["taskList"] = []
	configData["ioList"] = []

	for data in root.getchildren() :
		if data.tag == "platform" :
			for platform in data.getchildren() :
				if platform.tag == "os" :
					configData["os"] = platform.text
				elif platform.tag == "protocol" :
					configData["protocol"] = platform.text
				elif platform.tag == "period" :
					configData["period"] = platform.text

		elif data.tag == "configuration" :
			for config in data.getchildren() :
				if config.tag == "task" :
					task = {}
					task["type"] = config.get("type")
					configData["taskList"].append(task)
				if config.tag == "io" :
					io = {}
					io["varname"] = config.get("varname")
					io["address"] = config.get("address")
					io["type"] = config.get("type")
					io["direction"] = config.get("direction")
					configData["ioList"].append(io)

	return configData
