#!/usr/bin/python
import lutinModule as module
import lutinTools as tools
import lutinDebug as debug

def get_desc():
	return "river : Multi-nodal audio interface"


def create(target):
	myModule = module.Module(__file__, 'river', 'LIBRARY')
	
	myModule.add_src_file([
		'river/debug.cpp',
		'river/Manager.cpp',
		'river/Interface.cpp',
		'river/io/Node.cpp',
		'river/io/Manager.cpp'
		])
	
	myModule.add_module_depend(['audio', 'airtaudio', 'drain', 'ejson'])
	myModule.add_export_path(tools.get_current_path(__file__))
	
	# add the currrent module at the 
	return myModule








