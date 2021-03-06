#!/usr/bin/python
import realog.debug as debug
import lutin.tools as tools


def get_type():
	return "BINARY"

def get_sub_type():
	return "TEST"

def get_desc():
	return "Multi-nodal audio interface test"

def get_licence():
	return "MPL-2"

def get_compagny_type():
	return "com"

def get_compagny_name():
	return "atria-soft"

def get_maintainer():
	return "authors.txt"

def configure(target, my_module):
	my_module.add_src_file([
	    'test/main.cpp',
	    'test/testAEC.cpp',
	    'test/testEchoDelay.cpp',
	    'test/testFormat.cpp',
	    'test/testMuxer.cpp',
	    'test/testPlaybackCallback.cpp',
	    'test/testPlaybackWrite.cpp',
	    'test/testRecordCallback.cpp',
	    'test/testRecordRead.cpp',
	    'test/testVolume.cpp',
	    ])
	my_module.add_depend([
	    'audio-river',
	    'etest',
	    'etk',
	    'test-debug'
	    ])
	return True


