# ESPlib Web UI Panel

ESPlib supports the concepts of Web UI Panels. These are JSON files which are have specifications for a set of widgets to control aspects of whatever functions the ESP is doing. For instance for a controller of LED string lights, it might have an on/off switch and a color selector. You specify these widgets in the JSON file with any layout requirements and what to do when a widget interaction takes place. This is typically sending a HTTP request to the server which executes some Lua control to activate the hardware the widget is controlling.

## Overview

A Web UI Panel consists of a JSON file specifying the widget layout and behavior and a some Lua code to handle the HTTP requests resulting from the interactions with the widgets. A browser makes a request to an endpoint of the form:

	http://<hostname>/uipanel/<panel name>
	
This request serves a small webpage with references to CSS and JS files which implement the UI. The JS loads the JSON and uses it to build a UI panel. These files are located in the filesystem at:

    /fs/sys/ui/uipanel.css
    /fs/sys/ui/uipanel.js
    /fs/sys/ui/<panel name>.json

The webpage is mostly generic but includes two JS global variables. The first is *uiPanelJSON*, the path to the JSON file. It's used to fetch the JSON file. The second is *uipanelName*, the root name of the panel. It's used to notify the server which panel was used when an interaction takes place. The JSON specifies the widget types which are known to the JS. The widgets are added to the DOM inside a **\<div>** with the id *uipanel*.

When a widget interaction takes place a request is sent to:

	http://<hostname>/uipanel?<params>
	
Params sent are *name* (the **\<panel name>**), *widget* (the widget name), and *value* (the widget value). When received a Lua program located at:

	/sys/ui/<panel name>.lua
	
is executed passing the *widget* and *value* parameters as string arguments. This program then performs whatever operation is appropriate for those arguments.
