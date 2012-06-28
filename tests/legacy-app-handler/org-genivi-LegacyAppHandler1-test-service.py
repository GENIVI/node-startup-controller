#!/usr/bin/python
import gobject
import dbus
import dbus.service
import sys
from dbus.mainloop.glib import DBusGMainLoop

class TestService (dbus.service.Object):
	def __init__(self):
		bus_name = dbus.service.BusName(sys.argv[1], bus=dbus.SystemBus())
		dbus.service.Object.__init__(self, bus_name, '/org/genivi/LegacyAppHandler1/test1')

	@dbus.service.method('org.genivi.LegacyAppHandler1.test1')
	def hello(self):
		return "Hello World!"

DBusGMainLoop(set_as_default=True)
myservice = TestService()
loop = gobject.MainLoop()
loop.run()
