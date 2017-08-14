attributeKeywords = ["broadcast", "urgent", "const"]
typeKeywords = ["int", "bool", "chan", "clock"]
doubleOperators = ["+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ">=", "<=", "==", "!=", "&&", "||", "++", "--"]
singleOperators = ["=", "+", "-", "*", "/", "%", "&", "|", "^", ">", "<", "!", "(", ")", "{", "}", ",", ";"]

def parseDeclaration(text) :
	variables = []
	functions = []

	text = eraseComment(text)
	if not text :
		print("syntax error")
		return (None, None)

	# parse sentence and function
	s = 0
	i = 0
	l = len(text)
	while i < l :
		if text[i] == ";" :
			variables.append(parseSentence(text[s:i])[0])
			s = i + 1
		elif text[i] == "{" :
			c = 0
			e = -1
			while i < l :
				i = i + 1
				if text[i] == "{" :
					c = c + 1
				elif c != 0 and text[i] == "}" :
					c = c - 1
				elif c == 0 and text[i] == "}" :
					e = i
					break
			if e == -1 :
				print("syntax error")
				return (None, None)
			functions.append(parseFunction(text[s:e]))
			s = e + 1
			i = e
		i = i + 1
	return (variables, functions)

def eraseComment(text) :
	# erase "/* */" comment
	while True :
		s = text.find("/*")
		if s == -1 :
			break
		e = text.find("*/")
		if e == -1 :
			print("syntax error")
			return None
		text = text[:s] + text[2+e:]

	# erase "//" comment
	while True :
		s = text.find("//")
		if s == -1 :
			break
		e = text[s:].find("\n")
		if e == -1 :
			text = text[:s]
		else :
			text = text[:s] + text[1+s+e:]

	return text

def parseSentence(text) :
	text = text.replace("\n", "")
	text = text.strip()

	tokens = text.split(" ")

	# check whether declaration or code
	var = {}
	var["attribute"] = None
	var["type"] = None
	var["name"] = None
	var["initial"] = None
	i = 0
	if tokens[i] in attributeKeywords :
		var["attribute"] = tokens[i]
		i = i + 1
	if tokens[i] in typeKeywords :
		var["type"] = tokens[i]
		i = i + 1

	# declaration part
	if var["type"] :
		sstr = ""
		for j in range(i, len(tokens)) :
			sstr = sstr + tokens[j]
		sstr = sstr.replace(" ", "")
		j = sstr.find("=")
		if j == -1 :
			var["name"] = sstr
		else :
			var["name"] = sstr[:j]
			var["initial"] = sstr[j+1:]
		return (var, "variable")

	# code part
	else :
		code = []
		text = text.replace(" ", "")
		text = text.replace("\t", "")

		if(text[0:6] == "return") :
			code.append("return")
			text = text[6:]

		i = 0
		s = 0
		l = len(text)
		while i < l :
			if (i + 1 < l) and (text[i:i+2] in doubleOperators) :
				if text[s:i] :
					code.append(text[s:i])
				code.append(text[i:i+2])
				s = i + 2
				i = i + 1
			elif text[i] in  singleOperators :
				if text[s:i] :
					code.append(text[s:i])
				code.append(text[i:i+1])
				s = i + 1
			i = i + 1
		if text[s:] :
			code.append(text[s:])

		return (code, "code")

def parseFunction(text) :
	text = text.replace("\n", "")
	text = text.strip()

	func = {}

	i = text.find("{")
	head = text[:i]
	body = text[i+1:]

	# parse function head
	i = head.find(" ")
	func["return"] = head[:i]
	head = head[i+1:]
	i = head.find("(")
	func["name"] = head[:i]
	head = head[i+1:]
	i = head.find(")")
	head = head[:i]
	tokens = head.split(",")
	func["parameters"] = []
	for token in tokens :
		token = token.strip()
		if not token :
			continue
		t = token.split(" ")
		i = 0
		var = {}
		var["attribute"] = None
		var["type"] = None
		var["name"] = None
		var["initial"] = None
		if t[i] in attributeKeywords :
			var["attribute"] = t[i]
			i = i + 1
		if t[i] in typeKeywords :
			var["type"] = t[i]
			i = i + 1
		var["name"] = t[i]
		func["parameters"].append(var)

	# parse function body
	func["variables"] = []
	func["code"] = []
	i = 0
	s = 0
	l = len(body)
	while i < l :
		if (i + 1 < l and body[i:i+2] == "if") or \
		   (i + 2 < l and body[i:i+3] == "for") or \
		   (i + 4 < l and body[i:i+5] == "while") or \
		   (i + 5 < l and body[i:i+6] == "switch") :
			c = -1
			for e in range(i, l) :
				if body[e] == "(" :
					c = c + 1
				elif c != 0 and body[e] == ")" :
					c = c - 1
				elif c == 0 and body[e] == ")" :
					break
			e = e + 1
			func["code"].append(parseSentence(body[i:e])[0])
			s = e
			i = e
		elif body[i] in ["{", "}"] :
			func["code"].append(parseSentence(body[i])[0])
			s = i + 1
		elif body[i] in [";", ":"] :
			(sentence, kind) = parseSentence(body[s:i])
			func[kind].append(sentence)
			s = i + 1
		i = i + 1

	return func

def parseLabelFunction(text) :
	code = []
	text = text.replace("\n", "")
	tokens = text.split(",")
	for token in tokens :
		code.append(parseSentence(token)[0])
	return code
