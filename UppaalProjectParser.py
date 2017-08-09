from xml.etree.ElementTree import Element, parse
from DeclarationParser import *

def parseUppaalProject(projectFile) :
	project = parse(projectFile)
	root = project.getroot()

	system = {}
	system["name"] = "Program"
	system["variables"] = None
	system["functions"] = None
	system["templates"] = []

	for globalElement in root.getchildren() :
		##### system/declaration #####
		if globalElement.tag == "declaration" :
			(variables, functions) = parseDeclaration(globalElement.text)
			system["variables"] = variables
			system["functions"] = functions
		##### system/declaration #####

		##### system/template #####
		elif globalElement.tag == "template" :
			template = {}
			template["name"] = None
			template["initial"] = None
			template["variables"] = None
			template["functions"] = None
			template["locations"] = {}
			template["transitions"] = []
			transitionId = 0

			for templateElement in globalElement.getchildren() :
				### system/template/name ###
				if templateElement.tag == "name" : # TODO : must FIRST get template name ?? checking...
					template["name"] = templateElement.text
				### system/template/name ###

				### system/template/init ###
				elif templateElement.tag == "init" :
					template["initial"] = templateElement.get("ref")

				### system/template/declaration ###
				elif templateElement.tag == "declaration" :
					(variables, functions) = parseDeclaration(templateElement.text)
					template["variables"] = variables
					template["functions"] = functions
				### system/template/declaration ###

				### system/template/location ###
				elif templateElement.tag == "location" :
					location = {}
					location["name"] = None
					location["id"] = templateElement.get("id")
					location["invariant"] = {}
					location["committed"] = False
					location["urgent"] = False
					location["transitions"] = []

					for locationElement in templateElement.getchildren() :
						# system/template/location/name #
						if locationElement.tag == "name" :
							location["name"] = locationElement.text

						# system/template/location/label #
						elif locationElement.tag == "label" :
							location["invariant"]["code"] = parseLabelFunction(locationElement.text)

						# system/template/location/committed #
						elif locationElement.tag == "committed" :
							location["committed"] = True

						# system/template/location/urgent #
						elif locationElement.tag == "urgent" :
							location["urgent"] = True

					## location end-up part ##
					if template["locations"].get(location["id"]) :
						location["transitions"] = template["locations"][location["id"]]["transitions"] # TODO : test
					template["locations"][location["id"]] = location
					## location end-up part ##
				### system/template/location ###

				### system/template/transition ###
				elif templateElement.tag == "transition" :
					transition = {}
					transition["source"] = None
					transition["target"] = None
					transition["sync"] = {}
					transition["guard"] = {}
					transition["update"] = {}
					transition["id"] = transitionId

					for transitionElement in templateElement.getchildren() :
						# system/template/transition/source #
						if transitionElement.tag == "source" :
							sourceId = transitionElement.get("ref")
							transition["source"] = sourceId
							if template["locations"].get(sourceId) :
								template["locations"][sourceId]["transitions"].append(transitionId)
							else :
								temp = {}
								temp["transitions"] = []
								temp["transitions"].append(transitionId)
								template["locations"][sourceId] = temp

						# system/template/transition/target #
						elif transitionElement.tag == "target" :
							transition["target"] = transitionElement.get("ref")

						# system/template/transition/label #
						elif transitionElement.tag == "label" :
							if transitionElement.get("kind") == "synchronisation" :
								transition["sync"]["channel"] = transitionElement.text[:-1]
								if transitionElement.text[-1] == "!" :
									transition["sync"]["direction"] = "O"
								elif transitionElement.text[-1] == "?" :
									transition["sync"]["direction"] = "I"
							elif transitionElement.get("kind") == "guard" :
								transition["guard"]["code"] = parseLabelFunction(transitionElement.text)
							elif transitionElement.get("kind") == "assignment" :
								transition["update"]["code"] = parseLabelFunction(transitionElement.text)

					## transition end-up part ##
					template["transitions"].append(transition)
					transitionId += 1
					## transition end-up part ##
				### system/template/transition ###

			#### template end-up part ####
			system["templates"].append(template)
		##### system/template #####

	return system
