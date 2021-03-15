#include "stdafx.h"
#include "ChimeraStyleSheets.h"
#include <qfile.h>
#include <ConfigurationSystems/ConfigStream.h>

QString chimeraStyleSheets::stdStyleSheet ()
{
	QFile File ("stylesheet.css");
	File.open (QFile::ReadOnly);
	QString stylesheet = QLatin1String (File.readAll ());

	auto defs = getDefs();
	for (auto key : defs.keys()) {
		stylesheet.replace(key, defs[key]);
	}

	return stylesheet;
}

QMap<QString, QString> chimeraStyleSheets::getDefs() 
{
	QMap<QString, QString> defMap;
	ConfigStream defStream("StylesheetDefs.txt", true);
	std::string name, replVal;
	while (defStream.peek() != EOF) {
		defStream >> name;
		replVal = defStream.getline();
		defMap[qstr(name)] = qstr(replVal);
	}
	return defMap;
}